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
#include "maemoglobal.h"
#include "maemorunconfiguration.h"
#include "maemotoolchain.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <QtCore/QFileInfo>
#include <QtCore/QProcess>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

MaemoSshRunner::MaemoSshRunner(QObject *parent,
    MaemoRunConfiguration *runConfig, bool debugging)
    : QObject(parent), m_runConfig(runConfig),
      m_devConfig(runConfig->deviceConfig()),
      m_uploadJobId(SftpInvalidJob),
      m_debugging(debugging)
{
    m_procsToKill
        << QFileInfo(m_runConfig->localExecutableFilePath()).fileName()
        << QLatin1String("utfs-client");
    if (debugging)
        m_procsToKill << QLatin1String("gdbserver");
}

MaemoSshRunner::~MaemoSshRunner() {}

void MaemoSshRunner::setConnection(const QSharedPointer<Core::SshConnection> &connection)
{
    m_connection = connection;
}

void MaemoSshRunner::start()
{
    m_mountSpecs.clear();
    const MaemoRemoteMountsModel * const remoteMounts
        = m_runConfig->remoteMounts();
    for (int i = 0; i < remoteMounts->mountSpecificationCount(); ++i) {
        const MaemoRemoteMountsModel::MountSpecification &mountSpec
            = remoteMounts->mountSpecificationAt(i);
        if (mountSpec.isValid())
            m_mountSpecs << mountSpec;
    }
    if (m_debugging && m_runConfig->useRemoteGdb()) {
        m_mountSpecs << MaemoRemoteMountsModel::MountSpecification(
            m_runConfig->localDirToMountForRemoteGdb(),
            MaemoGlobal::remoteProjectSourcesMountPoint(),
            m_runConfig->gdbMountPort());
    }

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
    if (m_utfsClientUploader) {
        disconnect(m_utfsClientUploader.data(), 0, this, 0);
        m_utfsClientUploader->closeChannel();
    }
    if (m_mountProcess) {
        disconnect(m_mountProcess.data(), 0, this, 0);
        m_mountProcess->closeChannel();
    }
    if (m_debugging || m_runner) {
        if (m_runner) {
            disconnect(m_runner.data(), 0, this, 0);
            m_runner->closeChannel();
        }
        cleanup(false);
    }
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
    QString niceKill;
    QString brutalKill;
    foreach (const QString &proc, m_procsToKill) {
        niceKill += QString::fromLocal8Bit("pkill -x %1;").arg(proc);
        brutalKill += QString::fromLocal8Bit("pkill -x -9 %1;").arg(proc);
    }
    QString remoteCall = niceKill + QLatin1String("sleep 1; ") + brutalKill;

    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        remoteCall += QString::fromLocal8Bit("%1 umount %2;")
            .arg(MaemoGlobal::remoteSudo(), m_mountSpecs.at(i).remoteMountPoint);
    }
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

    foreach (QProcess *utfsServer, m_utfsServers) {
        utfsServer->terminate();
        utfsServer->waitForFinished(1000);
        utfsServer->kill();
    }
    qDeleteAll(m_utfsServers);
    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        emit error(tr("Initial cleanup failed: %1")
            .arg(m_initialCleaner->errorString()));
    } else if (!m_mountSpecs.isEmpty()) {
        deployUtfsClient();
    } else {
        emit readyForExecution();
    }
}

void MaemoSshRunner::deployUtfsClient()
{
    m_utfsClientUploader = m_connection->createSftpChannel();
    connect(m_utfsClientUploader.data(), SIGNAL(initialized()), this,
        SLOT(handleUploaderInitialized()));
    connect(m_utfsClientUploader.data(), SIGNAL(initializationFailed(QString)),
        this, SLOT(handleUploaderInitializationFailed(QString)));
    m_utfsClientUploader->initialize();
}

void MaemoSshRunner::handleUploaderInitializationFailed(const QString &reason)
{
    if (m_stop)
        return;

    emit error(tr("Failed to establish SFTP connection: %1").arg(reason));
}

void MaemoSshRunner::handleUploaderInitialized()
{
    if (m_stop)
        return;

    connect(m_utfsClientUploader.data(),
        SIGNAL(finished(Core::SftpJobId, QString)), this,
        SLOT(handleUploadFinished(Core::SftpJobId,QString)));
    const MaemoToolChain * const toolChain
        = dynamic_cast<const MaemoToolChain *>(m_runConfig->toolchain());
    Q_ASSERT_X(toolChain, Q_FUNC_INFO,
        "Impossible: Maemo run configuration has no Maemo Toolchain.");
    const QString localFile
        = toolChain->maddeRoot() + QLatin1String("/madlib/armel/utfs-client");
    m_uploadJobId
        = m_utfsClientUploader->uploadFile(localFile, utfsClientOnDevice(),
              SftpOverwriteExisting);
    if (m_uploadJobId == SftpInvalidJob)
        emit error(tr("Could not upload UTFS client (%1).").arg(localFile));
}

