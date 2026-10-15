#pragma once

#include <QAbstractTableModel>
#include <vector>
#include "models/AppData.h"

class AppListModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit AppListModel(QObject* parent = nullptr);

    // Qt Model interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Data management
    void setApps(const AppList& apps);
    void updateApp(const AppData& app);
    void addApp(const AppData& app);
    void removeApp(const QString& packageId);
    void clearApps();

    // Get app data
    AppData getApp(int row) const;
    AppData getAppById(const QString& packageId) const;
    int findAppRow(const QString& packageId) const;

private:
    AppList m_apps;
    
    // Column indices
    enum Column {
        NameColumn = 0,
        VersionColumn,
        PublisherColumn,
        StatusColumn,
        ColumnCount
    };
};
