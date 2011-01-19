/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
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
**************************************************************************/

#include "maemoremotemounter.h"

#include "maemoglobal.h"
#include "maemousedportsgatherer.h"
#include "qt4maemotarget.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

MaemoRemoteMounter::MaemoRemoteMounter(QObject *parent)
    : QObject(parent), m_utfsServerTimer(new QTimer(this)),
      m_uploadJobId(SftpInvalidJob), m_state(Inactive)
{
    connect(m_utfsServerTimer, SIGNAL(timeout()), this,
        SLOT(handleUtfsServerTimeout()));
    m_utfsServerTimer->setSingleShot(true);
}

MaemoRemoteMounter::~MaemoRemoteMounter()
{
    killAllUtfsServers();
}

void MaemoRemoteMounter::setConnection(const SshConnection::Ptr &connection)
{
    ASSERT_STATE(Inactive);
    m_connection = connection;
}

void MaemoRemoteMounter::setBuildConfiguration(const Qt4BuildConfiguration *bc)
{
    ASSERT_STATE(Inactive);
    const QtVersion * const qtVersion = bc->qtVersion();
    m_remoteMountsAllowed
        = qobject_cast<AbstractQt4MaemoTarget *>(bc->target())->allowsRemoteMounts();
    m_maddeRoot = MaemoGlobal::maddeRoot(qtVersion);
}

void MaemoRemoteMounter::addMountSpecification(const MaemoMountSpecification &mountSpec,
    bool mountAsRoot)
{
    ASSERT_STATE(Inactive);

    if (m_remoteMountsAllowed && mountSpec.isValid())
        m_mountSpecs << MountInfo(mountSpec, mountAsRoot);
}

bool MaemoRemoteMounter::hasValidMountSpecifications() const
{
    return !m_mountSpecs.isEmpty();
}

void MaemoRemoteMounter::mount(MaemoPortList *freePorts,
    const MaemoUsedPortsGatherer *portsGatherer)
{
    ASSERT_STATE(Inactive);
    Q_ASSERT(m_utfsServers.isEmpty());
    Q_ASSERT(m_connection);

    if (m_mountSpecs.isEmpty()) {
        setState(Inactive);
        emit reportProgress(tr("No directories to mount"));
        emit mounted();
    } else {
        m_freePorts = freePorts;
        m_portsGatherer = portsGatherer;
        deployUtfsClient();
    }
}

void MaemoRemoteMounter::unmount()
{
    ASSERT_STATE(Inactive);

    if (m_mountSpecs.isEmpty()) {
        emit reportProgress(tr("No directories to unmount"));
        emit unmounted();
        return;
    }

    QString remoteCall;
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        remoteCall += QString::fromLocal8Bit("%1 umount %2 && %1 rmdir %2;")
            .arg(MaemoGlobal::remoteSudo(),
                m_mountSpecs.at(i).mountSpec.remoteMountPoint);
    }

    m_umountStderr.clear();
    m_unmountProcess = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_unmountProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleUnmountProcessFinished(int)));
    connect(m_unmountProcess.data(), SIGNAL(errorOutputAvailable(QByteArray)),
        this, SLOT(handleUmountStderr(QByteArray)));
    setState(Unmounting);
    m_unmountProcess->start();
}

void MaemoRemoteMounter::handleUnmountProcessFinished(int exitStatus)
{
    ASSERT_STATE(QList<State>() << Unmounting << Inactive);

    if (m_state == Inactive)
        return;
    setState(Inactive);

    QString errorMsg;
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        errorMsg = tr("Could not execute unmount request.");
        break;
    case SshRemoteProcess::KilledBySignal:
        errorMsg = tr("Failure unmounting: %1")
            .arg(m_unmountProcess->errorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO,
            "Impossible SshRemoteProcess exit status.");
    }

    killAllUtfsServers();

    if (errorMsg.isEmpty()) {
        emit reportProgress(tr("Finished unmounting."));
        emit unmounted();
    } else {
        if (!m_umountStderr.isEmpty()) {
            errorMsg += tr("\nstderr was: '%1'")
                .arg(QString::fromUtf8(m_umountStderr));
        }
        emit error(errorMsg);
    }
}

void MaemoRemoteMounter::stop()
{
    setState(Inactive);
}

void MaemoRemoteMounter::deployUtfsClient()
{
    emit reportProgress(tr("Setting up SFTP connection..."));
    m_utfsClientUploader = m_connection->createSftpChannel();
    connect(m_utfsClientUploader.data(), SIGNAL(initialized()), this,
        SLOT(handleUploaderInitialized()));
    connect(m_utfsClientUploader.data(), SIGNAL(initializationFailed(QString)),
        this, SLOT(handleUploaderInitializationFailed(QString)));
    m_utfsClientUploader->initialize();

    setState(UploaderInitializing);
}

void MaemoRemoteMounter::handleUploaderInitializationFailed(const QString &reason)
{
    ASSERT_STATE(QList<State>() << UploaderInitializing << Inactive);

    if (m_state == UploaderInitializing) {
        emit error(tr("Failed to establish SFTP connection: %1").arg(reason));
        setState(Inactive);
    }
}

