/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemoremoteprocesslist.h"

#include "maemodeviceconfigurations.h"

#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtCore/QStringList>

using namespace Utils;

namespace Qt4ProjectManager {
namespace Internal {
namespace {
const QByteArray LineSeparator1("---");
const QByteArray LineSeparator2("QTCENDOFLINE---");
const QByteArray LineSeparator = LineSeparator1 + LineSeparator2;
} // anonymous namespace

MaemoRemoteProcessList::MaemoRemoteProcessList(const MaemoDeviceConfig::ConstPtr &devConfig,
    QObject *parent)
        : QAbstractTableModel(parent),
          m_process(SshRemoteProcessRunner::create(devConfig->sshParameters())),
          m_state(Inactive),
          m_devConfig(devConfig)
{
}

MaemoRemoteProcessList::~MaemoRemoteProcessList() {}

void MaemoRemoteProcessList::update()
{
    if (m_state != Inactive) {
        qDebug("%s: Did not expect state to be %d.", Q_FUNC_INFO, m_state);
        stop();
    }
    beginResetModel();
    m_remoteProcs.clear();
    QByteArray command;

    // The ps command on Fremantle ignores all command line options, so
    // we have to collect the information in /proc manually.
    if (m_devConfig->osVersion() == MaemoGlobal::Maemo5) {
        command = "sep1=" + LineSeparator1 + '\n'
            + "sep2=" + LineSeparator2 + '\n'
            + "pidlist=`ls /proc |grep -E '^[[:digit:]]+$' |sort -n`; "
            +  "for pid in $pidlist\n"
            +  "do\n"
            +  "    echo -n \"$pid \"\n"
            +  "    tr '\\0' ' ' < /proc/$pid/cmdline\n"
            +  "    echo -n \"$sep1$sep2\"\n"
            +  "done\n"
            +  "echo ''";
    } else {
        command = "ps -eo pid,args";
    }

    startProcess(command, Listing);
}

void MaemoRemoteProcessList::killProcess(int row)
{
    Q_ASSERT(row >= 0 && row < m_remoteProcs.count());
    const QByteArray command
        = "kill -9 " + QByteArray::number(m_remoteProcs.at(row).pid);
    startProcess(command, Killing);
}

void MaemoRemoteProcessList::startProcess(const QByteArray &cmdLine,
    State newState)
{
    if (m_state != Inactive) {
        qDebug("%s: Did not expect state to be %d.", Q_FUNC_INFO, m_state);
        stop();
    }
    m_state = newState;
    connect(m_process.data(), SIGNAL(connectionError(Utils::SshError)),
        SLOT(handleConnectionError()));
    connect(m_process.data(), SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdOut(QByteArray)));
    connect(m_process.data(),
        SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdErr(QByteArray)));
    connect(m_process.data(), SIGNAL(processClosed(int)),
        SLOT(handleRemoteProcessFinished(int)));
    m_remoteStdout.clear();
    m_remoteStderr.clear();
    m_errorMsg.clear();
    m_process->run(cmdLine);
}

void MaemoRemoteProcessList::handleConnectionError()
{
    if (m_state == Inactive)
        return;

    emit error(tr("Connection failure: %1")
        .arg(m_process->connection()->errorString()));
    stop();
}

void MaemoRemoteProcessList::handleRemoteStdOut(const QByteArray &output)
{
    if (m_state == Listing)
        m_remoteStdout += output;
}

void MaemoRemoteProcessList::handleRemoteStdErr(const QByteArray &output)
{
    if (m_state != Inactive)
        m_remoteStderr += output;
}

void MaemoRemoteProcessList::handleRemoteProcessFinished(int exitStatus)
{
    if (m_state == Inactive)
        return;

    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        m_errorMsg = tr("Error: Remote process failed to start: %1")
            .arg(m_process->process()->errorString());
        break;
    case SshRemoteProcess::KilledBySignal:
        m_errorMsg = tr("Error: Remote process crashed: %1")
            .arg(m_process->process()->errorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        if (m_process->process()->exitCode() == 0) {
            if (m_state == Listing)
                buildProcessList();
        } else {
            m_errorMsg = tr("Remote process failed.");
        }
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (!m_errorMsg.isEmpty()) {
        if (!m_remoteStderr.isEmpty()) {
            m_errorMsg += tr("\nRemote stderr was: %1")
                .arg(QString::fromUtf8(m_remoteStderr));
        }
        emit error(m_errorMsg);
    }
    stop();
}

void MaemoRemoteProcessList::stop()
{
    if (m_state == Inactive)
        return;

    disconnect(m_process.data(), 0, this, 0);
    if (m_state == Listing)
        endResetModel();
    else if (m_errorMsg.isEmpty())
        emit processKilled();
    m_state = Inactive;
}

void MaemoRemoteProcessList::buildProcessList()
{
    const bool isFremantle = m_devConfig->osVersion() == MaemoGlobal::Maemo5;
    const QString remoteOutput = QString::fromUtf8(m_remoteStdout);
    const QByteArray lineSeparator = isFremantle ? LineSeparator : "\n";
    QStringList lines = remoteOutput.split(QString::fromUtf8(lineSeparator));
    if (!isFremantle)
        lines.removeFirst(); // column headers
    foreach (const QString &line, lines) {
        const QString &trimmedLine = line.trimmed();
        const int pidEndPos = trimmedLine.indexOf(' ');
        if (pidEndPos == -1)
            continue;
        bool isNumber;
        const int pid = trimmedLine.left(pidEndPos).toInt(&isNumber);
        if (!isNumber) {
            qDebug("%s: Non-integer value where pid was expected. Line was: '%s'",
                Q_FUNC_INFO, qPrintable(trimmedLine));
            continue;
        }
        m_remoteProcs << RemoteProc(pid, trimmedLine.mid(pidEndPos));
    }
}

int MaemoRemoteProcessList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_remoteProcs.count();
}

int MaemoRemoteProcessList::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant MaemoRemoteProcessList::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole
            || section < 0 || section >= columnCount())
        return QVariant();
    if (section == 0)
        return tr("PID");
    else
        return tr("Command Line");
}

QVariant MaemoRemoteProcessList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(index.parent())
            || index.column() >= columnCount() || role != Qt::DisplayRole)
        return QVariant();
    const RemoteProc &proc = m_remoteProcs.at(index.row());
    if (index.column() == 0)
        return proc.pid;
    else
        return proc.cmdLine;
}

} // namespace Internal
} // namespace Qt4ProjectManager
