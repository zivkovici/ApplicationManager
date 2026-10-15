#include "AppListModel.h"
#include <QBrush>
#include <QColor>
#include <algorithm>

AppListModel::AppListModel(QObject* parent)
    : QAbstractTableModel(parent) {
}

int AppListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_apps.size());
}

int AppListModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant AppListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_apps.size())) {
        return QVariant();
    }

    const AppData& app = m_apps[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case NameColumn:
                return app.name;
            case VersionColumn:
                return app.version;
            case PublisherColumn:
                return app.publisher;
            case StatusColumn:
                return statusToString(app.status);
            default:
                return QVariant();
        }
    }
    else if (role == Qt::BackgroundRole) {
        // Color code status
        switch (app.status) {
            case AppStatus::Installing:
            case AppStatus::Updating:
                return QBrush(QColor(200, 200, 100));  // Yellow
            case AppStatus::Installed:
                return QBrush(QColor(100, 200, 100));  // Green
            case AppStatus::Error:
                return QBrush(QColor(200, 100, 100));  // Red
            default:
                return QVariant();
        }
    }
    else if (role == Qt::ToolTipRole) {
        if (index.column() == StatusColumn && !app.statusMessage.isEmpty()) {
            return app.statusMessage;
        }
    }

    return QVariant();
}

QVariant AppListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case NameColumn:
                return "Application Name";
            case VersionColumn:
                return "Version";
            case PublisherColumn:
                return "Publisher";
            case StatusColumn:
                return "Status";
            default:
                return QVariant();
        }
    }
    return QVariant();
}

void AppListModel::setApps(const AppList& apps) {
    beginResetModel();
    m_apps = apps;
    endResetModel();
}

void AppListModel::updateApp(const AppData& app) {
    int row = findAppRow(app.id);
    if (row >= 0 && row < static_cast<int>(m_apps.size())) {
        m_apps[row] = app;
        emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
    }
}

void AppListModel::addApp(const AppData& app) {
    // Check if app already exists
    int row = findAppRow(app.id);
    if (row >= 0) {
        updateApp(app);
        return;
    }

    beginInsertRows(QModelIndex(), m_apps.size(), m_apps.size());
    m_apps.push_back(app);
    endInsertRows();
}

void AppListModel::removeApp(const QString& packageId) {
    int row = findAppRow(packageId);
    if (row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        m_apps.erase(m_apps.begin() + row);
        endRemoveRows();
    }
}

void AppListModel::clearApps() {
    beginResetModel();
    m_apps.clear();
    endResetModel();
}

AppData AppListModel::getApp(int row) const {
    if (row >= 0 && row < static_cast<int>(m_apps.size())) {
        return m_apps[row];
    }
    return AppData();
}

AppData AppListModel::getAppById(const QString& packageId) const {
    auto it = std::find_if(m_apps.begin(), m_apps.end(),
                          [&packageId](const AppData& app) { return app.id == packageId; });
    if (it != m_apps.end()) {
        return *it;
    }
    return AppData();
}

int AppListModel::findAppRow(const QString& packageId) const {
    auto it = std::find_if(m_apps.begin(), m_apps.end(),
                          [&packageId](const AppData& app) { return app.id == packageId; });
    if (it != m_apps.end()) {
        return std::distance(m_apps.begin(), it);
    }
    return -1;
}
