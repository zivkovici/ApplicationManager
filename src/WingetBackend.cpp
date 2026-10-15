#include "WingetBackend.h"

#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>
#include <QMap>
#include <QVector>
#include <QList>
#include <QTemporaryFile>
#include <QTextStream>
#include <QDir>
#include <algorithm>

namespace {
// winget has no JSON output; parse its fixed-column text table instead.
// Column boundaries are taken from the start offset of each header word.
QVector<QMap<QString, QString>> parseWingetTable(const QString& output) {
    QVector<QMap<QString, QString>> rows;

    // winget redraws progress spinners with a bare '\r' (no '\n'), which glues
    // spinner frames and even the real header onto one '\n'-delimited chunk.
    // For each chunk, keep only the text after the last '\r' - i.e. what a
    // real terminal would actually display after all overwrites resolve.
    QStringList lines;
    for (const QString& rawLine : output.split('\n')) {
        QStringList segments = rawLine.split('\r');
        QString visible;
        for (int s = segments.size() - 1; s >= 0; --s) {
            if (!segments[s].isEmpty()) {
                visible = segments[s];
                break;
            }
        }
        lines.push_back(visible);
    }

    int headerIndex = -1;
    for (int i = 0; i < lines.size() - 1; ++i) {
        QString next = lines[i + 1].trimmed();
        // Require a real separator line, not a single-character progress
        // spinner frame (winget animates with '-', '\\', '|' while working).
        if (next.length() >= 10 && next.count('-') == next.length()) {
            headerIndex = i;
            break;
        }
    }
    if (headerIndex < 0) {
        return rows;
    }

    const QString headerLine = lines[headerIndex];
    QStringList headers;
    QList<int> starts;
    QRegularExpression wordRe("\\S+");
    auto it = wordRe.globalMatch(headerLine);
    while (it.hasNext()) {
        auto m = it.next();
        headers.push_back(m.captured());
        starts.push_back(m.capturedStart());
    }
    if (headers.isEmpty()) {
        return rows;
    }

    for (int i = headerIndex + 2; i < lines.size(); ++i) {
        QString line = lines[i];
        if (line.trimmed().isEmpty()) {
            continue;
        }

        QMap<QString, QString> row;
        for (int c = 0; c < headers.size(); ++c) {
            int start = starts[c];
            if (start >= line.length()) {
                row[headers[c]] = "";
                continue;
            }
            int end = (c + 1 < starts.size()) ? starts[c + 1] : line.length();
            row[headers[c]] = line.mid(start, end - start).trimmed();
        }
        if (!row.value("Name").isEmpty()) {
            rows.push_back(row);
        }
    }
    return rows;
}

// winget may emit UTF-16LE text instead of UTF-8/ANSI when its stdout is redirected.
QString decodeProcessOutput(const QByteArray& raw) {
    if (raw.size() >= 4) {
        int sampleSize = static_cast<int>(std::min<qsizetype>(raw.size(), 200));
        int zeroCount = 0;
        for (int i = 1; i < sampleSize; i += 2) {
            if (raw.at(i) == '\0') {
                ++zeroCount;
            }
        }
        if (zeroCount > sampleSize / 4) {
            return QString::fromUtf16(reinterpret_cast<const char16_t*>(raw.constData()), raw.size() / 2);
        }
    }
    return QString::fromUtf8(raw);
}
}

WingetBackend::WingetBackend() {
    checkWingetInstallation();
}

WingetBackend::~WingetBackend() = default;

bool WingetBackend::isWingetAvailable() const {
    QProcess process;
    process.setProgram("cmd.exe");
    process.setNativeArguments("/c winget --version");
    process.start();
    bool finished = process.waitForFinished(5000);
    return finished && process.exitCode() == 0;
}

bool WingetBackend::checkWingetInstallation() {
    if (!isWingetAvailable()) {
        m_lastError = "winget is not installed or not in PATH. "
                     "Please install Windows Package Manager from the Microsoft Store.";
        return false;
    }
    return true;
}

QString WingetBackend::executeWingetCommand(const QStringList& arguments, int timeoutMs) {
    // Launching the winget App Execution Alias directly with redirected stdio
    // hangs indefinitely (a known Windows quirk), and cmd.exe's inline quote
    // parsing mangles arguments with embedded quotes/spaces. Writing the
    // command to a script file sidesteps both: cmd just reads it like a
    // normal typed command line.
    QString commandLine = "winget";
    for (const QString& arg : arguments) {
        commandLine += ' ';
        commandLine += arg.contains(' ') ? ('"' + arg + '"') : arg;
    }

    QTemporaryFile scriptFile(QDir::tempPath() + "/wingetXXXXXX.bat");
    if (!scriptFile.open()) {
        m_lastError = "Failed to create temporary script file";
        return "";
    }
    scriptFile.setAutoRemove(true);
    {
        QTextStream stream(&scriptFile);
        stream << "@echo off\r\n" << commandLine << "\r\n";
    }
    scriptFile.close();

    QProcess process;
    // Bind stdin to NUL: if winget/cmd ever tries to read from stdin (e.g. a
    // prompt), an open-but-empty pipe would block the read forever.
    process.setStandardInputFile(QProcess::nullDevice());
    process.start("cmd.exe", QStringList() << "/c" << scriptFile.fileName());

    if (!process.waitForFinished(timeoutMs)) {
        m_lastError = "Command timed out: winget " + arguments.join(' ');
        process.kill();
        return "";
    }

    if (process.exitCode() != 0) {
        QString errorOutput = decodeProcessOutput(process.readAllStandardError());
        QString stdOutput = decodeProcessOutput(process.readAllStandardOutput());
        // winget often reports the real failure reason on stdout, not stderr.
        m_lastError = "winget command failed: " + (errorOutput.isEmpty() ? stdOutput.trimmed() : errorOutput);
        qWarning().noquote() << "WingetBackend error:" << m_lastError;
        return "";
    }

    return decodeProcessOutput(process.readAllStandardOutput());
}

