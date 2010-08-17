/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "maemosshrunner.h"

#include "maemodeploystep.h"
#include "maemodeviceconfigurations.h"
#include "maemoglobal.h"
#include "maemoremotemounter.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfiguration.h"

#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <QtCore/QFileInfo>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

MaemoSshRunner::MaemoSshRunner(QObject *parent,
    MaemoRunConfiguration *runConfig, bool debugging)
    : QObject(parent), m_runConfig(runConfig),
      m_mounter(new MaemoRemoteMounter(this, runConfig->toolchain())),
      m_devConfig(runConfig->deviceConfig()), m_shuttingDown(false),
      m_debugging(debugging)
{
    m_procsToKill
        << QFileInfo(m_runConfig->localExecutableFilePath()).fileName()
        << QLatin1String("utfs-client");
    if (debugging)
        m_procsToKill << QLatin1String("gdbserver");
    connect(m_mounter, SIGNAL(mounted()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMounterError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SIGNAL(reportProgress(QString)));
}

MaemoSshRunner::~MaemoSshRunner() {}

void MaemoSshRunner::setConnection(const QSharedPointer<Core::SshConnection> &connection)
{
    m_connection = connection;
}

void MaemoSshRunner::start()
{
    // Should not happen.
    if (m_shuttingDown) {
        emit error(tr("Can't restart yet, haven't shut down properly."));
        return;
    }

    m_stop = false;
    m_exitStatus = -1;
    if (m_connection)
        disconnect(m_connection.data(), 0, this, 0);
    bool reUse = isConnectionUsable();
    if (!reUse)
        m_connection = m_runConfig->deployStep()->sshConnection();
    reUse = isConnectionUsable();
    if (!reUse)
        m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(SshError)), this,
        SLOT(handleConnectionFailure()));
    if (reUse) {
        handleConnected();
    } else {
        emit reportProgress(tr("Connecting to device..."));
        m_connection->connectToHost(m_devConfig.server);
    }
}

void MaemoSshRunner::stop()
{
    if (m_shuttingDown)
        return;

    m_stop = true;
    m_mounter->stop();
    if (m_cleaner)
        disconnect(m_cleaner.data(), 0, this, 0);
    cleanup(false);
}

void MaemoSshRunner::handleConnected()
{
    if (m_stop)
        return;

    cleanup(true);
}

void MaemoSshRunner::handleConnectionFailure()
{
    emit error(tr("Could not connect to host: %1")
        .arg(m_connection->errorString()));
}

void MaemoSshRunner::cleanup(bool initialCleanup)
{
    if (!isConnectionUsable())
        return;

    emit reportProgress(tr("Killing remote process(es)..."));
    m_shuttingDown = !initialCleanup;
    QString niceKill;
    QString brutalKill;
    foreach (const QString &proc, m_procsToKill) {
        niceKill += QString::fromLocal8Bit("pkill -x %1;").arg(proc);
        brutalKill += QString::fromLocal8Bit("pkill -x -9 %1;").arg(proc);
    }
    QString remoteCall = niceKill + QLatin1String("sleep 1; ") + brutalKill;
    remoteCall.remove(remoteCall.count() - 1, 1); // Get rid of trailing semicolon.

    m_cleaner = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_cleaner.data(), SIGNAL(closed(int)), this,
        SLOT(handleCleanupFinished(int)));
    m_cleaner->start();
}

void MaemoSshRunner::handleCleanupFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (m_shuttingDown) {
        m_unmountState = ShutdownUnmount;
        m_mounter->unmount();
        return;
    }

    if (m_stop)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        emit error(tr("Initial cleanup failed: %1")
            .arg(m_cleaner->errorString()));
    } else {
        m_mounter->setConnection(m_connection);
        m_unmountState = InitialUnmount;
        m_mounter->unmount();
    }
}

void MaemoSshRunner::handleUnmounted()
{
    switch (m_unmountState) {
    case InitialUnmount: {
        if (m_stop)
            return;
        MaemoPortList portList = m_devConfig.freePorts();
        if (m_debugging && !m_runConfig->useRemoteGdb())
            portList.getNext(); // One has already been used for gdbserver.
        m_mounter->setPortList(portList);
        const MaemoRemoteMountsModel * const remoteMounts
            = m_runConfig->remoteMounts();
        for (int i = 0; i < remoteMounts->mountSpecificationCount(); ++i) {
            if (!addMountSpecification(remoteMounts->mountSpecificationAt(i)))
                return;
        }
        if (m_debugging && m_runConfig->useRemoteGdb()) {
            if (!addMountSpecification(MaemoMountSpecification(
                m_runConfig->localDirToMountForRemoteGdb(),
                MaemoGlobal::remoteProjectSourcesMountPoint())))
                return;
        }
        m_unmountState = PreMountUnmount;
        m_mounter->unmount();
        break;
    }
    case PreMountUnmount:
        if (m_stop)
            return;
        m_mounter->mount();
        break;
    case ShutdownUnmount:
        Q_ASSERT(m_shuttingDown);
        m_shuttingDown = false;
        if (m_exitStatus == SshRemoteProcess::ExitedNormally) {
            emit remoteProcessFinished(m_runner->exitCode());
        } else if (m_exitStatus == -1) {
            emit remoteProcessFinished(-1);
        } else {
            emit error(tr("Error running remote process: %1")
                .arg(m_runner->errorString()));
        }
        m_exitStatus = -1;
        break;
    }
}

void MaemoSshRunner::handleMounted()
{
    if (!m_stop)
        emit readyForExecution();
}

void MaemoSshRunner::handleMounterError(const QString &errorMsg)
{
    if (m_shuttingDown)
        m_shuttingDown = false;
    emit error(errorMsg);
}

void MaemoSshRunner::startExecution(const QByteArray &remoteCall)
{
    if (m_runConfig->remoteExecutableFilePath().isEmpty()) {
        emit error(tr("Cannot run: No remote executable set."));
        return;
    }

    m_runner = m_connection->createRemoteProcess(remoteCall);
    connect(m_runner.data(), SIGNAL(started()), this,
        SIGNAL(remoteProcessStarted()));
    connect(m_runner.data(), SIGNAL(closed(int)), this,
        SLOT(handleRemoteProcessFinished(int)));
    connect(m_runner.data(), SIGNAL(outputAvailable(QByteArray)), this,
        SIGNAL(remoteOutput(QByteArray)));
    connect(m_runner.data(), SIGNAL(errorOutputAvailable(QByteArray)), this,
        SIGNAL(remoteErrorOutput(QByteArray)));
    m_runner->start();
}

void MaemoSshRunner::handleRemoteProcessFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    m_exitStatus = exitStatus;
    cleanup(false);
}

bool MaemoSshRunner::addMountSpecification(const MaemoMountSpecification &mountSpec)
{
    if (!m_mounter->addMountSpecification(mountSpec, false)) {
        emit error(tr("The device does not have enough free ports "
            "for this run configuration."));
        return false;
    }
    return true;
}

bool MaemoSshRunner::isConnectionUsable() const
{
    return m_connection && m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == m_devConfig.server;
}

} // namespace Internal
} // namespace Qt4ProjectManager

