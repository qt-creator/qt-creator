/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "deviceprocesslist.h"

#include <utils/qtcassert.h>
#include <ssh/sshremoteprocessrunner.h>

using namespace QSsh;

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
    SshRemoteProcessRunner process;
    QList<DeviceProcess> remoteProcesses;
    QString errorMsg;
    State state;
};

} // namespace Internal

using namespace Internal;

DeviceProcessList::DeviceProcessList(const IDevice::ConstPtr &device, QObject *parent)
    : QAbstractTableModel(parent), d(new DeviceProcessListPrivate(device))
{
}

DeviceProcessList::~DeviceProcessList()
{
    delete d;
}

void DeviceProcessList::update()
{
    QTC_ASSERT(d->state == Inactive, return);

    if (!d->remoteProcesses.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, d->remoteProcesses.count() - 1);
        d->remoteProcesses.clear();
        endRemoveRows();
    }
    d->state = Listing;
    startProcess(d->device->listProcessesCommandLine());
}

void DeviceProcessList::killProcess(int row)
{
    QTC_ASSERT(row >= 0 && row < d->remoteProcesses.count(), return);
    QTC_ASSERT(d->state == Inactive, return);

    d->state = Killing;
    startProcess(d->device->killProcessCommandLine(d->remoteProcesses.at(row)));
}

DeviceProcess DeviceProcessList::at(int row) const
{
    return d->remoteProcesses.at(row);
}

IDevice::ConstPtr DeviceProcessList::device() const
{
    return d->device;
}

int DeviceProcessList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->remoteProcesses.count();
}

int DeviceProcessList::columnCount(const QModelIndex &) const { return 2; }

QVariant DeviceProcessList::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0
            || section >= columnCount())
        return QVariant();
    return section == 0? tr("PID") : tr("Command Line");
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

void DeviceProcessList::handleConnectionError()
{
    QTC_ASSERT(d->state != Inactive, return);

    emit error(tr("Connection failure: %1").arg(d->process.lastConnectionErrorString()));
    beginResetModel();
    d->remoteProcesses.clear();
    endResetModel();
    setFinished();
}

void DeviceProcessList::handleRemoteProcessFinished(int exitStatus)
{
    QTC_ASSERT(d->state != Inactive, return);

    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        d->errorMsg = tr("Error: Remote process failed to start: %1")
            .arg(d->process.processErrorString());
        break;
    case SshRemoteProcess::CrashExit:
        d->errorMsg = tr("Error: Remote process crashed: %1")
            .arg(d->process.processErrorString());
        break;
    case SshRemoteProcess::NormalExit:
        if (d->process.processExitCode() == 0) {
            if (d->state == Listing) {
                const QByteArray remoteStdout = d->process.readAllStandardOutput();
                QList<DeviceProcess> processes = d->device->buildProcessList(QString::fromUtf8(remoteStdout.data(),
                    remoteStdout.count()));
                if (!processes.isEmpty()) {
                    beginInsertRows(QModelIndex(), 0, processes.count()-1);
                    d->remoteProcesses = processes;
                    endInsertRows();
                }
            }
        } else {
            d->errorMsg = tr("Remote process failed.");
        }
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (d->state == Listing)
        emit processListUpdated();

    if (!d->errorMsg.isEmpty()) {
        const QByteArray remoteStderr = d->process.readAllStandardError();
        if (!remoteStderr.isEmpty())
            d->errorMsg += tr("\nRemote stderr was: %1").arg(QString::fromUtf8(remoteStderr));
        emit error(d->errorMsg);
    } else if (d->state == Killing) {
        emit processKilled();
    }

    setFinished();
}

void DeviceProcessList::startProcess(const QString &cmdLine)
{
    connect(&d->process, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(&d->process, SIGNAL(processClosed(int)),
        SLOT(handleRemoteProcessFinished(int)));
    d->errorMsg.clear();
    d->process.run(cmdLine.toUtf8(), d->device->sshParameters());
}

void DeviceProcessList::setFinished()
{
    disconnect(&d->process, 0, this, 0);
    d->state = Inactive;
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