AppList WingetBackend::searchApps(const QString& query) {
    if (!isWingetAvailable()) {
        m_lastError = "winget is not available";
        return AppList();
    }

    QString output = executeWingetCommand({"search", query, "--source", "msstore", "--accept-source-agreements"});
    
    if (output.isEmpty()) {
        return AppList();
    }

    return parseSearchResults(output);
}

AppList WingetBackend::parseSearchResults(const QString& tableOutput) {
    AppList results;

    for (const auto& row : parseWingetTable(tableOutput)) {
        AppData app;
        app.id = row.value("Id");
        app.name = row.value("Name");
        app.version = row.value("Version");
        app.status = AppStatus::Available;

        if (!app.id.isEmpty() && !app.name.isEmpty()) {
            results.push_back(app);
        }
    }

    return results;
}

AppList WingetBackend::getInstalledApps() {
    if (!isWingetAvailable()) {
        m_lastError = "winget is not available";
        return AppList();
    }

    QString output = executeWingetCommand({"list", "--accept-source-agreements"});
    
    if (output.isEmpty()) {
        return AppList();
    }

    return parseInstalledApps(output);
}

AppList WingetBackend::parseInstalledApps(const QString& tableOutput) {
    AppList results;

    for (const auto& row : parseWingetTable(tableOutput)) {
        AppData app;
        app.id = row.value("Id");
        app.name = row.value("Name");
        app.version = row.value("Version");
        app.status = AppStatus::Installed;
        app.lastChecked = QDateTime::currentDateTime();

        if (!app.id.isEmpty() && !app.name.isEmpty()) {
            results.push_back(app);
        }
    }

    return results;
}

bool WingetBackend::installApp(const QString& packageId) {
    if (!isWingetAvailable()) {
        m_lastError = "winget is not available";
        return false;
    }

    QString output = executeWingetCommand({"install", "--id", packageId, "--source", "msstore", "--accept-package-agreements", "--accept-source-agreements", "--silent"}, 300000);

    // winget's install auto-upgrades already-installed packages; treat "no
    // update needed" as success rather than a failure.
    if (output.isEmpty() && m_lastError.contains("No available upgrade found", Qt::CaseInsensitive)) {
        m_lastError.clear();
        return true;
    }

    return !output.isEmpty() || m_lastError.isEmpty();
}

bool WingetBackend::uninstallApp(const QString& packageId) {
    if (!isWingetAvailable()) {
        m_lastError = "winget is not available";
        return false;
    }

    QString output = executeWingetCommand({"uninstall", "--id", packageId, "--accept-source-agreements", "--silent"}, 300000);
    
    return !output.isEmpty() || m_lastError.isEmpty();
}

bool WingetBackend::launchApp(const QString& packageId) {
    // Try to find and launch the app using its package ID
    // This is a simplified implementation - in a real app you'd parse the registry
    QProcess process;
    process.startDetached(QString("cmd.exe"), QStringList() << "/c" << QString("start \"\" \"%1\"").arg(packageId));
    return true;
}

bool WingetBackend::checkUpdate(const QString& packageId, QString& availableVersion) {
    if (!isWingetAvailable()) {
        m_lastError = "winget is not available";
        return false;
    }

    QString output = executeWingetCommand({"upgrade", "--id", packageId, "--source", "msstore", "--accept-source-agreements"});
    
    if (output.isEmpty()) {
        return false;
    }

    for (const auto& row : parseWingetTable(output)) {
        if (row.value("Id").compare(packageId, Qt::CaseInsensitive) == 0) {
            availableVersion = row.value("Available");
            return !availableVersion.isEmpty();
        }
    }

    return false;
}

bool WingetBackend::updateApp(const QString& packageId) {
    if (!isWingetAvailable()) {
        m_lastError = "winget is not available";
        return false;
    }

    QString output = executeWingetCommand({"upgrade", "--id", packageId, "--source", "msstore", "--accept-package-agreements", "--accept-source-agreements", "--silent"}, 300000);
    
    return !output.isEmpty() || m_lastError.isEmpty();
}

AppData WingetBackend::getAppInfo(const QString& packageId) {
    if (!isWingetAvailable()) {
        m_lastError = "winget is not available";
        return AppData();
    }

    QString output = executeWingetCommand({"show", "--id", packageId, "--source", "msstore", "--accept-source-agreements"});
    
    AppData app;
    app.id = packageId;
    // Parse the output to fill in app details
    // This is a simplified implementation
    return app;
}
