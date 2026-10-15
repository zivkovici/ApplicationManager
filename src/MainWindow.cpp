#include "MainWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>
#include <QSplitter>
#include <QClipboard>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_worker(nullptr) {
    
    setWindowTitle("Application Manager - Microsoft Store");
    setGeometry(100, 100, 1000, 600);

    setupUI();
    createConnections();

    // Create worker thread
    m_workerThread = std::make_unique<QThread>(this);
    m_worker = new AsyncWorker();
    m_worker->moveToThread(m_workerThread.get());

    // Connect worker signals
    connect(m_worker, &AsyncWorker::searchCompleted, this, &MainWindow::onSearchCompleted);
    connect(m_worker, &AsyncWorker::installCompleted, this, &MainWindow::onInstallCompleted);
    connect(m_worker, &AsyncWorker::uninstallCompleted, this, &MainWindow::onUninstallCompleted);
    connect(m_worker, &AsyncWorker::launchCompleted, this, &MainWindow::onLaunchCompleted);
    connect(m_worker, &AsyncWorker::updateCheckCompleted, this, &MainWindow::onUpdateCheckCompleted);
    connect(m_worker, &AsyncWorker::updateCompleted, this, &MainWindow::onUpdateCompleted);
    connect(m_worker, &AsyncWorker::installedAppsReceived, this, &MainWindow::onInstalledAppsReceived);
    connect(m_worker, &AsyncWorker::operationProgress, this, &MainWindow::onOperationProgress);
    connect(m_worker, &AsyncWorker::errorOccurred, this, &MainWindow::onWorkerError);

    // Connect thread lifecycle
    connect(m_workerThread.get(), &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread->start();

    // Load installed apps on startup
    QMetaObject::invokeMethod(m_worker, "getInstalledApps", Qt::QueuedConnection);
}

MainWindow::~MainWindow() {
    if (m_workerThread) {
        m_workerThread->quit();
        // Bounded wait: a winget operation can still be running (up to 5 min
        // timeout); don't block app shutdown on it, and never call terminate()
        // here - killing the thread mid-QProcess corrupts state instead of
        // cleanly stopping it.
        m_workerThread->wait(3000);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_workerThread) {
        m_workerThread->quit();
        // Don't call terminate() if the wait times out: it force-kills the
        // thread at an arbitrary point, which can be mid-QProcess call and
        // corrupt process state instead of cleanly stopping it. Just let the
        // window close; any in-flight winget process is abandoned harmlessly.
        m_workerThread->wait(3000);
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Search bar section
    QGroupBox* searchGroup = new QGroupBox("Search Microsoft Store", this);
    QHBoxLayout* searchLayout = new QHBoxLayout(searchGroup);

    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Enter app name to search...");
    searchLayout->addWidget(m_searchInput);

    m_searchButton = new QPushButton("Search", this);
    m_searchButton->setMaximumWidth(100);
    searchLayout->addWidget(m_searchButton);

    m_refreshButton = new QPushButton("Refresh Installed", this);
    m_refreshButton->setMaximumWidth(120);
    searchLayout->addWidget(m_refreshButton);

    mainLayout->addWidget(searchGroup);

    // App list table
    m_model = std::make_unique<AppListModel>(this);
    m_appTable = new QTableView(this);
    m_appTable->setModel(m_model.get());
    m_appTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_appTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_appTable->setAlternatingRowColors(true);
    m_appTable->setColumnWidth(0, 200);  // Name
    m_appTable->setColumnWidth(1, 100);  // Version
    m_appTable->setColumnWidth(2, 150);  // Publisher
    m_appTable->setColumnWidth(3, 100);  // Status
    m_appTable->horizontalHeader()->setStretchLastSection(true);
    m_appTable->verticalHeader()->setVisible(false);

    mainLayout->addWidget(m_appTable);

    // Action buttons
    QGroupBox* actionsGroup = new QGroupBox("Actions", this);
    QHBoxLayout* actionsLayout = new QHBoxLayout(actionsGroup);

    m_installButton = new QPushButton("Install", this);
    m_installButton->setMaximumWidth(100);
    actionsLayout->addWidget(m_installButton);

    m_uninstallButton = new QPushButton("Uninstall", this);
    m_uninstallButton->setMaximumWidth(100);
    actionsLayout->addWidget(m_uninstallButton);

    m_launchButton = new QPushButton("Launch", this);
    m_launchButton->setMaximumWidth(100);
    actionsLayout->addWidget(m_launchButton);

    m_updateButton = new QPushButton("Update", this);
    m_updateButton->setMaximumWidth(100);
    actionsLayout->addWidget(m_updateButton);

    actionsLayout->addStretch();

    mainLayout->addWidget(actionsGroup);

    // Status bar
    m_statusLabel = new QLabel("Ready", this);
    m_infoLabel = new QLabel("", this);

    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_infoLabel);

    mainLayout->addLayout(statusLayout);

    updateButtonStates();
}