void MaemoRemoteMounter::handleUploaderInitialized()
{
    ASSERT_STATE(QList<State>() << UploaderInitializing << Inactive);
    if (m_state == Inactive)
        return;

    emit reportProgress(tr("Uploading UTFS client..."));
    connect(m_utfsClientUploader.data(),
        SIGNAL(finished(Core::SftpJobId, QString)), this,
        SLOT(handleUploadFinished(Core::SftpJobId, QString)));
    const QString localFile
        = m_maddeRoot + QLatin1String("/madlib/armel/utfs-client");
    m_uploadJobId = m_utfsClientUploader->uploadFile(localFile,
        utfsClientOnDevice(), SftpOverwriteExisting);
    if (m_uploadJobId == SftpInvalidJob) {
        setState(Inactive);
        emit error(tr("Could not upload UTFS client (%1).").arg(localFile));
    } else {
        setState(UploadRunning);
    }
}

void MaemoRemoteMounter::handleUploadFinished(Core::SftpJobId jobId,
    const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << UploadRunning << Inactive);

    if (m_state == Inactive)
        return;

    if (jobId != m_uploadJobId) {
        qWarning("Warning: unknown upload job %d finished.", jobId);
        return;
    }

    m_uploadJobId = SftpInvalidJob;
    if (!errorMsg.isEmpty()) {
        emit reportProgress(tr("Could not upload UTFS client (%1), continuing anyway.")
            .arg(errorMsg));
    }

    startUtfsClients();
}

void MaemoRemoteMounter::startUtfsClients()
{
    const QString chmodFuse
        = MaemoGlobal::remoteSudo() + QLatin1String(" chmod a+r+w /dev/fuse");
    const QString chmodUtfsClient
        = QLatin1String("chmod a+x ") + utfsClientOnDevice();
    const QLatin1String andOp(" && ");
    QString remoteCall = chmodFuse + andOp + chmodUtfsClient;
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        MountInfo &mountInfo = m_mountSpecs[i];
        mountInfo.remotePort
            = m_portsGatherer->getNextFreePort(m_freePorts);
        if (mountInfo.remotePort == -1) {
            setState(Inactive);
            emit error(tr("Error: Not enough free ports on device to fulfill all mount requests."));
            return;
        }

        const MaemoMountSpecification &mountSpec = mountInfo.mountSpec;
        const QString mkdir = QString::fromLocal8Bit("%1 mkdir -p %2")
            .arg(MaemoGlobal::remoteSudo(), mountSpec.remoteMountPoint);
        const QString chmod = QString::fromLocal8Bit("%1 chmod a+r+w+x %2")
            .arg(MaemoGlobal::remoteSudo(), mountSpec.remoteMountPoint);
        QString utfsClient
            = QString::fromLocal8Bit("%1 -l %2 -r %2 -b %2 %4 -o nonempty")
                  .arg(utfsClientOnDevice()).arg(mountInfo.remotePort)
                  .arg(mountSpec.remoteMountPoint);
        if (mountInfo.mountAsRoot)
            utfsClient.prepend(MaemoGlobal::remoteSudo() + QLatin1Char(' '));
        QLatin1String seqOp("; ");
        remoteCall += seqOp + MaemoGlobal::remoteSourceProfilesCommand()
            + seqOp + mkdir + andOp + chmod + andOp + utfsClient;
    }

    emit reportProgress(tr("Starting remote UTFS clients..."));
    m_utfsClientStderr.clear();
    m_mountProcess = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_mountProcess.data(), SIGNAL(started()), this,
        SLOT(handleUtfsClientsStarted()));
    connect(m_mountProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleUtfsClientsFinished(int)));
    connect(m_mountProcess.data(), SIGNAL(errorOutputAvailable(QByteArray)),
        this, SLOT(handleUtfsClientStderr(QByteArray)));
    m_mountProcess->start();

    setState(UtfsClientsStarting);
}

void MaemoRemoteMounter::handleUtfsClientsStarted()
{
    ASSERT_STATE(QList<State>() << UtfsClientsStarting << Inactive);
    if (m_state == UtfsClientsStarting) {
        setState(UtfsClientsStarted);
        QTimer::singleShot(250, this, SLOT(startUtfsServers()));
    }
}

void MaemoRemoteMounter::handleUtfsClientsFinished(int exitStatus)
{
    ASSERT_STATE(QList<State>() << UtfsClientsStarting << UtfsClientsStarted
        << UtfsServersStarted << Inactive);

    if (m_state == Inactive)
        return;

    setState(Inactive);
    if (exitStatus == SshRemoteProcess::ExitedNormally
            && m_mountProcess->exitCode() == 0) {
        emit reportProgress(tr("Mount operation succeeded."));
        emit mounted();
    } else {
        QString errMsg = tr("Failure running UTFS client: %1")
            .arg(m_mountProcess->errorString());
        if (!m_utfsClientStderr.isEmpty())
            errMsg += tr("\nstderr was: '%1'")
               .arg(QString::fromUtf8(m_utfsClientStderr));
        emit error(errMsg);
    }
}

