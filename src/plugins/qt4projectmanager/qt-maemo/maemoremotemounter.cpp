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

#include <QtCore/QProcess>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

MaemoRemoteMounter::MaemoRemoteMounter(QObject *parent,
    const MaemoToolChain *toolchain)
    : QObject(parent), m_toolChain(toolchain), m_uploadJobId(SftpInvalidJob),
      m_stop(false)
{
}

MaemoRemoteMounter::~MaemoRemoteMounter() {}

void MaemoRemoteMounter::setConnection(const Core::SshConnection::Ptr &connection)
{
    m_connection = connection;
}

void MaemoRemoteMounter::addMountSpecification(const MaemoMountSpecification &mountSpec)
{
    if (mountSpec.isValid())
        m_mountSpecs << mountSpec;
}

void MaemoRemoteMounter::mount(const MaemoDeviceConfig &devConfig)
{
    m_devConfig = devConfig;
    m_stop = false;
    Q_ASSERT(m_utfsServers.isEmpty());

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
                m_mountSpecs.at(i).remoteMountPoint);
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

    foreach (const ProcPtr &utfsServer, m_utfsServers) {
        utfsServer->terminate();
        utfsServer->waitForFinished(1000);
        utfsServer->kill();
    }
    m_mountSpecs.clear();

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
        const MaemoMountSpecification &mountSpec = m_mountSpecs.at(i);
        const QString mkdir = QString::fromLocal8Bit("%1 mkdir -p %2")
            .arg(MaemoGlobal::remoteSudo(), mountSpec.remoteMountPoint);
        const QString chmod = QString::fromLocal8Bit("%1 chmod a+r+w+x %2")
            .arg(MaemoGlobal::remoteSudo(), mountSpec.remoteMountPoint);
        const QString utfsClient
            = QString::fromLocal8Bit("%1 -l %2 -r %2 -b %2 %4")
                  .arg(utfsClientOnDevice()).arg(mountSpec.remotePort)
                  .arg(mountSpec.remoteMountPoint);
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
        startUtfsServers();
}

void MaemoRemoteMounter::handleUtfsClientsFinished(int exitStatus)
{
    if (m_stop)
        return;

    QString errMsg;
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        errMsg = tr("Could not execute mount request.");
        break;
    case SshRemoteProcess::KilledBySignal:
        errMsg = tr("Failure running UTFS client: %1")
            .arg(m_mountProcess->errorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        if (m_mountProcess->exitCode() != 0)
            errMsg = tr("Could not execute mount request.");
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO,
            "Impossible SshRemoteProcess exit status.");
    }

    if (!errMsg.isEmpty()) {
        if (!m_utfsClientStderr.isEmpty())
            errMsg += tr("\nstderr was: '%1'")
               .arg(QString::fromUtf8(m_utfsClientStderr));
        emit error(errMsg);
    }
}

void MaemoRemoteMounter::startUtfsServers()
{
    emit reportProgress(tr("Starting UTFS servers..."));
    for (int i = 0; i < m_mountSpecs.count(); ++i) {
        const MaemoMountSpecification &mountSpec = m_mountSpecs.at(i);
        const ProcPtr utfsServerProc(new QProcess);
        const QString port = QString::number(mountSpec.remotePort);
        const QString localSecretOpt = QLatin1String("-l");
        const QString remoteSecretOpt = QLatin1String("-r");
        const QStringList utfsServerArgs = QStringList() << localSecretOpt
            << port << remoteSecretOpt << port << QLatin1String("-c")
            << (m_devConfig.server.host + QLatin1Char(':') + port)
            << mountSpec.localDir;
        utfsServerProc->start(utfsServer(), utfsServerArgs);
        if (!utfsServerProc->waitForStarted()) {
            const QByteArray &errorOutput
                = utfsServerProc->readAllStandardError();
            QString errorMsg = tr("Could not start UTFS server: %1")
                .arg(utfsServerProc->errorString());
            if (!errorOutput.isEmpty()) {
                errorMsg += tr("\nstderr output was: '%1'")
                    .arg(QString::fromLocal8Bit(errorOutput));
            }
            emit error(errorMsg);
            return;
        }
        m_utfsServers << utfsServerProc;
    }

    emit mounted();
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

} // namespace Internal
} // namespace Qt4ProjectManager
