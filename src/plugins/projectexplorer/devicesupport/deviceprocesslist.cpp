// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deviceprocesslist.h"
#include "idevice.h"

#include <utils/processinfo.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

enum State { Inactive, Listing, Killing };

class DeviceProcessTreeItem : public TreeItem
{
public:
    DeviceProcessTreeItem(const ProcessInfo &p, Qt::ItemFlags f) : process(p), fl(f) {}

    QVariant data(int column, int role) const final;
    Qt::ItemFlags flags(int) const final { return fl; }

    ProcessInfo process;
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
    d->model.rootItem()->appendChild(
                new DeviceProcessTreeItem(
                    {0, tr("Fetching process list. This might take a while."), ""},
                    Qt::NoItemFlags));
    d->state = Listing;
    doUpdate();
}

void DeviceProcessList::reportProcessListUpdated(const QList<ProcessInfo> &processes)
{
    QTC_ASSERT(d->state == Listing, return);
    setFinished();
    d->model.clear();
    for (const ProcessInfo &process : processes) {
        Qt::ItemFlags fl;
        if (process.processId != d->ownPid)
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

ProcessInfo DeviceProcessList::at(int row) const
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
            return process.processId ? process.processId : QVariant();
        else
            return process.commandLine;
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

} // namespace ProjectExplorer
