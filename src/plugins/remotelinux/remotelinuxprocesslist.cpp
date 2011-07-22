/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
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
} // anonymous namespace

class AbstractRemoteLinuxProcessListPrivate
{
public:
    AbstractRemoteLinuxProcessListPrivate(const LinuxDeviceConfiguration::ConstPtr &devConf)
        : deviceConfiguration(devConf),
          process(SshRemoteProcessRunner::create(devConf->sshParameters())),
          state(Inactive)
    {
    }

    const LinuxDeviceConfiguration::ConstPtr deviceConfiguration;
    const SshRemoteProcessRunner::Ptr process;
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
    : QAbstractTableModel(parent), m_d(new AbstractRemoteLinuxProcessListPrivate(devConfig))
{
}

LinuxDeviceConfiguration::ConstPtr AbstractRemoteLinuxProcessList::deviceConfiguration() const
{
    return m_d->deviceConfiguration;
}

AbstractRemoteLinuxProcessList::~AbstractRemoteLinuxProcessList()
{
    delete m_d;
}

void AbstractRemoteLinuxProcessList::update()
{
    QTC_ASSERT(m_d->state == Inactive, return);

    beginResetModel();
    m_d->remoteProcesses.clear();
    m_d->state = Listing;
    startProcess(listProcessesCommandLine());
}

void AbstractRemoteLinuxProcessList::killProcess(int row)
{
    QTC_ASSERT(row >= 0 && row < m_d->remoteProcesses.count(), return);
    QTC_ASSERT(m_d->state == Inactive, return);

    m_d->state = Killing;
    startProcess(killProcessCommandLine(m_d->remoteProcesses.at(row)));
}

int AbstractRemoteLinuxProcessList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_d->remoteProcesses.count();
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
    const RemoteProcess &proc = m_d->remoteProcesses.at(index.row());
    if (index.column() == 0)
        return proc.pid;
    else
        return proc.cmdLine;
}

void AbstractRemoteLinuxProcessList::handleRemoteStdOut(const QByteArray &output)
{
    if (m_d->state == Listing)
        m_d->remoteStdout += output;
}

void AbstractRemoteLinuxProcessList::handleRemoteStdErr(const QByteArray &output)
{
    if (m_d->state != Inactive)
        m_d->remoteStderr += output;
}

void AbstractRemoteLinuxProcessList::handleConnectionError()
{
    QTC_ASSERT(m_d->state != Inactive, return);

    emit error(tr("Connection failure: %1").arg(m_d->process->connection()->errorString()));
    beginResetModel();
    m_d->remoteProcesses.clear();
    endResetModel();
    setFinished();
}

void AbstractRemoteLinuxProcessList::handleRemoteProcessFinished(int exitStatus)
{
    QTC_ASSERT(m_d->state != Inactive, return);

    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        m_d->errorMsg = tr("Error: Remote process failed to start: %1")
            .arg(m_d->process->process()->errorString());
        break;
    case SshRemoteProcess::KilledBySignal:
        m_d->errorMsg = tr("Error: Remote process crashed: %1")
            .arg(m_d->process->process()->errorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        if (m_d->process->process()->exitCode() == 0) {
            if (m_d->state == Listing)
                m_d->remoteProcesses = buildProcessList(QString::fromUtf8(m_d->remoteStdout));
        } else {
            m_d->errorMsg = tr("Remote process failed.");
        }
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (!m_d->errorMsg.isEmpty()) {
        if (!m_d->remoteStderr.isEmpty())
            m_d->errorMsg += tr("\nRemote stderr was: %1").arg(QString::fromUtf8(m_d->remoteStderr));
        emit error(m_d->errorMsg);
    } else if (m_d->state == Killing) {
        emit processKilled();
    }

    if (m_d->state == Listing)
        endResetModel();

    setFinished();
}

void AbstractRemoteLinuxProcessList::startProcess(const QString &cmdLine)
{
    connect(m_d->process.data(), SIGNAL(connectionError(Utils::SshError)),
        SLOT(handleConnectionError()));
    connect(m_d->process.data(), SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdOut(QByteArray)));
    connect(m_d->process.data(), SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdErr(QByteArray)));
    connect(m_d->process.data(), SIGNAL(processClosed(int)),
        SLOT(handleRemoteProcessFinished(int)));
    m_d->remoteStdout.clear();
    m_d->remoteStderr.clear();
    m_d->errorMsg.clear();
    m_d->process->run(cmdLine.toUtf8());
}

void AbstractRemoteLinuxProcessList::setFinished()
{
    disconnect(m_d->process.data(), 0, this, 0);
    m_d->state = Inactive;
}


GenericRemoteLinuxProcessList::GenericRemoteLinuxProcessList(const LinuxDeviceConfiguration::ConstPtr &devConfig,
        QObject *parent)
    : AbstractRemoteLinuxProcessList(devConfig, parent)
{
}

QString GenericRemoteLinuxProcessList::listProcessesCommandLine() const
{
    return QLatin1String("ps -eo pid,args");
}

QString GenericRemoteLinuxProcessList::killProcessCommandLine(const RemoteProcess &process) const
{
    return QLatin1String("kill -9 ") + QString::number(process.pid);
}

QList<AbstractRemoteLinuxProcessList::RemoteProcess> GenericRemoteLinuxProcessList::buildProcessList(const QString &listProcessesReply) const
{
    QList<RemoteProcess> processes;
    QStringList lines = listProcessesReply.split(QLatin1Char('\n'));
    lines.removeFirst(); // column headers
    foreach (const QString &line, lines) {
        const QString &trimmedLine = line.trimmed();
        const int pidEndPos = trimmedLine.indexOf(QLatin1Char(' '));
        if (pidEndPos == -1)
            continue;
        bool isNumber;
        const int pid = trimmedLine.left(pidEndPos).toInt(&isNumber);
        if (!isNumber) {
            qDebug("%s: Non-integer value where pid was expected. Line was: '%s'",
                Q_FUNC_INFO, qPrintable(trimmedLine));
            continue;
        }
        processes << RemoteProcess(pid, trimmedLine.mid(pidEndPos));
    }

    return processes;
}

} // namespace RemoteLinux
