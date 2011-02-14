/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
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
** $QT_END_LICENSE$
**
****************************************************************************/
#include "maemousedportsgatherer.h"

#include "maemoglobal.h"

#include <utils/ssh/sshremoteprocessrunner.h>

using namespace Utils;

namespace Qt4ProjectManager {
namespace Internal {

MaemoUsedPortsGatherer::MaemoUsedPortsGatherer(QObject *parent) :
    QObject(parent), m_running(false)
{
}

MaemoUsedPortsGatherer::~MaemoUsedPortsGatherer() {}

void MaemoUsedPortsGatherer::start(const Utils::SshConnection::Ptr &connection,
    const MaemoPortList &portList)
{
    if (m_running)
        qWarning("Unexpected call of %s in running state", Q_FUNC_INFO);
    m_usedPorts.clear();
    m_remoteStdout.clear();
    m_remoteStderr.clear();
    m_procRunner = SshRemoteProcessRunner::create(connection);
    connect(m_procRunner.data(), SIGNAL(connectionError(Utils::SshError)),
        SLOT(handleConnectionError()));
    connect(m_procRunner.data(), SIGNAL(processClosed(int)),
        SLOT(handleProcessClosed(int)));
    connect(m_procRunner.data(), SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdOut(QByteArray)));
    connect(m_procRunner.data(), SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdErr(QByteArray)));
    const QString command = MaemoGlobal::remoteSudo()
        + QLatin1String(" lsof -nPi4tcp:") + portList.toString()
        + QLatin1String(" -F n |grep '^n' |sed -r 's/[^:]*:([[:digit:]]+).*/\\1/g' |sort -n |uniq");
    m_procRunner->run(command.toUtf8());
    m_running = true;
}

void MaemoUsedPortsGatherer::stop()
{
    if (!m_running)
        return;
    m_running = false;
    disconnect(m_procRunner->connection().data(), 0, this, 0);
    if (m_procRunner->process())
        m_procRunner->process()->closeChannel();
}

int MaemoUsedPortsGatherer::getNextFreePort(MaemoPortList *freePorts) const
{
    while (freePorts->hasMore()) {
        const int port = freePorts->getNext();
        if (!m_usedPorts.contains(port))
            return port;
    }
    return -1;
}

void MaemoUsedPortsGatherer::setupUsedPorts()
{
    const QList<QByteArray> &portStrings = m_remoteStdout.split('\n');
    foreach (const QByteArray &portString, portStrings) {
        if (portString.isEmpty())
            continue;
        bool ok;
        const int port = portString.toInt(&ok);
        if (ok) {
            m_usedPorts << port;
        } else {
            qWarning("%s: Unexpected string '%s' is not a port.",
                Q_FUNC_INFO, portString.data());
        }
    }
    emit portListReady();
}

void MaemoUsedPortsGatherer::handleConnectionError()
{
    if (!m_running)
        return;
    emit error(tr("Connection error: %1").
        arg(m_procRunner->connection()->errorString()));
    stop();
}

void MaemoUsedPortsGatherer::handleProcessClosed(int exitStatus)
{
    if (!m_running)
        return;
    QString errMsg;
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        errMsg = tr("Could not start remote process: %1")
            .arg(m_procRunner->process()->errorString());
        break;
    case SshRemoteProcess::KilledBySignal:
        errMsg = tr("Remote process crashed: %1")
            .arg(m_procRunner->process()->errorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        if (m_procRunner->process()->exitCode() == 0) {
            setupUsedPorts();
        } else {
            errMsg = tr("Remote process failed: %1")
                .arg(m_procRunner->process()->errorString());
        }
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (!errMsg.isEmpty()) {
        if (!m_remoteStderr.isEmpty()) {
            errMsg += tr("\nRemote error output was: %1")
                .arg(QString::fromUtf8(m_remoteStderr));
        }
        emit error(errMsg);
    }
    stop();
}

void MaemoUsedPortsGatherer::handleRemoteStdOut(const QByteArray &output)
{
    m_remoteStdout += output;
}

void MaemoUsedPortsGatherer::handleRemoteStdErr(const QByteArray &output)
{
    m_remoteStderr += output;
}

} // namespace Internal
} // namespace Qt4ProjectManager
