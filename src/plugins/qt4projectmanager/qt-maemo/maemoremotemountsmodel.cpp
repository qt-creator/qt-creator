/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "maemoremotemountsmodel.h"

#include "maemoconstants.h"

namespace Qt4ProjectManager {
namespace Internal {

namespace {
const QLatin1String InvalidMountPoint("/");
} // anonymous namespace

MaemoRemoteMountsModel::MountSpecification::MountSpecification(const QString &l,
    const QString &r, int p) : localDir(l), remoteMountPoint(r), remotePort(p) {}

bool MaemoRemoteMountsModel::MountSpecification::isValid() const
{
    return remoteMountPoint != InvalidMountPoint;
}


MaemoRemoteMountsModel::MaemoRemoteMountsModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

void MaemoRemoteMountsModel::addMountSpecification(const QString &localDir)
{
    int port = 10100;
    int i = 0;
    while (i < rowCount()) {
        if (mountSpecificationAt(i).remotePort == port) {
            ++port;
            i = 0;
        } else {
            ++i;
        }
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_mountSpecs << MountSpecification(localDir, InvalidMountPoint, port);
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
    foreach (const MountSpecification &m, m_mountSpecs) {
        if (m.isValid())
            ++count;
    }
    return count;
}

bool MaemoRemoteMountsModel::hasValidMountSpecifications() const
{
    foreach (const MountSpecification &m, m_mountSpecs) {
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
    QVariantList mountPortsList;
    foreach (const MountSpecification &mountSpec, m_mountSpecs) {
        localDirsList << mountSpec.localDir;
        remoteMountPointsList << mountSpec.remoteMountPoint;
        mountPortsList << mountSpec.remotePort;
    }
    map.insert(ExportedLocalDirsKey, localDirsList);
    map.insert(RemoteMountPointsKey, remoteMountPointsList);
    map.insert(MountPortsKey, mountPortsList);
    return map;
}

void MaemoRemoteMountsModel::fromMap(const QVariantMap &map)
{
    const QVariantList &localDirsList
        = map.value(ExportedLocalDirsKey).toList();
    const QVariantList &remoteMountPointsList
        = map.value(RemoteMountPointsKey).toList();
    const QVariantList &mountPortsList = map.value(MountPortsKey).toList();
    const int count = qMin(qMin(localDirsList.count(),
        remoteMountPointsList.count()), mountPortsList.count());
    for (int i = 0; i < count; ++i) {
        const QString &localDir = localDirsList.at(i).toString();
        const QString &remoteMountPoint
            = remoteMountPointsList.at(i).toString();
        const int port = mountPortsList.at(i).toInt();
        m_mountSpecs << MountSpecification(localDir, remoteMountPoint, port);
    }
}

Qt::ItemFlags MaemoRemoteMountsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags ourFlags = QAbstractTableModel::flags(index);
    if (index.column() == RemoteMountPointRow || index.column() == PortRow)
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
    case PortRow: return tr("Remote port");
    default: return QVariant();
    }
}

QVariant MaemoRemoteMountsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const MountSpecification &mountSpec = mountSpecificationAt(index.row());
    switch (index.column()) {
    case LocalDirRow:
        if (role == Qt::DisplayRole)
            return mountSpec.localDir;
        break;
    case RemoteMountPointRow:
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return mountSpec.remoteMountPoint;
        break;
    case PortRow:
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return mountSpec.remotePort;
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
            const MountSpecification &mountSpec = m_mountSpecs.at(i);
            if (i != index.row() && mountSpec.isValid()
                && mountSpec.remoteMountPoint == newRemoteMountPoint)
                return false;
        }
        m_mountSpecs[index.row()].remoteMountPoint = newRemoteMountPoint;
        set = true;
        break;
    }
    case PortRow: {
        const int newPort = value.toInt();
        for (int i = 0; i < m_mountSpecs.count(); ++i) {
            if (i != index.row() && m_mountSpecs.at(i).remotePort == newPort)
                return false;
        }
        m_mountSpecs[index.row()].remotePort = newPort;
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
} // namespace Qt4ProjectManager
