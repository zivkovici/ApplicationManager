#pragma once

#include <QString>
#include <QDateTime>
#include <vector>

enum class AppStatus {
    Available,    // Available but not installed
    Installing,   // Currently installing
    Installed,    // Successfully installed
    Updating,     // Currently updating
    Removing,     // Currently being removed
    Error         // Error occurred
};

inline QString statusToString(AppStatus status) {
    switch (status) {
        case AppStatus::Available: return "Available";
        case AppStatus::Installing: return "Installing...";
        case AppStatus::Installed: return "Installed";
        case AppStatus::Updating: return "Updating...";
        case AppStatus::Removing: return "Removing...";
        case AppStatus::Error: return "Error";
        default: return "Unknown";
    }
}

struct AppData {
    QString id;                    // Package ID from winget
    QString name;                  // Display name
    QString version;               // Current version
    QString availableVersion;      // Available version for updates
    QString publisher;             // Publisher name
    QString description;           // App description
    AppStatus status;              // Current status
    QString statusMessage;         // Detailed status message
    QDateTime lastChecked;         // When we last checked status
    bool canUpdate;               // True if update is available
    QString installPath;          // Installation path (if installed)
    
    AppData() : status(AppStatus::Available), canUpdate(false) {}
    
    AppData(const QString& _id, const QString& _name, const QString& _version,
            const QString& _publisher, const QString& _description)
        : id(_id), name(_name), version(_version), publisher(_publisher),
          description(_description), status(AppStatus::Available), 
          canUpdate(false) {}
};

using AppList = std::vector<AppData>;
