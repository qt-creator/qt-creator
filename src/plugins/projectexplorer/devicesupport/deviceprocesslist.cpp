/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "deviceprocesslist.h"
#include "localprocesslist.h"

#include <utils/qtcassert.h>
#include <utils/treemodel.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

enum State { Inactive, Listing, Killing };

class DeviceProcessTreeItem : public TreeItem
{
public:
    DeviceProcessTreeItem(const DeviceProcessItem &p, Qt::ItemFlags f) : process(p), fl(f) {}

    QVariant data(int column, int role) const final;
    Qt::ItemFlags flags(int) const final { return fl; }

    DeviceProcessItem process;
    Qt::ItemFlags fl;
};

class DeviceProcessListPrivate
{
public:
    DeviceProcessListPrivate(const IDevice::ConstPtr &device)
        : device(device)
    { }

    qint64 ownPid = -1;
    const IDevice::ConstPtr device;
    State state = Inactive;
    TreeModel<TypedTreeItem<DeviceProcessTreeItem>, DeviceProcessTreeItem> model;
};

} // namespace Internal

using namespace Internal;

DeviceProcessList::DeviceProcessList(const IDevice::ConstPtr &device, QObject *parent)
    : QObject(parent), d(std::make_unique<DeviceProcessListPrivate>(device))
{
    d->model.setHeader({tr("Process ID"), tr("Command Line")});
}

DeviceProcessList::~DeviceProcessList() = default;

void DeviceProcessList::update()
{
    QTC_ASSERT(d->state == Inactive, return);
    QTC_ASSERT(device(), return);

    d->model.clear();
    d->state = Listing;
    doUpdate();
}

void DeviceProcessList::reportProcessListUpdated(const QList<DeviceProcessItem> &processes)
{
    QTC_ASSERT(d->state == Listing, return);
    setFinished();
    for (const DeviceProcessItem &process : processes) {
        Qt::ItemFlags fl;
        if (process.pid != d->ownPid)
            fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        d->model.rootItem()->appendChild(new DeviceProcessTreeItem(process, fl));
    }

    emit processListUpdated();
}

void DeviceProcessList::killProcess(int row)
{
    QTC_ASSERT(row >= 0 && row < d->model.rootItem()->childCount(), return);
    QTC_ASSERT(d->state == Inactive, return);
    QTC_ASSERT(device(), return);

    d->state = Killing;
    doKillProcess(at(row));
}

void DeviceProcessList::setOwnPid(qint64 pid)
{
    d->ownPid = pid;
}

void DeviceProcessList::reportProcessKilled()
{
    QTC_ASSERT(d->state == Killing, return);
    setFinished();
    emit processKilled();
}

DeviceProcessItem DeviceProcessList::at(int row) const
{
    return d->model.rootItem()->childAt(row)->process;
}

QAbstractItemModel *DeviceProcessList::model() const
{
    return &d->model;
}

QVariant DeviceProcessTreeItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        if (column == 0)
            return process.pid;
        else
            return process.cmdLine;
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

QList<DeviceProcessItem> DeviceProcessList::localProcesses()
{
    return LocalProcessList::getLocalProcesses();
}

bool DeviceProcessItem::operator <(const DeviceProcessItem &other) const
{
    if (pid != other.pid)
        return pid < other.pid;
    if (exe != other.exe)
        return exe < other.exe;
    return cmdLine < other.cmdLine;
}

} // namespace ProjectExplorer
