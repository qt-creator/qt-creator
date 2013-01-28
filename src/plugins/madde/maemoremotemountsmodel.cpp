/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "maemoremotemountsmodel.h"

#include "maemoconstants.h"

namespace Madde {
namespace Internal {

MaemoRemoteMountsModel::MaemoRemoteMountsModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

void MaemoRemoteMountsModel::addMountSpecification(const QString &localDir)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_mountSpecs << MaemoMountSpecification(localDir,
        MaemoMountSpecification::InvalidMountPoint);
    endInsertRows();
}

void MaemoRemoteMountsModel::removeMountSpecificationAt(int pos)
{
    Q_ASSERT(pos >= 0 && pos < rowCount());
    beginRemoveRows(QModelIndex(), pos, pos);
    m_mountSpecs.removeAt(pos);
    endRemoveRows();
}

void MaemoRemoteMountsModel::setLocalDir(int pos, const QString &localDir)
{
    Q_ASSERT(pos >= 0 && pos < rowCount());
    m_mountSpecs[pos].localDir = localDir;
    const QModelIndex currentIndex = index(pos, LocalDirRow);
    emit dataChanged(currentIndex, currentIndex);
}

int MaemoRemoteMountsModel::validMountSpecificationCount() const
{
    int count = 0;
    foreach (const MaemoMountSpecification &m, m_mountSpecs) {
        if (m.isValid())
            ++count;
    }
    return count;
}

bool MaemoRemoteMountsModel::hasValidMountSpecifications() const
{
    foreach (const MaemoMountSpecification &m, m_mountSpecs) {
        if (m.isValid())
            return true;
    }
    return false;
}

QVariantMap MaemoRemoteMountsModel::toMap() const
{
    QVariantMap map;
    QVariantList localDirsList;
    QVariantList remoteMountPointsList;
    foreach (const MaemoMountSpecification &mountSpec, m_mountSpecs) {
        localDirsList << mountSpec.localDir;
        remoteMountPointsList << mountSpec.remoteMountPoint;
    }
    map.insert(ExportedLocalDirsKey, localDirsList);
    map.insert(RemoteMountPointsKey, remoteMountPointsList);
    return map;
}

void MaemoRemoteMountsModel::fromMap(const QVariantMap &map)
{
    const QVariantList &localDirsList
        = map.value(ExportedLocalDirsKey).toList();
    const QVariantList &remoteMountPointsList
        = map.value(RemoteMountPointsKey).toList();
    const int count
        = qMin(localDirsList.count(), remoteMountPointsList.count());
    for (int i = 0; i < count; ++i) {
        const QString &localDir = localDirsList.at(i).toString();
        const QString &remoteMountPoint
            = remoteMountPointsList.at(i).toString();
        m_mountSpecs << MaemoMountSpecification(localDir, remoteMountPoint);
    }
}

Qt::ItemFlags MaemoRemoteMountsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags ourFlags = QAbstractTableModel::flags(index);
    if (index.column() == RemoteMountPointRow)
        ourFlags |= Qt::ItemIsEditable;
    return ourFlags;
}

QVariant MaemoRemoteMountsModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case LocalDirRow: return tr("Local directory");
    case RemoteMountPointRow: return tr("Remote mount point");
    default: return QVariant();
    }
}

QVariant MaemoRemoteMountsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const MaemoMountSpecification &mountSpec = mountSpecificationAt(index.row());
    switch (index.column()) {
    case LocalDirRow:
        if (role == Qt::DisplayRole)
            return mountSpec.localDir;
        break;
    case RemoteMountPointRow:
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return mountSpec.remoteMountPoint;
        break;
    }
    return QVariant();
}

bool MaemoRemoteMountsModel::setData(const QModelIndex &index,
    const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::EditRole)
        return false;

    bool set;
    switch (index.column()) {
    case RemoteMountPointRow: {
        const QString &newRemoteMountPoint = value.toString();
        for (int i = 0; i < m_mountSpecs.count(); ++i) {
            const MaemoMountSpecification &mountSpec = m_mountSpecs.at(i);
            if (i != index.row() && mountSpec.isValid()
                && mountSpec.remoteMountPoint == newRemoteMountPoint)
                return false;
        }
        m_mountSpecs[index.row()].remoteMountPoint = newRemoteMountPoint;
        set = true;
        break;
    }
    case LocalDirRow:
    default:
        set = false;
        break;
    }

    if (set)
        emit dataChanged(index, index);
    return set;
}

} // namespace Internal
} // namespace Madde
