/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "itemlibraryassetsdirsmodel.h"
#include "itemlibraryassetsmodel.h"

#include <QDebug>
#include <QMetaProperty>

namespace QmlDesigner {

ItemLibraryAssetsDirsModel::ItemLibraryAssetsDirsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // add roles
    const QMetaObject meta = ItemLibraryAssetsDir::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(i, meta.property(i).name());
}

QVariant ItemLibraryAssetsDirsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid index requested: " << QString::number(index.row());
        return {};
    }

    if (m_roleNames.contains(role))
        return m_dirs[index.row()]->property(m_roleNames[role]);

    qWarning() << Q_FUNC_INFO << "Invalid role requested: " << QString::number(role);
    return {};
}

bool ItemLibraryAssetsDirsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // currently only dirExpanded property is updatable
    if (index.isValid() && m_roleNames.contains(role)) {
        QVariant currValue = m_dirs.at(index.row())->property(m_roleNames.value(role));
        if (currValue != value) {
            m_dirs.at(index.row())->setProperty(m_roleNames.value(role), value);
            if (m_roleNames.value(role) == "dirExpanded")
                ItemLibraryAssetsModel::saveExpandedState(value.toBool(), m_dirs.at(index.row())->dirPath());
            emit dataChanged(index, index, {role});
            return true;
        }
    }
    return false;
}

int ItemLibraryAssetsDirsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_dirs.size();
}

QHash<int, QByteArray> ItemLibraryAssetsDirsModel::roleNames() const
{
    return m_roleNames;
}

void ItemLibraryAssetsDirsModel::addDir(ItemLibraryAssetsDir *assetsDir)
{
    m_dirs.append(assetsDir);
}

const QList<ItemLibraryAssetsDir *> ItemLibraryAssetsDirsModel::assetsDirs() const
{
    return m_dirs;
}

} // namespace QmlDesigner
