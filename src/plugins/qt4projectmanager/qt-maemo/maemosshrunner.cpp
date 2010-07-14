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

#include "maemodeviceconfigurations.h"
#include "maemorunconfiguration.h"

#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <QtCore/QFileInfo>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

MaemoSshRunner::MaemoSshRunner(QObject *parent,
    MaemoRunConfiguration *runConfig)
    : QObject(parent), m_runConfig(runConfig),
      m_devConfig(runConfig->deviceConfig())
{
    m_procsToKill << QFileInfo(m_runConfig->localExecutableFilePath()).fileName();
}

MaemoSshRunner::~MaemoSshRunner() {}

void MaemoSshRunner::setConnection(const QSharedPointer<Core::SshConnection> &connection)
{
    m_connection = connection;
}

void MaemoSshRunner::addProcsToKill(const QStringList &appNames)
{
    m_procsToKill << appNames;
}

void MaemoSshRunner::start()
{
    m_stop = false;
    if (m_connection)
    disconnect(m_connection.data(), 0, this, 0);
    const bool reUse = m_connection
        && m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == m_devConfig.server;
    if (!reUse)
        m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(SshError)), this,
        SLOT(handleConnectionFailure()));
    if (reUse)
        handleConnected();
    else
        m_connection->connectToHost(m_devConfig.server);
}

void MaemoSshRunner::stop()
{
    m_stop = true;
    disconnect(m_connection.data(), 0, this, 0);
    if (m_initialCleaner)
        disconnect(m_initialCleaner.data(), 0, this, 0);
    if (m_runner) {
        disconnect(m_runner.data(), 0, this, 0);
        m_runner->closeChannel();
        killRemoteProcs(false);
    }
}

void MaemoSshRunner::handleConnected()
{
    if (m_stop)
        return;

    killRemoteProcs(true);
}

void MaemoSshRunner::handleConnectionFailure()
{
    emit error(tr("Could not connect to host: %1")
        .arg(m_connection->errorString()));
}

void MaemoSshRunner::killRemoteProcs(bool initialCleanup)
{
    QString niceKill;
    QString brutalKill;
    foreach (const QString &proc, m_procsToKill) {
        niceKill += QString::fromLocal8Bit("pkill -x %1;").arg(proc);
        brutalKill += QString::fromLocal8Bit("pkill -x -9 %1;").arg(proc);
    }
    QString remoteCall = niceKill + QLatin1String("sleep 1; ") + brutalKill;
    remoteCall.remove(remoteCall.count() - 1, 1); // Get rid of trailing semicolon.
    SshRemoteProcess::Ptr proc
        = m_connection->createRemoteProcess(remoteCall.toUtf8());
    if (initialCleanup) {
        m_initialCleaner = proc;
        connect(m_initialCleaner.data(), SIGNAL(closed(int)), this,
            SLOT(handleInitialCleanupFinished(int)));
    }
    proc->start();
}

void MaemoSshRunner::handleInitialCleanupFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (m_stop)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        emit error(tr("Initial cleanup failed: %1")
            .arg(m_initialCleaner->errorString()));
    } else {
        emit readyForExecution();
    }
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

    if (m_stop)
        return;

    if (exitStatus == SshRemoteProcess::ExitedNormally) {
        emit remoteProcessFinished(m_runner->exitCode());
    } else {
        emit error(tr("Error running remote process: %1")
            .arg(m_runner->errorString()));
    }
}


} // namespace Internal
} // namespace Qt4ProjectManager

