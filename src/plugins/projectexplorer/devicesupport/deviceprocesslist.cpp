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

#include "deviceprocesslist.h"
#include "localprocesslist.h"

#include <utils/qtcassert.h>

namespace ProjectExplorer {
namespace Internal {

enum State { Inactive, Listing, Killing };

class DeviceProcessListPrivate
{
public:
    DeviceProcessListPrivate(const IDevice::ConstPtr &device)
        : device(device),
          state(Inactive)
    { }

    const IDevice::ConstPtr device;
    QList<DeviceProcess> remoteProcesses;
    State state;
};

} // namespace Internal

using namespace Internal;

DeviceProcessList::DeviceProcessList(const IDevice::ConstPtr &device, QObject *parent)
    : QAbstractItemModel(parent), d(new DeviceProcessListPrivate(device))
{
}

DeviceProcessList::~DeviceProcessList()
{
    delete d;
}

QModelIndex DeviceProcessList::parent(const QModelIndex &) const
{
    return QModelIndex();
}

bool DeviceProcessList::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rowCount(parent) > 0 && columnCount(parent) > 0;
    return false;
}

QModelIndex DeviceProcessList::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}


void DeviceProcessList::update()
{
    QTC_ASSERT(d->state == Inactive, return);
    QTC_ASSERT(device(), return);

    if (!d->remoteProcesses.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, d->remoteProcesses.count() - 1);
        d->remoteProcesses.clear();
        endRemoveRows();
    }
    d->state = Listing;
    doUpdate();
}

void DeviceProcessList::reportProcessListUpdated(const QList<DeviceProcess> &processes)
{
    QTC_ASSERT(d->state == Listing, return);
    setFinished();
    if (!processes.isEmpty()) {
        beginInsertRows(QModelIndex(), 0, processes.count() - 1);
        d->remoteProcesses = processes;
        endInsertRows();
    }
    emit processListUpdated();
}

void DeviceProcessList::killProcess(int row)
{
    QTC_ASSERT(row >= 0 && row < d->remoteProcesses.count(), return);
    QTC_ASSERT(d->state == Inactive, return);
    QTC_ASSERT(device(), return);

    d->state = Killing;
    doKillProcess(d->remoteProcesses.at(row));
}

void DeviceProcessList::reportProcessKilled()
{
    QTC_ASSERT(d->state == Killing, return);
    setFinished();
    emit processKilled();
}

DeviceProcess DeviceProcessList::at(int row) const
{
    return d->remoteProcesses.at(row);
}

int DeviceProcessList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->remoteProcesses.count();
}

int DeviceProcessList::columnCount(const QModelIndex &) const
{
    return 2;
}

QVariant DeviceProcessList::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0
            || section >= columnCount())
        return QVariant();
    return section == 0? tr("Process ID") : tr("Command Line");
}

QVariant DeviceProcessList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(index.parent())
            || index.column() >= columnCount())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        const DeviceProcess &proc = d->remoteProcesses.at(index.row());
        if (index.column() == 0)
            return proc.pid;
        else
            return proc.cmdLine;
    }
    return QVariant();
}

void DeviceProcessList::setFinished()
{
    d->state = Inactive;
}

IDevice::ConstPtr DeviceProcessList::device() const
{
    return d->device;
}

void DeviceProcessList::reportError(const QString &message)
{
    QTC_ASSERT(d->state != Inactive, return);
    setFinished();
    emit error(message);
}

QList<DeviceProcess> DeviceProcessList::localProcesses()
{
    return LocalProcessList::getLocalProcesses();
}

bool DeviceProcess::operator <(const DeviceProcess &other) const
{
    if (pid != other.pid)
        return pid < other.pid;
    if (exe != other.exe)
        return exe < other.exe;
    return cmdLine < other.cmdLine;
}

} // namespace ProjectExplorer