void MainWindow::createConnections() {
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::onSearchButtonClicked);
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshInstalledAppsClicked);

    connect(m_installButton, &QPushButton::clicked, this, &MainWindow::onInstallButtonClicked);
    connect(m_uninstallButton, &QPushButton::clicked, this, &MainWindow::onUninstallButtonClicked);
    connect(m_launchButton, &QPushButton::clicked, this, &MainWindow::onLaunchButtonClicked);
    connect(m_updateButton, &QPushButton::clicked, this, &MainWindow::onUpdateButtonClicked);

    connect(m_appTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onTableSelectionChanged);
}

void MainWindow::onSearchButtonClicked() {
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) {
        QMessageBox::warning(this, "Search", "Please enter an application name to search.");
        return;
    }

    m_statusLabel->setText("Searching for \"" + query + "\"...");
    m_model->clearApps();
    
    QMetaObject::invokeMethod(m_worker, "searchApps", Qt::QueuedConnection, Q_ARG(QString, query));
}

void MainWindow::onSearchTextChanged(const QString& text) {
    if (text.isEmpty()) {
        m_statusLabel->setText("Ready");
        QMetaObject::invokeMethod(m_worker, "getInstalledApps", Qt::QueuedConnection);
    }
}

void MainWindow::onRefreshInstalledAppsClicked() {
    m_statusLabel->setText("Refreshing installed applications...");
    m_model->clearApps();
    QMetaObject::invokeMethod(m_worker, "getInstalledApps", Qt::QueuedConnection);
}

void MainWindow::onInstallButtonClicked() {
    AppData app = getSelectedApp();
    if (app.id.isEmpty()) {
        QMessageBox::warning(this, "Install", "Please select an application to install.");
        return;
    }

    int ret = QMessageBox::question(this, "Install Application",
        QString("Do you want to install '%1'?").arg(app.name),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        m_statusLabel->setText("Installing " + app.name + "...");
        QMetaObject::invokeMethod(m_worker, "installApp", Qt::QueuedConnection, Q_ARG(QString, app.id));
    }
}

void MainWindow::onUninstallButtonClicked() {
    AppData app = getSelectedApp();
    if (app.id.isEmpty()) {
        QMessageBox::warning(this, "Uninstall", "Please select an application to uninstall.");
        return;
    }

    int ret = QMessageBox::question(this, "Uninstall Application",
        QString("Do you want to uninstall '%1'?").arg(app.name),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        m_statusLabel->setText("Uninstalling " + app.name + "...");
        QMetaObject::invokeMethod(m_worker, "uninstallApp", Qt::QueuedConnection, Q_ARG(QString, app.id));
    }
}

void MainWindow::onLaunchButtonClicked() {
    AppData app = getSelectedApp();
    if (app.id.isEmpty()) {
        QMessageBox::warning(this, "Launch", "Please select an application to launch.");
        return;
    }

    m_statusLabel->setText("Launching " + app.name + "...");
    QMetaObject::invokeMethod(m_worker, "launchApp", Qt::QueuedConnection, Q_ARG(QString, app.id));
}

void MainWindow::onUpdateButtonClicked() {
    AppData app = getSelectedApp();
    if (app.id.isEmpty()) {
        QMessageBox::warning(this, "Update", "Please select an application to update.");
        return;
    }

    m_statusLabel->setText("Checking for updates...");
    QMetaObject::invokeMethod(m_worker, "checkUpdate", Qt::QueuedConnection, Q_ARG(QString, app.id));
}

