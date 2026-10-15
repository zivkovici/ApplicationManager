#include "AsyncWorker.h"
#include "WingetBackend.h"

class AsyncWorker::Impl {
public:
    WingetBackend backend;
};

AsyncWorker::AsyncWorker() : pImpl(std::make_unique<Impl>()) {
}

AsyncWorker::~AsyncWorker() = default;

void AsyncWorker::searchApps(const QString& query) {
    emit operationProgress("", AppStatus::Available, "Searching for apps...");
    
    AppList results = pImpl->backend.searchApps(query);
    emit searchCompleted(results);
}

void AsyncWorker::installApp(const QString& packageId) {
    emit operationProgress(packageId, AppStatus::Installing, "Starting installation...");
    
    bool success = pImpl->backend.installApp(packageId);
    QString message = success ? "Installation completed" : pImpl->backend.getLastError();
    
    emit installCompleted(packageId, success, message);
    emit operationProgress(packageId, success ? AppStatus::Installed : AppStatus::Error, message);
}

void AsyncWorker::uninstallApp(const QString& packageId) {
    emit operationProgress(packageId, AppStatus::Removing, "Starting removal...");
    
    bool success = pImpl->backend.uninstallApp(packageId);
    QString message = success ? "Removal completed" : pImpl->backend.getLastError();
    
    emit uninstallCompleted(packageId, success, message);
    emit operationProgress(packageId, success ? AppStatus::Available : AppStatus::Error, message);
}

void AsyncWorker::launchApp(const QString& packageId) {
    bool success = pImpl->backend.launchApp(packageId);
    emit launchCompleted(packageId, success);
}

void AsyncWorker::checkUpdate(const QString& packageId) {
    QString availableVersion;
    bool hasUpdate = pImpl->backend.checkUpdate(packageId, availableVersion);
    
    emit updateCheckCompleted(packageId, availableVersion, hasUpdate);
}

void AsyncWorker::updateApp(const QString& packageId) {
    emit operationProgress(packageId, AppStatus::Updating, "Starting update...");
    
    bool success = pImpl->backend.updateApp(packageId);
    QString message = success ? "Update completed" : pImpl->backend.getLastError();
    
    emit updateCompleted(packageId, success, message);
    emit operationProgress(packageId, success ? AppStatus::Installed : AppStatus::Error, message);
}

void AsyncWorker::getInstalledApps() {
    emit operationProgress("", AppStatus::Available, "Fetching installed apps...");
    
    AppList apps = pImpl->backend.getInstalledApps();
    emit installedAppsReceived(apps);
}
