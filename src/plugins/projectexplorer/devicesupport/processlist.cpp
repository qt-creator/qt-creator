// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processlist.h"

#include "idevice.h"
#include "../projectexplorertr.h"

#include <utils/async.h>
#include <utils/processinfo.h>
#include <utils/qtcassert.h>
#include <utils/result.h>
#include <utils/treemodel.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QTimer>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

using namespace QtTaskTree;
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
    {
        // FIXME: This should not be used for non-desktop cases.
#if defined(Q_OS_UNIX)
        ownPid = getpid();
#elif defined(Q_OS_WIN)
        ownPid = GetCurrentProcessId();
#endif
    }

    qint64 ownPid = -1;
    const IDevice::ConstPtr device;
    State state = Inactive;
    QSingleTaskTreeRunner m_taskTree;
    TreeModel<TypedTreeItem<DeviceProcessTreeItem>, DeviceProcessTreeItem> model;
    QSingleTaskTreeRunner m_signalOperationRunner;
};

} // namespace Internal

using namespace Internal;

ProcessList::ProcessList(const IDevice::ConstPtr &device, QObject *parent)
    : QObject(parent), d(std::make_unique<DeviceProcessListPrivate>(device))
{
    d->model.setHeader({Tr::tr("Process ID"), Tr::tr("Command Line")});
}

ProcessList::~ProcessList() = default;

void ProcessList::update()
{
    QTC_ASSERT(d->state == Inactive, return);
    QTC_ASSERT(d->device, return);

    d->model.clear();
    d->model.rootItem()->appendChild(new DeviceProcessTreeItem(
        {0, {}, Tr::tr("Fetching process list. This might take a while.")}, Qt::NoItemFlags));
    d->state = Listing;

    using ProcessListResult = Result<QList<ProcessInfo>>;

    auto setupListFetcher = [this](Async<ProcessListResult> &async) {
        async.setConcurrentCallData(&ProcessInfo::processInfoList, d->device->rootPath());
    };

    auto listFetchDone = [this](const Async<ProcessListResult> &async) {
        const ProcessListResult result = async.result();

        setFinished();
        d->model.clear();

        if (result) {
            for (const ProcessInfo &process : *result) {
                Qt::ItemFlags fl;
                if (process.processId != d->ownPid)
                    fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
                d->model.rootItem()->appendChild(new DeviceProcessTreeItem(process, fl));
            }
        } else {
            d->model.rootItem()->appendChild(new DeviceProcessTreeItem(
                {0, {}, Tr::tr("Failed to fetch process list.")}, Qt::NoItemFlags));

            QStringList errors = result.error().split('\n');
            for (const QString &error : errors) {
                d->model.rootItem()->appendChild(
                    new DeviceProcessTreeItem({1, {}, error}, Qt::NoItemFlags));
            }
        }

        emit processListUpdated();
    };

    d->m_taskTree.start({AsyncTask<ProcessListResult>(setupListFetcher, listFetchDone)});
}

void ProcessList::killProcess(int row)
{
    QTC_ASSERT(row >= 0 && row < d->model.rootItem()->childCount(), return);
    QTC_ASSERT(d->state == Inactive, return);
    QTC_ASSERT(d->device, return);

    d->state = Killing;

    const SignalOperationData data{.mode = SignalOperationMode::KillByPid,
                                   .pid = at(row).processId};
    const Storage<Utils::Result<>> resultStorage;

    const auto onDone = [this, resultStorage] {
        if (*resultStorage) {
            QTC_CHECK(d->state == Killing);
            setFinished();
            emit processKilled();
        } else {
            QTC_CHECK(d->state != Inactive);
            setFinished();
            emit error(resultStorage->error());
        }
    };

    const Group recipe {
        resultStorage,
        d->device->signalOperationRecipe(data, resultStorage),
        onGroupDone(onDone)
    };

    d->m_signalOperationRunner.start(recipe);
}

ProcessInfo ProcessList::at(int row) const
{
    return d->model.rootItem()->childAt(row)->process;
}

QAbstractItemModel *ProcessList::model() const
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
    return {};
}

void ProcessList::setFinished()
{
    d->state = Inactive;
}

} // ProjectExplorer
