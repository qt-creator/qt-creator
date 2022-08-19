// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "assetslibrarydirsmodel.h"
#include "assetslibrarymodel.h"

#include <QDebug>
#include <QMetaProperty>

namespace QmlDesigner {

AssetsLibraryDirsModel::AssetsLibraryDirsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // add roles
    const QMetaObject meta = AssetsLibraryDir::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(i, meta.property(i).name());
}

QVariant AssetsLibraryDirsModel::data(const QModelIndex &index, int role) const
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

bool AssetsLibraryDirsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // currently only dirExpanded property is updatable
    if (index.isValid() && m_roleNames.contains(role)) {
        QVariant currValue = m_dirs.at(index.row())->property(m_roleNames.value(role));
        if (currValue != value) {
            m_dirs.at(index.row())->setProperty(m_roleNames.value(role), value);
            if (m_roleNames.value(role) == "dirExpanded")
                AssetsLibraryModel::saveExpandedState(value.toBool(), m_dirs.at(index.row())->dirPath());
            emit dataChanged(index, index, {role});
            return true;
        }
    }
    return false;
}

int AssetsLibraryDirsModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_dirs.size();
}

QHash<int, QByteArray> AssetsLibraryDirsModel::roleNames() const
{
    return m_roleNames;
}

void AssetsLibraryDirsModel::addDir(AssetsLibraryDir *assetsDir)
{
    m_dirs.append(assetsDir);
}

const QList<AssetsLibraryDir *> AssetsLibraryDirsModel::assetsDirs() const
{
    return m_dirs;
}

} // namespace QmlDesigner
