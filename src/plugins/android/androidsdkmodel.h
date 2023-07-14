// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidconfigurations.h"

#include <QAbstractItemModel>

#include <memory>

namespace Android {
namespace Internal {

class AndroidSdkManager;

class AndroidSdkModel : public QAbstractItemModel
{
public:
    enum PackageColumn {
        packageNameColumn = 0,
        apiLevelColumn,
        packageRevisionColumn
    };

    enum ExtraRoles {
        PackageTypeRole = Qt::UserRole + 1,
        PackageStateRole
    };

    explicit AndroidSdkModel(const AndroidConfig &config, AndroidSdkManager *sdkManager,
                             QObject *parent = nullptr);

    // QAbstractItemModel overrides.
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void selectMissingEssentials();
    QList<const AndroidSdkPackage *> userSelection() const;
    void resetSelection();

    QStringList missingEssentials() const { return m_missingEssentials; }

private:
    void clearContainers();
    void refreshData();

    const AndroidConfig &m_config;
    AndroidSdkManager *m_sdkManager;
    QList<const SdkPlatform *> m_sdkPlatforms;
    QList<const AndroidSdkPackage *> m_tools;
    QSet<const AndroidSdkPackage *> m_changeState;
    QStringList m_missingEssentials;
};

} // namespace Internal
} // namespace Android
