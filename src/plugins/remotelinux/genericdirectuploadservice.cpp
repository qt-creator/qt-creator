/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "genericdirectuploadservice.h"

#include "deployablefile.h"

#include <utils/qtcassert.h>
#include <ssh/sftpchannel.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QString>

using namespace QSsh;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, InitializingSftp, Uploading };
} // anonymous namespace

class GenericDirectUploadServicePrivate
{
public:
    GenericDirectUploadServicePrivate()
        : incremental(false), stopRequested(false), state(Inactive) {}

    bool incremental;
    bool stopRequested;
    State state;
    QList<DeployableFile> filesToUpload;
    SftpChannel::Ptr uploader;
    SshRemoteProcess::Ptr mkdirProc;
    SshRemoteProcess::Ptr lnProc;
    QList<DeployableFile> deployableFiles;
};

} // namespace Internal

using namespace Internal;

GenericDirectUploadService::GenericDirectUploadService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent), d(new GenericDirectUploadServicePrivate)
{
}

GenericDirectUploadService::~GenericDirectUploadService()
{
    delete d;
}

void GenericDirectUploadService::setDeployableFiles(const QList<DeployableFile> &deployableFiles)
{
    d->deployableFiles = deployableFiles;
}

void GenericDirectUploadService::setIncrementalDeployment(bool incremental)
{
    d->incremental = incremental;
}

bool GenericDirectUploadService::isDeploymentNecessary() const
{
    d->filesToUpload.clear();
    for (int i = 0; i < d->deployableFiles.count(); ++i)
        checkDeploymentNeeded(d->deployableFiles.at(i));
    return !d->filesToUpload.isEmpty();
}

void GenericDirectUploadService::doDeviceSetup()
{
    QTC_ASSERT(d->state == Inactive, return);

    handleDeviceSetupDone(true);
}

void GenericDirectUploadService::stopDeviceSetup()
{
    QTC_ASSERT(d->state == Inactive, return);

    handleDeviceSetupDone(false);
}

void GenericDirectUploadService::doDeploy()
{
    QTC_ASSERT(d->state == Inactive, setFinished(); return);

    d->uploader = connection()->createSftpChannel();
    connect(d->uploader.data(), SIGNAL(initialized()), SLOT(handleSftpInitialized()));
    connect(d->uploader.data(), SIGNAL(initializationFailed(QString)),
        SLOT(handleSftpInitializationFailed(QString)));
    d->uploader->initialize();
    d->state = InitializingSftp;
}

void GenericDirectUploadService::handleSftpInitialized()
{
    QTC_ASSERT(d->state == InitializingSftp, setFinished(); return);

    if (d->stopRequested) {
        setFinished();
        handleDeploymentDone();
        return;
    }

    Q_ASSERT(!d->filesToUpload.isEmpty());
    connect(d->uploader.data(), SIGNAL(finished(QSsh::SftpJobId,QString)),
        SLOT(handleUploadFinished(QSsh::SftpJobId,QString)));
    d->state = Uploading;
    uploadNextFile();
}

void GenericDirectUploadService::handleSftpInitializationFailed(const QString &message)
{
    QTC_ASSERT(d->state == InitializingSftp, setFinished(); return);

    emit errorMessage(tr("SFTP initialization failed: %1").arg(message));
    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::handleUploadFinished(QSsh::SftpJobId jobId, const QString &errorMsg)
{
    Q_UNUSED(jobId);

    QTC_ASSERT(d->state == Uploading, setFinished(); return);

    if (d->stopRequested) {
        setFinished();
        handleDeploymentDone();
    }

    const DeployableFile df = d->filesToUpload.takeFirst();
    if (!errorMsg.isEmpty()) {
        QString errorString = tr("Upload of file '%1' failed. The server said: '%2'.")
            .arg(QDir::toNativeSeparators(df.localFilePath), errorMsg);
        if (errorMsg == QLatin1String("Failure") && df.remoteDir.contains(QLatin1String("/bin"))) {
            errorString += QLatin1Char(' ') + tr("If '%1' is currently running "
                "on the remote host, you might need to stop it first.").arg(df.remoteFilePath());
        }
        emit errorMessage(errorString);
        setFinished();
        handleDeploymentDone();
    } else {
        saveDeploymentTimeStamp(df);

        // Terrible hack for Windows.
        if (df.remoteDir.contains(QLatin1String("bin"))) {
            const QString command = QLatin1String("chmod a+x ") + df.remoteFilePath();
            connection()->createRemoteProcess(command.toUtf8())->start();
        }

        uploadNextFile();
    }
}

void GenericDirectUploadService::handleLnFinished(int exitStatus)
{
    QTC_ASSERT(d->state == Uploading, setFinished(); return);

    if (d->stopRequested) {
        setFinished();
        handleDeploymentDone();
    }

    const DeployableFile df = d->filesToUpload.takeFirst();
    const QString nativePath = QDir::toNativeSeparators(df.localFilePath);
    if (exitStatus != SshRemoteProcess::NormalExit || d->lnProc->exitCode() != 0) {
        emit errorMessage(tr("Failed to upload file '%1'.").arg(nativePath));
        setFinished();
        handleDeploymentDone();
        return;
    } else {
        saveDeploymentTimeStamp(df);
        uploadNextFile();
    }
}

void GenericDirectUploadService::handleMkdirFinished(int exitStatus)
{
    QTC_ASSERT(d->state == Uploading, setFinished(); return);

    if (d->stopRequested) {
        setFinished();
        handleDeploymentDone();
    }

    const DeployableFile &df = d->filesToUpload.first();
    QFileInfo fi(df.localFilePath);
    const QString nativePath = QDir::toNativeSeparators(df.localFilePath);
    if (exitStatus != SshRemoteProcess::NormalExit || d->mkdirProc->exitCode() != 0) {
        emit errorMessage(tr("Failed to upload file '%1'.").arg(nativePath));
        setFinished();
        handleDeploymentDone();
    } else if (fi.isDir()) {
        saveDeploymentTimeStamp(df);
        d->filesToUpload.removeFirst();
        uploadNextFile();
    } else {
        const QString remoteFilePath = df.remoteDir + QLatin1Char('/')  + fi.fileName();
        if (fi.isSymLink()) {
             const QString target = fi.dir().relativeFilePath(fi.symLinkTarget()); // see QTBUG-5817.
             const QString command = QLatin1String("ln -sf ") + target + QLatin1Char(' ')
                 + remoteFilePath;

             // See comment in SftpChannel::createLink as to why we can't use it.
             d->lnProc = connection()->createRemoteProcess(command.toUtf8());
             connect(d->lnProc.data(), SIGNAL(closed(int)), SLOT(handleLnFinished(int)));
             connect(d->lnProc.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleStdOutData()));
             connect(d->lnProc.data(), SIGNAL(readyReadStandardError()), SLOT(handleStdErrData()));
             d->lnProc->start();
        } else {
            const SftpJobId job = d->uploader->uploadFile(df.localFilePath, remoteFilePath,
                SftpOverwriteExisting);
            if (job == SftpInvalidJob) {
                emit errorMessage(tr("Failed to upload file '%1': "
                    "Could not open for reading.").arg(nativePath));
                setFinished();
                handleDeploymentDone();
            }
        }
    }
}

