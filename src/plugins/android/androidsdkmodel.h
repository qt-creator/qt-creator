/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#pragma once

#include "androidconfigurations.h"

#include <QAbstractItemModel>

#include <memory>

namespace Android {
namespace Internal {

class AndroidSdkManager;

class AndroidSdkModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum PackageColumn {
        packageNameColumn = 0,
        apiLevelColumn,
        packageRevisionColumn,
        operationColumn
    };

    enum ExtraRoles {
        PackageTypeRole = Qt::UserRole + 1,
        PackageStateRole
    };

    explicit AndroidSdkModel(const AndroidConfig &config, AndroidSdkManager *sdkManager,
                             QObject *parent = 0);

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

private:
    void clearContainers();
    void refreshData();

private:
    const AndroidConfig &m_config;
    AndroidSdkManager *m_sdkManager;
    QList<const SdkPlatform *> m_sdkPlatforms;
    QList<const AndroidSdkPackage *> m_tools;
    QSet<const AndroidSdkPackage *> m_changeState;
};

} // namespace Internal
} // namespace Android
