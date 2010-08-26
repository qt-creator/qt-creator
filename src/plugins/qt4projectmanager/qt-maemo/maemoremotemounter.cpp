/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "maemoremotemounter.h"

#include "maemoglobal.h"
#include "maemotoolchain.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <QtCore/QTimer>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

MaemoRemoteMounter::MaemoRemoteMounter(QObject *parent)
    : QObject(parent), m_utfsServerTimer(new QTimer(this)),
      m_uploadJobId(SftpInvalidJob), m_stop(false)
{
    connect(m_utfsServerTimer, SIGNAL(timeout()), this,
        SLOT(handleUtfsServerTimeout()));
}

MaemoRemoteMounter::~MaemoRemoteMounter()
{
    killAllUtfsServers();
}

void MaemoRemoteMounter::setConnection(const Core::SshConnection::Ptr &connection)
{
    m_connection = connection;
}

bool MaemoRemoteMounter::addMountSpecification(const MaemoMountSpecification &mountSpec,
    bool mountAsRoot)
{
    if (mountSpec.isValid()) {
        if (!m_portList.hasMore())
            return false;
        else
            m_mountSpecs << MountInfo(mountSpec, m_portList.getNext(), mountAsRoot);
    }
    return true;
}

void MaemoRemoteMounter::mount()
{
    m_stop = false;
    Q_ASSERT(m_utfsServers.isEmpty());

    if (!m_toolChain->allowsRemoteMounts())
        m_mountSpecs.clear();
    if (m_mountSpecs.isEmpty())
        emit mounted();
    else
        deployUtfsClient();
}

void MaemoRemoteMounter::unmount()
{
    m_stop = false;
    if (m_mountSpecs.isEmpty()) {
        emit unmounted();
        return;
    }

    QString remoteCall;
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        remoteCall += QString::fromLocal8Bit("%1 umount %2;")
            .arg(MaemoGlobal::remoteSudo(),
                m_mountSpecs.at(i).mountSpec.remoteMountPoint);
    }

    emit reportProgress(tr("Unmounting remote mount points..."));
    m_umountStderr.clear();
    m_unmountProcess = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_unmountProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleUnmountProcessFinished(int)));
    connect(m_unmountProcess.data(), SIGNAL(errorOutputAvailable(QByteArray)),
        this, SLOT(handleUmountStderr(QByteArray)));
    m_unmountProcess->start();
}

void MaemoRemoteMounter::handleUnmountProcessFinished(int exitStatus)
{
    if (m_stop)
        return;

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
    m_stop = true;
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

void MaemoRemoteMounter::deployUtfsClient()
{
    emit reportProgress(tr("Setting up SFTP connection..."));
    m_utfsClientUploader = m_connection->createSftpChannel();
    connect(m_utfsClientUploader.data(), SIGNAL(initialized()), this,
        SLOT(handleUploaderInitialized()));
    connect(m_utfsClientUploader.data(), SIGNAL(initializationFailed(QString)),
        this, SLOT(handleUploaderInitializationFailed(QString)));
    m_utfsClientUploader->initialize();
}

void MaemoRemoteMounter::handleUploaderInitializationFailed(const QString &reason)
{
    if (m_stop)
        return;

    emit error(tr("Failed to establish SFTP connection: %1").arg(reason));
}

void MaemoRemoteMounter::handleUploaderInitialized()
{
    if (m_stop)
        return;

    emit reportProgress(tr("Uploading UTFS client..."));
    connect(m_utfsClientUploader.data(),
        SIGNAL(finished(Core::SftpJobId, QString)), this,
        SLOT(handleUploadFinished(Core::SftpJobId, QString)));
    const QString localFile
        = m_toolChain->maddeRoot() + QLatin1String("/madlib/armel/utfs-client");
    m_uploadJobId = m_utfsClientUploader->uploadFile(localFile,
        utfsClientOnDevice(), SftpOverwriteExisting);
    if (m_uploadJobId == SftpInvalidJob)
        emit error(tr("Could not upload UTFS client (%1).").arg(localFile));
}

void MaemoRemoteMounter::handleUploadFinished(Core::SftpJobId jobId,
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
        const MountInfo &mountInfo = m_mountSpecs.at(i);
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
        remoteCall += andOp + mkdir + andOp + chmod + andOp + utfsClient;
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
}

void MaemoRemoteMounter::handleUtfsClientsStarted()
{
    if (!m_stop)
        QTimer::singleShot(250, this, SLOT(startUtfsServers()));
}

void MaemoRemoteMounter::handleUtfsClientsFinished(int exitStatus)
{
    if (m_stop)
        return;

    if (exitStatus == SshRemoteProcess::ExitedNormally
            && m_mountProcess->exitCode() == 0) {
        m_utfsServerTimer->stop();
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
    if (m_stop)
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
}

void MaemoRemoteMounter::handleUtfsServerStderr()
{
    if (!m_stop) {
        QProcess * const proc = static_cast<QProcess *>(sender());
        const QByteArray &output = proc->readAllStandardError();
        emit debugOutput(QString::fromLocal8Bit(output));
    }
}

void MaemoRemoteMounter::handleUtfsServerError(QProcess::ProcessError)
{
    if (m_stop || m_utfsServers.isEmpty())
        return;

    QProcess * const proc = static_cast<QProcess *>(sender());
    QString errorString = proc->errorString();
    const QByteArray &errorOutput = proc->readAllStandardError();
    if (!errorOutput.isEmpty()) {
        errorString += tr("\nstderr was: %1")
            .arg(QString::fromLocal8Bit(errorOutput));
    }
    killAllUtfsServers();
    m_utfsServerTimer->stop();
    emit error(tr("Error running UTFS server: %1").arg(errorString));
}

void MaemoRemoteMounter::handleUtfsServerFinished(int /* exitCode */,
    QProcess::ExitStatus exitStatus)
{
    if (!m_stop && exitStatus != QProcess::NormalExit)
        handleUtfsServerError(static_cast<QProcess *>(sender())->error());
}

void MaemoRemoteMounter::handleUtfsClientStderr(const QByteArray &output)
{
    m_utfsClientStderr += output;
}

void MaemoRemoteMounter::handleUmountStderr(const QByteArray &output)
{
    m_umountStderr += output;
}

QString MaemoRemoteMounter::utfsClientOnDevice() const
{
    return MaemoGlobal::homeDirOnDevice(QLatin1String("developer"))
        + QLatin1String("/utfs-client");
}

QString MaemoRemoteMounter::utfsServer() const
{
    return m_toolChain->maddeRoot() + QLatin1String("/madlib/utfs-server");
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
    if (m_stop)
        return;

    killAllUtfsServers();
    emit error(tr("Timeout waiting for UTFS servers to connect."));
}

} // namespace Internal
} // namespace Qt4ProjectManager
