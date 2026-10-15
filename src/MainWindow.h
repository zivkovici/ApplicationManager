#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QLabel>
#include <QThread>
#include <memory>
#include "AppListModel.h"
#include "AsyncWorker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Search and discovery
    void onSearchButtonClicked();
    void onSearchTextChanged(const QString& text);
    void onRefreshInstalledAppsClicked();

    // App actions
    void onInstallButtonClicked();
    void onUninstallButtonClicked();
    void onLaunchButtonClicked();
    void onUpdateButtonClicked();

    // Worker signals
    void onSearchCompleted(const AppList& apps);
    void onInstallCompleted(const QString& packageId, bool success, const QString& message);
    void onUninstallCompleted(const QString& packageId, bool success, const QString& message);
    void onLaunchCompleted(const QString& packageId, bool success);
    void onUpdateCheckCompleted(const QString& packageId, const QString& availableVersion, bool hasUpdate);
    void onUpdateCompleted(const QString& packageId, bool success, const QString& message);
    void onInstalledAppsReceived(const AppList& apps);
    void onOperationProgress(const QString& packageId, AppStatus status, const QString& message);
    void onWorkerError(const QString& message);

    // UI interactions
    void onTableSelectionChanged();

private:
    void setupUI();
    void createConnections();
    void updateButtonStates();
    AppData getSelectedApp() const;

    // UI Elements
    QLineEdit* m_searchInput;
    QPushButton* m_searchButton;
    QPushButton* m_refreshButton;
    QTableView* m_appTable;
    QPushButton* m_installButton;
    QPushButton* m_uninstallButton;
    QPushButton* m_launchButton;
    QPushButton* m_updateButton;
    QLabel* m_statusLabel;
    QLabel* m_infoLabel;

    // Data and workers
    std::unique_ptr<AppListModel> m_model;
    std::unique_ptr<QThread> m_workerThread;
    AsyncWorker* m_worker;
};
