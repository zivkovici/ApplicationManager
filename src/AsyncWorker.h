#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <functional>
#include "models/AppData.h"

class AsyncWorker : public QObject {
    Q_OBJECT

public:
    AsyncWorker();
    ~AsyncWorker();

    // Enum for operation type
    enum OperationType {
        Search,
        Install,
        Uninstall,
        Launch,
        CheckUpdate,
        Update,
        GetInstalledApps
    };

public slots:
    void searchApps(const QString& query);
    void installApp(const QString& packageId);
    void uninstallApp(const QString& packageId);
    void launchApp(const QString& packageId);
    void checkUpdate(const QString& packageId);
    void updateApp(const QString& packageId);
    void getInstalledApps();

signals:
    // Signals to notify UI of completion
    void searchCompleted(const AppList& apps);
    void installCompleted(const QString& packageId, bool success, const QString& message);
    void uninstallCompleted(const QString& packageId, bool success, const QString& message);
    void launchCompleted(const QString& packageId, bool success);
    void updateCheckCompleted(const QString& packageId, const QString& availableVersion, bool hasUpdate);
    void updateCompleted(const QString& packageId, bool success, const QString& message);
    void installedAppsReceived(const AppList& apps);
    void operationProgress(const QString& packageId, AppStatus status, const QString& message);
    void errorOccurred(const QString& message);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