void MaemoRemoteMounter::startUtfsServers()
{
    ASSERT_STATE(QList<State>() << UtfsClientsStarted << Inactive);

    if (m_state == Inactive)
        return;

    emit reportProgress(tr("Starting UTFS servers..."));
    m_utfsServerTimer->start(30000);
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        const MountInfo &mountInfo = m_mountSpecs.at(i);
        const MaemoMountSpecification &mountSpec = mountInfo.mountSpec;
        const ProcPtr utfsServerProc(new QProcess);
        const QString port = QString::number(mountInfo.remotePort);
        const QString localSecretOpt = QLatin1String("-l");
        const QString remoteSecretOpt = QLatin1String("-r");
        const QStringList utfsServerArgs = QStringList() << localSecretOpt
            << port << remoteSecretOpt << port << QLatin1String("-c")
            << (m_connection->connectionParameters().host + QLatin1Char(':') + port)
            << mountSpec.localDir;
        connect(utfsServerProc.data(),
            SIGNAL(finished(int,QProcess::ExitStatus)), this,
            SLOT(handleUtfsServerFinished(int,QProcess::ExitStatus)));
        connect(utfsServerProc.data(), SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(handleUtfsServerError(QProcess::ProcessError)));
        connect(utfsServerProc.data(), SIGNAL(readyReadStandardError()), this,
            SLOT(handleUtfsServerStderr()));
        m_utfsServers << utfsServerProc;
        utfsServerProc->start(utfsServer(), utfsServerArgs);
    }

    setState(UtfsServersStarted);
}

void MaemoRemoteMounter::handleUtfsServerStderr()
{
    if (m_state != Inactive) {
        QProcess * const proc = static_cast<QProcess *>(sender());
        const QByteArray &output = proc->readAllStandardError();
        emit debugOutput(QString::fromLocal8Bit(output));
    }
}

void MaemoRemoteMounter::handleUtfsServerError(QProcess::ProcessError)
{
    if (m_state == Inactive || m_utfsServers.isEmpty())
        return;

    QProcess * const proc = static_cast<QProcess *>(sender());
    QString errorString = proc->errorString();
    const QByteArray &errorOutput = proc->readAllStandardError();
    if (!errorOutput.isEmpty()) {
        errorString += tr("\nstderr was: %1")
            .arg(QString::fromLocal8Bit(errorOutput));
    }
    killAllUtfsServers();
    emit error(tr("Error running UTFS server: %1").arg(errorString));

    setState(Inactive);
}

void MaemoRemoteMounter::handleUtfsServerFinished(int /* exitCode */,
    QProcess::ExitStatus exitStatus)
{
    if (m_state != Inactive && exitStatus != QProcess::NormalExit)
        handleUtfsServerError(static_cast<QProcess *>(sender())->error());
}

void MaemoRemoteMounter::handleUtfsClientStderr(const QByteArray &output)
{
    if (m_state != Inactive)
        m_utfsClientStderr += output;
}

void MaemoRemoteMounter::handleUmountStderr(const QByteArray &output)
{
    if (m_state != Inactive)
        m_umountStderr += output;
}

QString MaemoRemoteMounter::utfsClientOnDevice() const
{
    return MaemoGlobal::homeDirOnDevice(m_connection->connectionParameters().uname)
        + QLatin1String("/utfs-client");
}

QString MaemoRemoteMounter::utfsServer() const
{
    return m_maddeRoot + QLatin1String("/madlib/utfs-server");
}

void MaemoRemoteMounter::killAllUtfsServers()
{
    foreach (const ProcPtr &proc, m_utfsServers)
        killUtfsServer(proc.data());
    m_utfsServers.clear();
}

void MaemoRemoteMounter::killUtfsServer(QProcess *proc)
{
    disconnect(proc, 0, this, 0);
    proc->terminate();
    proc->waitForFinished(1000);
    proc->kill();
}

void MaemoRemoteMounter::handleUtfsServerTimeout()
{
    ASSERT_STATE(QList<State>() << UtfsServersStarted << Inactive);
    if (m_state == Inactive)
        return;

    killAllUtfsServers();
    emit error(tr("Timeout waiting for UTFS servers to connect."));

    setState(Inactive);
}

void MaemoRemoteMounter::setState(State newState)
{
    if (newState == Inactive) {
        m_utfsServerTimer->stop();
        if (m_utfsClientUploader) {
            disconnect(m_utfsClientUploader.data(), 0, this, 0);
            m_utfsClientUploader->closeChannel();
        }
        if (m_mountProcess) {
            disconnect(m_mountProcess.data(), 0, this, 0);
            m_mountProcess->closeChannel();
        }
        if (m_unmountProcess) {
            disconnect(m_unmountProcess.data(), 0, this, 0);
            m_unmountProcess->closeChannel();
        }
    }
    m_state = newState;
}

} // namespace Internal
} // namespace Qt4ProjectManager
