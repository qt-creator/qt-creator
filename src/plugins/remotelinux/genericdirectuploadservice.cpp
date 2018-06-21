/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "genericdirectuploadservice.h"

#include <projectexplorer/deployablefile.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <ssh/sftpchannel.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QString>

using namespace ProjectExplorer;
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
        : incremental(false), ignoreMissingFiles(false), stopRequested(false), state(Inactive) {}

    bool incremental;
    bool ignoreMissingFiles;
    bool stopRequested;
    State state;
    QList<DeployableFile> filesToUpload;
    SftpChannel::Ptr uploader;
    SshRemoteProcess::Ptr mkdirProc;
    SshRemoteProcess::Ptr lnProc;
    SshRemoteProcess::Ptr chmodProc;
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

void GenericDirectUploadService::setIgnoreMissingFiles(bool ignoreMissingFiles)
{
    d->ignoreMissingFiles = ignoreMissingFiles;
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
    connect(d->uploader.data(), &SftpChannel::initialized,
            this, &GenericDirectUploadService::handleSftpInitialized);
    connect(d->uploader.data(), &SftpChannel::channelError,
            this, &GenericDirectUploadService::handleSftpChannelError);
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
    connect(d->uploader.data(), &SftpChannel::finished,
            this, &GenericDirectUploadService::handleUploadFinished);
    d->state = Uploading;
    uploadNextFile();
}

