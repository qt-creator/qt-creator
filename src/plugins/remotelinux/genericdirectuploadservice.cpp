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
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <ssh/sftptransfer.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QQueue>
#include <QString>

using namespace ProjectExplorer;
using namespace QSsh;

namespace RemoteLinux {
namespace Internal {

enum State { Inactive, PreChecking, Uploading, PostProcessing };

const int MaxConcurrentStatCalls = 10;

class GenericDirectUploadServicePrivate
{
public:
    DeployableFile getFileForProcess(SshRemoteProcess *proc)
    {
        const auto it = remoteProcs.find(proc);
        QTC_ASSERT(it != remoteProcs.end(), return DeployableFile());
        const DeployableFile file = *it;
        remoteProcs.erase(it);
        return file;
    }

    IncrementalDeployment incremental = IncrementalDeployment::NotSupported;
    bool ignoreMissingFiles = false;
    QHash<SshRemoteProcess *, DeployableFile> remoteProcs;
    QQueue<DeployableFile> filesToStat;
    State state = Inactive;
    QList<DeployableFile> filesToUpload;
    SftpTransferPtr uploader;
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

void GenericDirectUploadService::setIncrementalDeployment(IncrementalDeployment incremental)
{
    d->incremental = incremental;
}

void GenericDirectUploadService::setIgnoreMissingFiles(bool ignoreMissingFiles)
{
    d->ignoreMissingFiles = ignoreMissingFiles;
}

bool GenericDirectUploadService::isDeploymentNecessary() const
{
    QTC_ASSERT(d->filesToUpload.isEmpty(), d->filesToUpload.clear());
    QList<DeployableFile> collected;
    for (int i = 0; i < d->deployableFiles.count(); ++i)
        collected.append(collectFilesToUpload(d->deployableFiles.at(i)));

    QTC_CHECK(collected.size() >= d->deployableFiles.size());
    d->deployableFiles = collected;
    return !d->deployableFiles.isEmpty();
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
    d->state = PreChecking;
    queryFiles();
}

QDateTime GenericDirectUploadService::timestampFromStat(const DeployableFile &file,
                                                        SshRemoteProcess *statProc,
                                                        const QString &errorMsg)
{
    QString errorDetails;
    if (!errorMsg.isEmpty())
        errorDetails = errorMsg;
    else if (statProc->exitCode() != 0)
        errorDetails = QString::fromUtf8(statProc->readAllStandardError());
    if (!errorDetails.isEmpty()) {
        emit warningMessage(tr("Failed to retrieve remote timestamp for file \"%1\". "
                               "Incremental deployment will not work. Error message was: %2")
                            .arg(file.remoteFilePath(), errorDetails));
        return QDateTime();
    }
    QByteArray output = statProc->readAllStandardOutput().trimmed();
    const QString warningString(tr("Unexpected stat output for remote file \"%1\": %2")
                                .arg(file.remoteFilePath()).arg(QString::fromUtf8(output)));
    if (!output.startsWith(file.remoteFilePath().toUtf8())) {
        emit warningMessage(warningString);
        return QDateTime();
    }
    const QByteArrayList columns = output.mid(file.remoteFilePath().toUtf8().size() + 1).split(' ');
    if (columns.size() < 14) { // Normal Linux stat: 16 columns in total, busybox stat: 15 columns
        emit warningMessage(warningString);
        return QDateTime();
    }
    bool isNumber;
    const qint64 secsSinceEpoch = columns.at(11).toLongLong(&isNumber);
    if (!isNumber) {
        emit warningMessage(warningString);
        return QDateTime();
    }
    return QDateTime::fromSecsSinceEpoch(secsSinceEpoch);
}

void GenericDirectUploadService::checkForStateChangeOnRemoteProcFinished()
{
    if (d->remoteProcs.size() < MaxConcurrentStatCalls && !d->filesToStat.isEmpty())
        runStat(d->filesToStat.dequeue());
    if (!d->remoteProcs.isEmpty())
        return;
    if (d->state == PreChecking) {
        uploadFiles();
        return;
    }
    QTC_ASSERT(d->state == PostProcessing, return);
    emit progressMessage(tr("All files successfully deployed."));
    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::stopDeployment()
{
    QTC_ASSERT(d->state != Inactive, return);

    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::runStat(const DeployableFile &file)
{
    // We'd like to use --format=%Y, but it's not supported by busybox.
    const QString statCmd = "stat -t " + Utils::QtcProcess::quoteArgUnix(file.remoteFilePath());
    SshRemoteProcess * const statProc = connection()->createRemoteProcess(statCmd).release();
    statProc->setParent(this);
    connect(statProc, &SshRemoteProcess::done, this,
            [this, statProc, state = d->state](const QString &errorMsg) {
        QTC_ASSERT(d->state == state, return);
        const DeployableFile file = d->getFileForProcess(statProc);
        QTC_ASSERT(file.isValid(), return);
        const QDateTime timestamp = timestampFromStat(file, statProc, errorMsg);
        statProc->deleteLater();
        switch (state) {
        case PreChecking:
            if (!timestamp.isValid() || hasRemoteFileChanged(file, timestamp))
                d->filesToUpload.append(file);
            break;
        case PostProcessing:
            if (timestamp.isValid())
                saveDeploymentTimeStamp(file, timestamp);
            break;
        case Inactive:
        case Uploading:
            QTC_CHECK(false);
            break;
        }
        checkForStateChangeOnRemoteProcFinished();
    });
    d->remoteProcs.insert(statProc, file);
    statProc->start();
}

QList<DeployableFile> GenericDirectUploadService::collectFilesToUpload(
        const DeployableFile &deployable) const
{
    QList<DeployableFile> collected;
    QFileInfo fileInfo = deployable.localFilePath().toFileInfo();
    if (fileInfo.isDir()) {
        const QStringList files = QDir(deployable.localFilePath().toString())
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &fileName : files) {
            const QString localFilePath = deployable.localFilePath().toString()
                + QLatin1Char('/') + fileName;
            const QString remoteDir = deployable.remoteDirectory() + QLatin1Char('/')
                + fileInfo.fileName();
            collected.append(collectFilesToUpload(DeployableFile(localFilePath, remoteDir)));
        }
    } else {
        collected << deployable;
    }
    return collected;
}

void GenericDirectUploadService::setFinished()
{
    d->state = Inactive;
    d->filesToStat.clear();
    for (auto it = d->remoteProcs.begin(); it != d->remoteProcs.end(); ++it) {
        it.key()->disconnect();
        it.key()->terminate();
    }
    d->remoteProcs.clear();
    if (d->uploader) {
        d->uploader->disconnect();
        d->uploader->stop();
        d->uploader.release()->deleteLater();
    }
    d->filesToUpload.clear();
}

void GenericDirectUploadService::queryFiles()
{
    QTC_ASSERT(d->state == PreChecking || d->state == PostProcessing, return);
    QTC_ASSERT(d->state == PostProcessing || d->remoteProcs.isEmpty(), return);

    const QList<DeployableFile> &filesToCheck = d->state == PreChecking
            ? d->deployableFiles : d->filesToUpload;
    for (const DeployableFile &file : filesToCheck) {
        if (d->state == PreChecking && (d->incremental != IncrementalDeployment::Enabled
                                        || hasLocalFileChanged(file))) {
            d->filesToUpload.append(file);
            continue;
        }
        if (d->incremental == IncrementalDeployment::NotSupported)
            continue;
        if (d->remoteProcs.size() >= MaxConcurrentStatCalls)
            d->filesToStat << file;
        else
            runStat(file);
    }
    checkForStateChangeOnRemoteProcFinished();
}

void GenericDirectUploadService::uploadFiles()
{
    QTC_ASSERT(d->state == PreChecking, return);
    d->state = Uploading;
    if (d->filesToUpload.empty()) {
        emit progressMessage(tr("No files need to be uploaded."));
        setFinished();
        handleDeploymentDone();
        return;
    }
    emit progressMessage(tr("%n file(s) need to be uploaded.", "", d->filesToUpload.size()));
    FilesToTransfer filesToTransfer;
    for (const DeployableFile &f : d->filesToUpload) {
        if (!f.localFilePath().exists()) {
            const QString message = tr("Local file \"%1\" does not exist.")
                    .arg(f.localFilePath().toUserOutput());
            if (d->ignoreMissingFiles) {
                emit warningMessage(message);
                continue;
            } else {
                emit errorMessage(message);
                setFinished();
                handleDeploymentDone();
                return;
            }
        }
        filesToTransfer << FileToTransfer(f.localFilePath().toString(), f.remoteFilePath());
    }
    d->uploader = connection()->createUpload(filesToTransfer, FileTransferErrorHandling::Abort);
    connect(d->uploader.get(), &SftpTransfer::done, [this](const QString &error) {
        QTC_ASSERT(d->state == Uploading, return);
        if (!error.isEmpty()) {
            emit errorMessage(error);
            setFinished();
            handleDeploymentDone();
            return;
        }
        d->state = PostProcessing;
        chmod();
        queryFiles();
    });
    connect(d->uploader.get(), &SftpTransfer::progress,
            this, &GenericDirectUploadService::progressMessage);
    d->uploader->start();
}

void GenericDirectUploadService::chmod()
{
    QTC_ASSERT(d->state == PostProcessing, return);
    if (!Utils::HostOsInfo::isWindowsHost())
        return;
    for (const DeployableFile &f : d->filesToUpload) {
        if (!f.isExecutable())
            continue;
        const QString command = QLatin1String("chmod a+x ")
                + Utils::QtcProcess::quoteArgUnix(f.remoteFilePath());
        SshRemoteProcess * const chmodProc
                = connection()->createRemoteProcess(command).release();
        chmodProc->setParent(this);
        connect(chmodProc, &SshRemoteProcess::done, this,
                [this, chmodProc, state = d->state](const QString &error) {
            QTC_ASSERT(state == d->state, return);
            const DeployableFile file = d->getFileForProcess(chmodProc);
            QTC_ASSERT(file.isValid(), return);
            if (!error.isEmpty()) {
                emit warningMessage(tr("Remote chmod failed for file \"%1\": %2")
                                    .arg(file.remoteFilePath(), error));
            } else if (chmodProc->exitCode() != 0) {
                emit warningMessage(tr("Remote chmod failed for file \"%1\": %2")
                                    .arg(file.remoteFilePath(),
                                         QString::fromUtf8(chmodProc->readAllStandardError())));
            }
            chmodProc->deleteLater();
            checkForStateChangeOnRemoteProcFinished();
        });
        d->remoteProcs.insert(chmodProc, f);
        chmodProc->start();
    }
}

} //namespace RemoteLinux
