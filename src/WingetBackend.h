#pragma once

#include <QString>
#include <vector>
#include <functional>
#include "models/AppData.h"

class WingetBackend {
public:
    WingetBackend();
    ~WingetBackend();

    // Check if winget is available
    bool isWingetAvailable() const;

    // Search for apps - returns list of available apps
    AppList searchApps(const QString& query);

    // Get list of installed apps
    AppList getInstalledApps();

    // Install an app by its package ID
    bool installApp(const QString& packageId);

    // Uninstall an app
    bool uninstallApp(const QString& packageId);

    // Launch an installed app
    bool launchApp(const QString& packageId);

    // Check for updates for a specific app
    bool checkUpdate(const QString& packageId, QString& availableVersion);

    // Update an app
    bool updateApp(const QString& packageId);

    // Get detailed info about an app
    AppData getAppInfo(const QString& packageId);

    // Get command output (for debugging)
    QString getLastError() const { return m_lastError; }

private:
    QString m_lastError;

    // Helper method to run winget with the given arguments and get its text output
    QString executeWingetCommand(const QStringList& arguments, int timeoutMs = 60000);

    // Parse winget's table-formatted text output
    AppList parseSearchResults(const QString& tableOutput);
    AppList parseInstalledApps(const QString& tableOutput);

    // Check if winget CLI is installed
    bool checkWingetInstallation();
};
