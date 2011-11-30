/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "remotelinuxprocesslist.h"

#include "linuxdeviceconfiguration.h"

#include <utils/qtcassert.h>
#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtCore/QByteArray>
#include <QtCore/QString>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, Listing, Killing };
const char Delimiter[] = "x-----";
} // anonymous namespace

class AbstractRemoteLinuxProcessListPrivate
{
public:
    AbstractRemoteLinuxProcessListPrivate(const LinuxDeviceConfiguration::ConstPtr &devConf)
        : deviceConfiguration(devConf),
          state(Inactive)
    {
    }


    const LinuxDeviceConfiguration::ConstPtr deviceConfiguration;
    SshRemoteProcessRunner process;
    QList<AbstractRemoteLinuxProcessList::RemoteProcess> remoteProcesses;
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    QString errorMsg;
    State state;
};

} // namespace Internal

using namespace Internal;

AbstractRemoteLinuxProcessList::AbstractRemoteLinuxProcessList(const LinuxDeviceConfiguration::ConstPtr &devConfig,
        QObject *parent)
    : QAbstractTableModel(parent), d(new AbstractRemoteLinuxProcessListPrivate(devConfig))
{
}

LinuxDeviceConfiguration::ConstPtr AbstractRemoteLinuxProcessList::deviceConfiguration() const
{
    return d->deviceConfiguration;
}

AbstractRemoteLinuxProcessList::~AbstractRemoteLinuxProcessList()
{
    delete d;
}

void AbstractRemoteLinuxProcessList::update()
{
    QTC_ASSERT(d->state == Inactive, return);

    beginResetModel();
    d->remoteProcesses.clear();
    d->state = Listing;
    startProcess(listProcessesCommandLine());
}

void AbstractRemoteLinuxProcessList::killProcess(int row)
{
    QTC_ASSERT(row >= 0 && row < d->remoteProcesses.count(), return);
    QTC_ASSERT(d->state == Inactive, return);

    d->state = Killing;
    startProcess(killProcessCommandLine(d->remoteProcesses.at(row)));
}

int AbstractRemoteLinuxProcessList::pidAt(int row) const
{
    return d->remoteProcesses.at(row).pid;
}

int AbstractRemoteLinuxProcessList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->remoteProcesses.count();
}

int AbstractRemoteLinuxProcessList::columnCount(const QModelIndex &) const { return 2; }

QVariant AbstractRemoteLinuxProcessList::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0
            || section >= columnCount())
        return QVariant();
    return section == 0? tr("PID") : tr("Command Line");
}

QVariant AbstractRemoteLinuxProcessList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(index.parent())
            || index.column() >= columnCount() || role != Qt::DisplayRole)
        return QVariant();
    const RemoteProcess &proc = d->remoteProcesses.at(index.row());
    if (index.column() == 0)
        return proc.pid;
    else
        return proc.cmdLine;
}

void AbstractRemoteLinuxProcessList::handleRemoteStdOut(const QByteArray &output)
{
    if (d->state == Listing)
        d->remoteStdout += output;
}

void AbstractRemoteLinuxProcessList::handleRemoteStdErr(const QByteArray &output)
{
    if (d->state != Inactive)
        d->remoteStderr += output;
}

void AbstractRemoteLinuxProcessList::handleConnectionError()
{
    QTC_ASSERT(d->state != Inactive, return);

    emit error(tr("Connection failure: %1").arg(d->process.lastConnectionErrorString()));
    beginResetModel();
    d->remoteProcesses.clear();
    endResetModel();
    setFinished();
}

