/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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
#include "genericdirectuploadservice.h"

#include "deployablefile.h"

#include <utils/qtcassert.h>
#include <utils/ssh/sftpchannel.h>
#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshremoteprocess.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QString>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, InitializingSftp, Uploading };
} // anonymous namespace

class GenericDirectUploadServicePrivate
{
public:
    GenericDirectUploadServicePrivate() : stopRequested(false), state(Inactive) {}

    bool stopRequested;
    State state;
    QList<DeployableFile> filesToUpload;
    SftpChannel::Ptr uploader;
    SshRemoteProcess::Ptr mkdirProc;
    QList<DeployableFile> deployableFiles;
};

} // namespace Internal

using namespace Internal;

GenericDirectUploadService::GenericDirectUploadService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent), m_d(new GenericDirectUploadServicePrivate)
{
}

void GenericDirectUploadService::setDeployableFiles(const QList<DeployableFile> &deployableFiles)
{
    m_d->deployableFiles = deployableFiles;
}

bool GenericDirectUploadService::isDeploymentNecessary() const
{
    m_d->filesToUpload.clear();
    for (int i = 0; i < m_d->deployableFiles.count(); ++i)
        checkDeploymentNeeded(m_d->deployableFiles.at(i));
    return !m_d->filesToUpload.isEmpty();
}

void GenericDirectUploadService::doDeviceSetup()
{
    QTC_ASSERT(m_d->state == Inactive, return);

    handleDeviceSetupDone(true);
}

void GenericDirectUploadService::stopDeviceSetup()
{
    QTC_ASSERT(m_d->state == Inactive, return);

    handleDeviceSetupDone(false);
}

void GenericDirectUploadService::doDeploy()
{
    QTC_ASSERT(m_d->state == Inactive, setFinished(); return);

    m_d->uploader = connection()->createSftpChannel();
    connect(m_d->uploader.data(), SIGNAL(initialized()), SLOT(handleSftpInitialized()));
    connect(m_d->uploader.data(), SIGNAL(initializationFailed(QString)),
        SLOT(handleSftpInitializationFailed(QString)));
    m_d->uploader->initialize();
    m_d->state = InitializingSftp;
}

void GenericDirectUploadService::handleSftpInitialized()
{
    QTC_ASSERT(m_d->state == InitializingSftp, setFinished(); return);

    if (m_d->stopRequested) {
        setFinished();
        handleDeploymentDone();
        return;
    }

    Q_ASSERT(!m_d->filesToUpload.isEmpty());
    connect(m_d->uploader.data(), SIGNAL(finished(Utils::SftpJobId, QString)),
        SLOT(handleUploadFinished(Utils::SftpJobId,QString)));
    m_d->state = Uploading;
    uploadNextFile();
}

void GenericDirectUploadService::handleSftpInitializationFailed(const QString &message)
{
    QTC_ASSERT(m_d->state == InitializingSftp, setFinished(); return);

    emit errorMessage(tr("SFTP initialization failed: %1").arg(message));
    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::handleUploadFinished(Utils::SftpJobId jobId, const QString &errorMsg)
{
    Q_UNUSED(jobId);

    QTC_ASSERT(m_d->state == Uploading, setFinished(); return);

    if (m_d->stopRequested) {
        setFinished();
        handleDeploymentDone();
    }

    const DeployableFile d = m_d->filesToUpload.takeFirst();
    if (!errorMsg.isEmpty()) {
        emit errorMessage(tr("Upload of file '%1' failed: %2")
            .arg(QDir::toNativeSeparators(d.localFilePath), errorMsg));
        setFinished();
        handleDeploymentDone();
    } else {
        saveDeploymentTimeStamp(d);
        uploadNextFile();
    }
}

void GenericDirectUploadService::handleMkdirFinished(int exitStatus)
{
    QTC_ASSERT(m_d->state == Uploading, setFinished(); return);

    if (m_d->stopRequested) {
        setFinished();
        handleDeploymentDone();
    }

    const DeployableFile &d = m_d->filesToUpload.first();
    QFileInfo fi(d.localFilePath);
    const QString nativePath = QDir::toNativeSeparators(d.localFilePath);
    if (exitStatus != SshRemoteProcess::ExitedNormally || m_d->mkdirProc->exitCode() != 0) {
        emit errorMessage(tr("Failed to upload file '%1'.").arg(nativePath));
        setFinished();
        handleDeploymentDone();
    } else if (fi.isDir()) {
        saveDeploymentTimeStamp(d);
        m_d->filesToUpload.removeFirst();
        uploadNextFile();
    } else {
        const SftpJobId job = m_d->uploader->uploadFile(d.localFilePath,
            d.remoteDir + QLatin1Char('/')  + fi.fileName(),
            SftpOverwriteExisting);
        if (job == SftpInvalidJob) {
            emit errorMessage(tr("Failed to upload file '%1': "
                "Could not open for reading.").arg(nativePath));
            setFinished();
            handleDeploymentDone();
        }
    }
}

void GenericDirectUploadService::stopDeployment()
{
    QTC_ASSERT(m_d->state == InitializingSftp || m_d->state == Uploading, setFinished(); return);

    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::checkDeploymentNeeded(const DeployableFile &deployable) const
{
    QFileInfo fileInfo(deployable.localFilePath);
    if (fileInfo.isDir()) {
        const QStringList files = QDir(deployable.localFilePath)
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (files.isEmpty() && hasChangedSinceLastDeployment(deployable))
            m_d->filesToUpload << deployable;
        foreach (const QString &fileName, files) {
            const QString localFilePath = deployable.localFilePath
                + QLatin1Char('/') + fileName;
            const QString remoteDir = deployable.remoteDir + QLatin1Char('/')
                + fileInfo.fileName();
            checkDeploymentNeeded(DeployableFile(localFilePath, remoteDir));
        }
    } else if (hasChangedSinceLastDeployment(deployable)) {
        m_d->filesToUpload << deployable;
    }
}

void GenericDirectUploadService::setFinished()
{
    m_d->stopRequested = false;
    m_d->state = Inactive;
    if (m_d->mkdirProc) {
        disconnect(m_d->mkdirProc.data(), 0, this, 0);
    }
    if (m_d->uploader) {
        disconnect(m_d->uploader.data(), 0, this, 0);
        m_d->uploader->closeChannel();
    }
}

void GenericDirectUploadService::uploadNextFile()
{
    if (m_d->filesToUpload.isEmpty()) {
        emit progressMessage(tr("All files successfully deployed."));
        setFinished();
        handleDeploymentDone();
        return;
    }

    const DeployableFile &d = m_d->filesToUpload.first();
    QString dirToCreate = d.remoteDir;
    QFileInfo fi(d.localFilePath);
    if (fi.isDir())
        dirToCreate += QLatin1Char('/') + fi.fileName();
    const QByteArray command = "mkdir -p " + dirToCreate.toUtf8();
    m_d->mkdirProc = connection()->createRemoteProcess(command);
    connect(m_d->mkdirProc.data(), SIGNAL(closed(int)), SLOT(handleMkdirFinished(int)));
    // TODO: Connect stderr.
    emit progressMessage(tr("Uploading file '%1'...")
        .arg(QDir::toNativeSeparators(d.localFilePath)));
    m_d->mkdirProc->start();
}

} //namespace RemoteLinux