void GenericDirectUploadService::handleStdOutData()
{
    SshRemoteProcess * const process = qobject_cast<SshRemoteProcess *>(sender());
    if (process)
        emit stdOutData(QString::fromUtf8(process->readAllStandardOutput()));
}

void GenericDirectUploadService::handleStdErrData()
{
    SshRemoteProcess * const process = qobject_cast<SshRemoteProcess *>(sender());
    if (process)
        emit stdErrData(QString::fromUtf8(process->readAllStandardError()));
}

void GenericDirectUploadService::stopDeployment()
{
    QTC_ASSERT(d->state == InitializingSftp || d->state == Uploading, setFinished(); return);

    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::checkDeploymentNeeded(const DeployableFile &deployable) const
{
    QFileInfo fileInfo(deployable.localFilePath);
    if (fileInfo.isDir()) {
        const QStringList files = QDir(deployable.localFilePath)
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (files.isEmpty() && (!d->incremental || hasChangedSinceLastDeployment(deployable)))
            d->filesToUpload << deployable;
        foreach (const QString &fileName, files) {
            const QString localFilePath = deployable.localFilePath
                + QLatin1Char('/') + fileName;
            const QString remoteDir = deployable.remoteDir + QLatin1Char('/')
                + fileInfo.fileName();
            checkDeploymentNeeded(DeployableFile(localFilePath, remoteDir));
        }
    } else  if (!d->incremental || hasChangedSinceLastDeployment(deployable)) {
        d->filesToUpload << deployable;
    }
}

void GenericDirectUploadService::setFinished()
{
    d->stopRequested = false;
    d->state = Inactive;
    if (d->mkdirProc)
        disconnect(d->mkdirProc.data(), 0, this, 0);
    if (d->lnProc)
        disconnect(d->lnProc.data(), 0, this, 0);
    if (d->uploader) {
        disconnect(d->uploader.data(), 0, this, 0);
        d->uploader->closeChannel();
    }
}

void GenericDirectUploadService::uploadNextFile()
{
    if (d->filesToUpload.isEmpty()) {
        emit progressMessage(tr("All files successfully deployed."));
        setFinished();
        handleDeploymentDone();
        return;
    }

    const DeployableFile &df = d->filesToUpload.first();
    QString dirToCreate = df.remoteDir;
    if (dirToCreate.isEmpty()) {
        emit warningMessage(tr("Warning: No remote path set for local file '%1'. Skipping upload.")
            .arg(QDir::toNativeSeparators(df.localFilePath)));
        d->filesToUpload.takeFirst();
        uploadNextFile();
        return;
    }

    QFileInfo fi(df.localFilePath);
    if (fi.isDir())
        dirToCreate += QLatin1Char('/') + fi.fileName();
    const QString command = QLatin1String("mkdir -p ") + dirToCreate;
    d->mkdirProc = connection()->createRemoteProcess(command.toUtf8());
    connect(d->mkdirProc.data(), SIGNAL(closed(int)), SLOT(handleMkdirFinished(int)));
    connect(d->mkdirProc.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleStdOutData()));
    connect(d->mkdirProc.data(), SIGNAL(readyReadStandardError()), SLOT(handleStdErrData()));
    emit progressMessage(tr("Uploading file '%1'...")
        .arg(QDir::toNativeSeparators(df.localFilePath)));
    d->mkdirProc->start();
}

} //namespace RemoteLinux