void GenericDirectUploadService::handleSftpChannelError(const QString &message)
{
    QTC_ASSERT(d->state == InitializingSftp, setFinished(); return);

    emit errorMessage(tr("SFTP initialization failed: %1").arg(message));
    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::handleUploadFinished(SftpJobId jobId, const QString &errorMsg)
{
    Q_UNUSED(jobId);

    QTC_ASSERT(d->state == Uploading, setFinished(); return);

    if (d->stopRequested) {
        setFinished();
        handleDeploymentDone();
    }

    const DeployableFile df = d->filesToUpload.takeFirst();
    if (!errorMsg.isEmpty()) {
        QString errorString = tr("Upload of file \"%1\" failed. The server said: \"%2\".")
            .arg(df.localFilePath().toUserOutput(), errorMsg);
        if (errorMsg == QLatin1String("Failure")
                && df.remoteDirectory().contains(QLatin1String("/bin"))) {
            errorString += QLatin1Char(' ') + tr("If \"%1\" is currently running "
                "on the remote host, you might need to stop it first.").arg(df.remoteFilePath());
        }
        emit errorMessage(errorString);
        setFinished();
        handleDeploymentDone();
    } else {
        saveDeploymentTimeStamp(df);

        // This is done for Windows.
        if (df.isExecutable()) {
            const QString command = QLatin1String("chmod a+x ")
                    + Utils::QtcProcess::quoteArgUnix(df.remoteFilePath());
            d->chmodProc = connection()->createRemoteProcess(command.toUtf8());
            connect(d->chmodProc.data(), &SshRemoteProcess::closed,
                    this, &GenericDirectUploadService::handleChmodFinished);
            connect(d->chmodProc.data(), &SshRemoteProcess::readyReadStandardOutput,
                    this, &GenericDirectUploadService::handleStdOutData);
            connect(d->chmodProc.data(), &SshRemoteProcess::readyReadStandardError,
                    this, &GenericDirectUploadService::handleStdErrData);
            connect(d->chmodProc.data(), &SshRemoteProcess::readChannelFinished,
                    this, &GenericDirectUploadService::handleReadChannelFinished);
            d->chmodProc->start();
        } else {
            uploadNextFile();
        }
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
    const QString nativePath = df.localFilePath().toUserOutput();
    if (exitStatus != SshRemoteProcess::NormalExit || d->lnProc->exitCode() != 0) {
        emit errorMessage(tr("Failed to upload file \"%1\".").arg(nativePath));
        setFinished();
        handleDeploymentDone();
        return;
    } else {
        saveDeploymentTimeStamp(df);
        uploadNextFile();
    }
}

void GenericDirectUploadService::handleChmodFinished(int exitStatus)
{
    QTC_ASSERT(d->state == Uploading, setFinished(); return);

    if (d->stopRequested) {
        setFinished();
        handleDeploymentDone();
        return;
    }

    if (exitStatus != SshRemoteProcess::NormalExit || d->chmodProc->exitCode() != 0) {
        emit errorMessage(tr("Failed to set executable flag."));
        setFinished();
        handleDeploymentDone();
        return;
    }
    uploadNextFile();
}

void GenericDirectUploadService::handleMkdirFinished(int exitStatus)
{
    QTC_ASSERT(d->state == Uploading, setFinished(); return);

    if (d->stopRequested) {
        setFinished();
        handleDeploymentDone();
    }

    const DeployableFile &df = d->filesToUpload.first();
    QFileInfo fi = df.localFilePath().toFileInfo();
    const QString nativePath = df.localFilePath().toUserOutput();
    if (exitStatus != SshRemoteProcess::NormalExit || d->mkdirProc->exitCode() != 0) {
        emit errorMessage(tr("Failed to upload file \"%1\".").arg(nativePath));
        setFinished();
        handleDeploymentDone();
    } else if (fi.isDir()) {
        saveDeploymentTimeStamp(df);
        d->filesToUpload.removeFirst();
        uploadNextFile();
    } else {
        const QString remoteFilePath = df.remoteDirectory() + QLatin1Char('/')  + fi.fileName();
        if (fi.isSymLink()) {
             const QString target = fi.dir().relativeFilePath(fi.symLinkTarget()); // see QTBUG-5817.
             const QStringList args = QStringList() << QLatin1String("ln") << QLatin1String("-sf")
                                                    << target << remoteFilePath;
             const QString command = Utils::QtcProcess::joinArgs(args, Utils::OsTypeLinux);

             // See comment in SftpChannel::createLink as to why we can't use it.
             d->lnProc = connection()->createRemoteProcess(command.toUtf8());
             connect(d->lnProc.data(), &SshRemoteProcess::closed,
                     this, &GenericDirectUploadService::handleLnFinished);
             connect(d->lnProc.data(), &SshRemoteProcess::readyReadStandardOutput,
                     this, &GenericDirectUploadService::handleStdOutData);
             connect(d->lnProc.data(), &SshRemoteProcess::readyReadStandardError,
                     this, &GenericDirectUploadService::handleStdErrData);
             connect(d->lnProc.data(), &SshRemoteProcess::readChannelFinished,
                     this, &GenericDirectUploadService::handleReadChannelFinished);
             d->lnProc->start();
        } else {
            const SftpJobId job = d->uploader->uploadFile(df.localFilePath().toString(),
                    remoteFilePath, SftpOverwriteExisting);
            if (job == SftpInvalidJob) {
                const QString message = tr("Failed to upload file \"%1\": "
                                           "Could not open for reading.").arg(nativePath);
                if (d->ignoreMissingFiles) {
                    emit warningMessage(message);
                    d->filesToUpload.removeFirst();
                    uploadNextFile();
                } else {
                    emit errorMessage(message);
                    setFinished();
                    handleDeploymentDone();
                }
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

void GenericDirectUploadService::handleReadChannelFinished()
{
    SshRemoteProcess * const process = qobject_cast<SshRemoteProcess *>(sender());
    if (process && process->atEnd())
        process->close();
}

void GenericDirectUploadService::stopDeployment()
{
    QTC_ASSERT(d->state == InitializingSftp || d->state == Uploading, setFinished(); return);

    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::checkDeploymentNeeded(const DeployableFile &deployable) const
{
    QFileInfo fileInfo = deployable.localFilePath().toFileInfo();
    if (fileInfo.isDir()) {
        const QStringList files = QDir(deployable.localFilePath().toString())
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (files.isEmpty() && (!d->incremental || hasChangedSinceLastDeployment(deployable)))
            d->filesToUpload << deployable;
        foreach (const QString &fileName, files) {
            const QString localFilePath = deployable.localFilePath().toString()
                + QLatin1Char('/') + fileName;
            const QString remoteDir = deployable.remoteDirectory() + QLatin1Char('/')
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
    QString dirToCreate = df.remoteDirectory();
    if (dirToCreate.isEmpty()) {
        emit warningMessage(tr("Warning: No remote path set for local file \"%1\". Skipping upload.")
            .arg(df.localFilePath().toUserOutput()));
        d->filesToUpload.takeFirst();
        uploadNextFile();
        return;
    }

    QFileInfo fi = df.localFilePath().toFileInfo();
    if (fi.isDir())
        dirToCreate += QLatin1Char('/') + fi.fileName();
    const QString command = QLatin1String("mkdir -p ")
            + Utils::QtcProcess::quoteArgUnix(dirToCreate);
    d->mkdirProc = connection()->createRemoteProcess(command.toUtf8());
    connect(d->mkdirProc.data(), &SshRemoteProcess::closed,
            this, &GenericDirectUploadService::handleMkdirFinished);
    connect(d->mkdirProc.data(), &SshRemoteProcess::readyReadStandardOutput,
            this, &GenericDirectUploadService::handleStdOutData);
    connect(d->mkdirProc.data(), &SshRemoteProcess::readyReadStandardError,
            this, &GenericDirectUploadService::handleStdErrData);
    connect(d->mkdirProc.data(), &SshRemoteProcess::readChannelFinished,
            this, &GenericDirectUploadService::handleReadChannelFinished);
    emit progressMessage(tr("Uploading file \"%1\"...")
        .arg(df.localFilePath().toUserOutput()));
    d->mkdirProc->start();
}

} //namespace RemoteLinux