void MaemoSshRunner::handleUploadFinished(Core::SftpJobId jobId,
    const QString &errorMsg)
{
    if (m_stop)
        return;

    if (jobId != m_uploadJobId) {
        qWarning("Warning: unknown upload job %d finished.", jobId);
        return;
    }

    m_uploadJobId = SftpInvalidJob;
    if (!errorMsg.isEmpty()) {
        emit error(tr("Could not upload UTFS client: %1").arg(errorMsg));
        return;
    }

    mount();
}

void MaemoSshRunner::mount()
{
    const QString chmodFuse
        = MaemoGlobal::remoteSudo() + QLatin1String(" chmod a+r+w /dev/fuse");
    const QString chmodUtfsClient
        = QLatin1String("chmod a+x ") + utfsClientOnDevice();
    const QLatin1String andOp(" && ");
    QString remoteCall = chmodFuse + andOp + chmodUtfsClient;
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        const MaemoRemoteMountsModel::MountSpecification &mountSpec
            = m_mountSpecs.at(i);
        QProcess * const utfsServerProc = new QProcess(this);
        connect(utfsServerProc, SIGNAL(readyReadStandardError()), this,
            SLOT(handleUtfsServerErrorOutput()));
        const QString port = QString::number(mountSpec.port);
        const QString localSecretOpt = QLatin1String("-l");
        const QString remoteSecretOpt = QLatin1String("-r");
        const QStringList utfsServerArgs = QStringList() << localSecretOpt
            << port << remoteSecretOpt << port << QLatin1String("-b") << port
            << mountSpec.localDir;
        utfsServerProc->start(utfsServer(), utfsServerArgs);
        if (!utfsServerProc->waitForStarted()) {
            delete utfsServerProc;
            emit error(tr("Could not start UTFS server: %1")
                .arg(utfsServerProc->errorString()));
            return;
        }
        m_utfsServers << utfsServerProc;
        const QString mkdir = QString::fromLocal8Bit("%1 mkdir -p %2")
            .arg(MaemoGlobal::remoteSudo(), mountSpec.remoteMountPoint);
        const QString chmod = QString::fromLocal8Bit("%1 chmod a+r+w+x %2")
            .arg(MaemoGlobal::remoteSudo(), mountSpec.remoteMountPoint);
        const QString utfsClient
            = QString::fromLocal8Bit("%1 -l %2 -r %2 -c %3:%2 %4")
                  .arg(utfsClientOnDevice()).arg(port)
                  .arg(m_runConfig->localHostAddressFromDevice())
                  .arg(mountSpec.remoteMountPoint);
        remoteCall += andOp + mkdir + andOp + chmod + andOp + utfsClient;
    }

    m_mountProcess = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_mountProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleMountProcessFinished(int)));
    connect(m_mountProcess.data(), SIGNAL(errorOutputAvailable(QByteArray)),
        this, SIGNAL(remoteErrorOutput(QByteArray)));
    m_mountProcess->start();
}

void MaemoSshRunner::handleMountProcessFinished(int exitStatus)
{
    if (m_stop)
        return;

    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        emit error(tr("Could not execute mount request."));
        break;
    case SshRemoteProcess::KilledBySignal:
        emit error(tr("Failure running UTFS client: %1")
            .arg(m_mountProcess->errorString()));
        break;
    case SshRemoteProcess::ExitedNormally:
        if (m_mountProcess->exitCode() == 0) {
            emit readyForExecution();
        } else {
            emit error(tr("Could not execute mount request."));
        }
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO,
            "Impossible SshRemoteProcess exit status.");
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
    cleanup(false);
}

QString MaemoSshRunner::utfsClientOnDevice() const
{
    return MaemoGlobal::homeDirOnDevice(QLatin1String("developer"))
        + QLatin1String("/utfs-client");
}

QString MaemoSshRunner::utfsServer() const
{
    const MaemoToolChain * const toolChain
        = dynamic_cast<const MaemoToolChain *>(m_runConfig->toolchain());
    Q_ASSERT_X(toolChain, Q_FUNC_INFO,
        "Impossible: Maemo run configuration has no Maemo Toolchain.");
    return toolChain->maddeRoot() + QLatin1String("/madlib/utfs-server");
}

void MaemoSshRunner::handleUtfsServerErrorOutput()
{
    emit remoteErrorOutput(qobject_cast<QProcess *>(sender())->readAllStandardError());
}

} // namespace Internal
} // namespace Qt4ProjectManager