void MainWindow::onSearchCompleted(const AppList& apps) {
    m_model->clearApps();
    for (const auto& app : apps) {
        m_model->addApp(app);
    }

    m_statusLabel->setText(QString("Found %1 application(s)").arg(apps.size()));
    m_infoLabel->setText("");
}

void MainWindow::onInstallCompleted(const QString& packageId, bool success, const QString& message) {
    if (success) {
        m_statusLabel->setText("Installation completed successfully");
        QMessageBox::information(this, "Install", message);
        
        // Refresh installed apps
        QMetaObject::invokeMethod(m_worker, "getInstalledApps", Qt::QueuedConnection);
    } else {
        m_statusLabel->setText("Installation failed");
        QMessageBox::critical(this, "Install Failed", message);
    }
}

void MainWindow::onUninstallCompleted(const QString& packageId, bool success, const QString& message) {
    if (success) {
        m_statusLabel->setText("Uninstall completed successfully");
        QMessageBox::information(this, "Uninstall", message);
        
        // Refresh installed apps
        QMetaObject::invokeMethod(m_worker, "getInstalledApps", Qt::QueuedConnection);
    } else {
        m_statusLabel->setText("Uninstall failed");
        QMessageBox::critical(this, "Uninstall Failed", message);
    }
}

void MainWindow::onLaunchCompleted(const QString& packageId, bool success) {
    if (success) {
        m_statusLabel->setText("Application launched");
    } else {
        m_statusLabel->setText("Failed to launch application");
        QMessageBox::warning(this, "Launch Failed", "Could not launch the application.");
    }
}

void MainWindow::onUpdateCheckCompleted(const QString& packageId, const QString& availableVersion, bool hasUpdate) {
    if (hasUpdate) {
        int ret = QMessageBox::question(this, "Update Available",
            QString("Version %1 is available. Do you want to update?").arg(availableVersion),
            QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            m_statusLabel->setText("Updating...");
            QMetaObject::invokeMethod(m_worker, "updateApp", Qt::QueuedConnection, Q_ARG(QString, packageId));
        }
    } else {
        QMessageBox::information(this, "Update Check", "Your application is up to date.");
        m_statusLabel->setText("Ready");
    }
}

void MainWindow::onUpdateCompleted(const QString& packageId, bool success, const QString& message) {
    if (success) {
        m_statusLabel->setText("Update completed successfully");
        QMessageBox::information(this, "Update", message);
        
        // Refresh installed apps
        QMetaObject::invokeMethod(m_worker, "getInstalledApps", Qt::QueuedConnection);
    } else {
        m_statusLabel->setText("Update failed");
        QMessageBox::critical(this, "Update Failed", message);
    }
}

void MainWindow::onInstalledAppsReceived(const AppList& apps) {
    m_model->setApps(apps);
    m_statusLabel->setText(QString("Loaded %1 installed application(s)").arg(apps.size()));
}

void MainWindow::onOperationProgress(const QString& packageId, AppStatus status, const QString& message) {
    m_infoLabel->setText(message);
    
    if (!packageId.isEmpty()) {
        AppData app = m_model->getAppById(packageId);
        if (!app.id.isEmpty()) {
            app.status = status;
            app.statusMessage = message;
            m_model->updateApp(app);
        }
    }
}

void MainWindow::onWorkerError(const QString& message) {
    m_statusLabel->setText("Error");
    QMessageBox::critical(this, "Error", message);
}

void MainWindow::updateButtonStates() {
    AppData app = getSelectedApp();
    bool hasSelection = !app.id.isEmpty();
    bool isInstalled = hasSelection && app.status == AppStatus::Installed;

    m_installButton->setEnabled(hasSelection && !isInstalled);
    m_uninstallButton->setEnabled(hasSelection && isInstalled);
    m_launchButton->setEnabled(hasSelection && isInstalled);
    m_updateButton->setEnabled(hasSelection && isInstalled);
}

void MainWindow::onTableSelectionChanged() {
    updateButtonStates();
}

AppData MainWindow::getSelectedApp() const {
    QModelIndexList selected = m_appTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return AppData();
    }
    return m_model->getApp(selected.first().row());
}
