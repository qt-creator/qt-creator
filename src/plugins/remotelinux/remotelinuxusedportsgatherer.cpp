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
#include "remotelinuxusedportsgatherer.h"

#include "linuxdeviceconfiguration.h"
#include "portlist.h"

#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtCore/QString>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxUsedPortsGathererPrivate
{
 public:
    RemoteLinuxUsedPortsGathererPrivate() : running(false) {}

    SshRemoteProcessRunner::Ptr procRunner;
    QList<int> usedPorts;
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    bool running;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxUsedPortsGatherer::RemoteLinuxUsedPortsGatherer(QObject *parent) :
    QObject(parent), m_d(new RemoteLinuxUsedPortsGathererPrivate)
{
}

RemoteLinuxUsedPortsGatherer::~RemoteLinuxUsedPortsGatherer()
{
    delete m_d;
}

void RemoteLinuxUsedPortsGatherer::start(const Utils::SshConnection::Ptr &connection,
    const LinuxDeviceConfiguration::ConstPtr &devConf)
{
    if (m_d->running)
        qWarning("Unexpected call of %s in running state", Q_FUNC_INFO);
    m_d->usedPorts.clear();
    m_d->remoteStdout.clear();
    m_d->remoteStderr.clear();
    m_d->procRunner = SshRemoteProcessRunner::create(connection);
    connect(m_d->procRunner.data(), SIGNAL(connectionError(Utils::SshError)),
        SLOT(handleConnectionError()));
    connect(m_d->procRunner.data(), SIGNAL(processClosed(int)),
        SLOT(handleProcessClosed(int)));
    connect(m_d->procRunner.data(), SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdOut(QByteArray)));
    connect(m_d->procRunner.data(), SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdErr(QByteArray)));
    const QString command = QLatin1String("lsof -nPi4tcp:") + devConf->freePorts().toString()
        + QLatin1String(" -F n |grep '^n' |sed -r 's/[^:]*:([[:digit:]]+).*/\\1/g' |sort -n |uniq");
    m_d->procRunner->run(command.toUtf8());
    m_d->running = true;
}

void RemoteLinuxUsedPortsGatherer::stop()
{
    if (!m_d->running)
        return;
    m_d->running = false;
    disconnect(m_d->procRunner->connection().data(), 0, this, 0);
    if (m_d->procRunner->process())
        m_d->procRunner->process()->closeChannel();
}

int RemoteLinuxUsedPortsGatherer::getNextFreePort(PortList *freePorts) const
{
    while (freePorts->hasMore()) {
        const int port = freePorts->getNext();
        if (!m_d->usedPorts.contains(port))
            return port;
    }
    return -1;
}

QList<int> RemoteLinuxUsedPortsGatherer::usedPorts() const
{
    return m_d->usedPorts;
}

void RemoteLinuxUsedPortsGatherer::setupUsedPorts()
{
    const QList<QByteArray> &portStrings = m_d->remoteStdout.split('\n');
    foreach (const QByteArray &portString, portStrings) {
        if (portString.isEmpty())
            continue;
        bool ok;
        const int port = portString.toInt(&ok);
        if (ok) {
            m_d->usedPorts << port;
        } else {
            qWarning("%s: Unexpected string '%s' is not a port.",
                Q_FUNC_INFO, portString.data());
        }
    }
    emit portListReady();
}

void RemoteLinuxUsedPortsGatherer::handleConnectionError()
{
    if (!m_d->running)
        return;
    emit error(tr("Connection error: %1").
        arg(m_d->procRunner->connection()->errorString()));
    stop();
}

void RemoteLinuxUsedPortsGatherer::handleProcessClosed(int exitStatus)
{
    if (!m_d->running)
        return;
    QString errMsg;
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        errMsg = tr("Could not start remote process: %1")
            .arg(m_d->procRunner->process()->errorString());
        break;
    case SshRemoteProcess::KilledBySignal:
        errMsg = tr("Remote process crashed: %1")
            .arg(m_d->procRunner->process()->errorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        if (m_d->procRunner->process()->exitCode() == 0) {
            setupUsedPorts();
        } else {
            errMsg = tr("Remote process failed; exit code was %1.")
                .arg(m_d->procRunner->process()->exitCode());
        }
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (!errMsg.isEmpty()) {
        if (!m_d->remoteStderr.isEmpty()) {
            errMsg += tr("\nRemote error output was: %1")
                .arg(QString::fromUtf8(m_d->remoteStderr));
        }
        emit error(errMsg);
    }
    stop();
}

void RemoteLinuxUsedPortsGatherer::handleRemoteStdOut(const QByteArray &output)
{
    m_d->remoteStdout += output;
}

void RemoteLinuxUsedPortsGatherer::handleRemoteStdErr(const QByteArray &output)
{
    m_d->remoteStderr += output;
}

} // namespace RemoteLinux