void AbstractRemoteLinuxProcessList::handleRemoteProcessFinished(int exitStatus)
{
    QTC_ASSERT(d->state != Inactive, return);

    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        d->errorMsg = tr("Error: Remote process failed to start: %1")
            .arg(d->process.processErrorString());
        break;
    case SshRemoteProcess::KilledBySignal:
        d->errorMsg = tr("Error: Remote process crashed: %1")
            .arg(d->process.processErrorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        if (d->process.processExitCode() == 0) {
            if (d->state == Listing) {
                d->remoteProcesses = buildProcessList(QString::fromUtf8(d->remoteStdout.data(),
                    d->remoteStdout.count()));
            }
        } else {
            d->errorMsg = tr("Remote process failed.");
        }
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (!d->errorMsg.isEmpty()) {
        if (!d->remoteStderr.isEmpty())
            d->errorMsg += tr("\nRemote stderr was: %1").arg(QString::fromUtf8(d->remoteStderr));
        emit error(d->errorMsg);
    } else if (d->state == Killing) {
        emit processKilled();
    }

    if (d->state == Listing)
        endResetModel();

    setFinished();
}

void AbstractRemoteLinuxProcessList::startProcess(const QString &cmdLine)
{
    connect(&d->process, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(&d->process, SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdOut(QByteArray)));
    connect(&d->process, SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdErr(QByteArray)));
    connect(&d->process, SIGNAL(processClosed(int)),
        SLOT(handleRemoteProcessFinished(int)));
    d->remoteStdout.clear();
    d->remoteStderr.clear();
    d->errorMsg.clear();
    d->process.run(cmdLine.toUtf8(), d->deviceConfiguration->sshParameters());
}

void AbstractRemoteLinuxProcessList::setFinished()
{
    disconnect(&d->process, 0, this, 0);
    d->state = Inactive;
}


GenericRemoteLinuxProcessList::GenericRemoteLinuxProcessList(const LinuxDeviceConfiguration::ConstPtr &devConfig,
        QObject *parent)
    : AbstractRemoteLinuxProcessList(devConfig, parent)
{
}

QString GenericRemoteLinuxProcessList::listProcessesCommandLine() const
{
    return QString::fromLocal8Bit("for dir in `ls -d /proc/[0123456789]*`; "
        "do printf \"${dir}%1\";cat $dir/cmdline; printf '%1';cat $dir/stat;printf '\\n'; done")
        .arg(Delimiter);
}

QString GenericRemoteLinuxProcessList::killProcessCommandLine(const RemoteProcess &process) const
{
    return QLatin1String("kill -9 ") + QString::number(process.pid);
}

QList<AbstractRemoteLinuxProcessList::RemoteProcess> GenericRemoteLinuxProcessList::buildProcessList(const QString &listProcessesReply) const
{
    QList<RemoteProcess> processes;
    const QStringList &lines = listProcessesReply.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    foreach (const QString &line, lines) {
        const QStringList elements = line.split(QString::fromLocal8Bit(Delimiter));
        if (elements.count() < 3) {
            qDebug("%s: Expected three list elements, got %d.", Q_FUNC_INFO, elements.count());
            continue;
        }
        bool ok;
        const int pid = elements.first().mid(6).toInt(&ok);
        if (!ok) {
            qDebug("%s: Expected number in %s.", Q_FUNC_INFO, qPrintable(elements.first()));
            continue;
        }
        QString command = elements.at(1);
        command.replace(QLatin1Char('\0'), QLatin1Char(' '));
        if (command.isEmpty()) {
            const QString &statString = elements.at(2);
            const int openParenPos = statString.indexOf(QLatin1Char('('));
            const int closedParenPos = statString.indexOf(QLatin1Char(')'), openParenPos);
            if (openParenPos == -1 || closedParenPos == -1)
                continue;
            command = QLatin1Char('[')
                + statString.mid(openParenPos + 1, closedParenPos - openParenPos - 1)
                + QLatin1Char(']');
        }

        int insertPos;
        for (insertPos = 0; insertPos < processes.count(); ++insertPos) {
            if (pid < processes.at(insertPos).pid)
                break;
        }
        processes.insert(insertPos, RemoteProcess(pid, command));
    }

    return processes;
}

} // namespace RemoteLinux
