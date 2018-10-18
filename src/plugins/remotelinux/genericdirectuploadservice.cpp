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
#include <QDateTime>
#include <QHash>

using namespace ProjectExplorer;
using namespace QSsh;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, InitializingSftp, Uploading };
} // anonymous namespace

enum class JobType {
    PreQuery,
    Upload,
    Mkdir,
    Ln,
    Chmod,
    PostQuery,
    None
};

struct Job
{
    DeployableFile file;
    JobType type;
    QDateTime result;
    explicit Job(const DeployableFile &file = DeployableFile(), JobType type = JobType::None,
                 const QDateTime &result = QDateTime())
        : file(file), type(type), result(result) {}
};

class GenericDirectUploadServicePrivate
{
public:
    GenericDirectUploadServicePrivate()
        : incremental(false), ignoreMissingFiles(false), state(Inactive) {}

    bool incremental;
    bool ignoreMissingFiles;
    bool uploadJobRunning = false;
    State state;

    QList<DeployableFile> filesToUpload;

    QHash<SftpJobId, Job> runningJobs;

    SshRemoteProcess::Ptr runningProc;
    DeployableFile runningProcFile;

    SftpChannel::Ptr uploader;
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
    QTC_ASSERT(!d->deployableFiles.isEmpty(), setFinished(); return);
    connect(d->uploader.data(), &SftpChannel::finished,
            this, &GenericDirectUploadService::handleJobFinished);
    connect(d->uploader.data(), &SftpChannel::fileInfoAvailable,
            this, &GenericDirectUploadService::handleFileInfoAvailable);
    d->state = Uploading;
    queryFiles();
}

void GenericDirectUploadService::handleSftpChannelError(const QString &message)
{
    QTC_ASSERT(d->state == InitializingSftp, setFinished(); return);

    emit errorMessage(tr("SFTP initialization failed: %1").arg(message));
    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::handleFileInfoAvailable(SftpJobId jobId,
                                                         const QList<SftpFileInfo> &fileInfos)
{
    QTC_ASSERT(d->state == Uploading, return);
    QTC_ASSERT(fileInfos.length() == 1, return);
    auto it = d->runningJobs.find(jobId);
    QTC_ASSERT(it != d->runningJobs.end(), return);
    it->result = QDateTime::fromTime_t(fileInfos.at(0).mtime);
}

void GenericDirectUploadService::handleJobFinished(SftpJobId jobId, const QString &errorMsg)
{
    auto it = d->runningJobs.find(jobId);
    QTC_ASSERT(it != d->runningJobs.end(), return);

    Job job = *it;
    d->runningJobs.erase(it);

    switch (job.type) {
    case JobType::PreQuery:
        if (hasRemoteFileChanged(job.file, job.result)) {
            d->filesToUpload.append(job.file);
            if (!d->uploadJobRunning)
                uploadNextFile();
        } else {
            tryFinish();
        }
        break;
    case JobType::Upload:
        QTC_CHECK(d->uploadJobRunning);

        if (!errorMsg.isEmpty()) {
            QString errorString = tr("Upload of file \"%1\" failed. The server said: \"%2\".")
                    .arg(job.file.localFilePath().toUserOutput(), errorMsg);
            if (errorMsg == QLatin1String("Failure")
                    && job.file.remoteDirectory().contains(QLatin1String("/bin"))) {
                errorString += QLatin1Char(' ')
                        + tr("If \"%1\" is currently running on the remote host, "
                             "you might need to stop it first.").arg(job.file.remoteFilePath());
            }
            emit errorMessage(errorString);
            setFinished();
            handleDeploymentDone();
        }

        // This is done for Windows.
        if (job.file.isExecutable()) {
            const QString command = QLatin1String("chmod a+x ")
                    + Utils::QtcProcess::quoteArgUnix(job.file.remoteFilePath());
            d->runningProc = connection()->createRemoteProcess(command.toUtf8());
            d->runningProcFile = job.file;
            connect(d->runningProc.data(), &SshRemoteProcess::closed,
                    this, &GenericDirectUploadService::handleUploadProcFinished);
            connect(d->runningProc.data(), &SshRemoteProcess::readyReadStandardOutput,
                    this, &GenericDirectUploadService::handleStdOutData);
            connect(d->runningProc.data(), &SshRemoteProcess::readyReadStandardError,
                    this, &GenericDirectUploadService::handleStdErrData);
            connect(d->runningProc.data(), &SshRemoteProcess::readChannelFinished,
                    this, &GenericDirectUploadService::handleReadChannelFinished);
            d->runningProc->start();
        } else {
            d->uploadJobRunning = false;
            const SftpJobId jobId = d->uploader->statFile(job.file.remoteFilePath());
            if (jobId == SftpInvalidJob) {
                emit errorMessage(tr("SFTP stat query for %1 failed.")
                                  .arg(job.file.remoteFilePath()));
                saveDeploymentTimeStamp(job.file, QDateTime());
            } else {
                d->runningJobs.insert(jobId, Job(job.file, JobType::PostQuery));
            }
            uploadNextFile();
        }
        break;
    case JobType::PostQuery:
        if (!errorMsg.isEmpty()) {
            emit warningMessage(tr("Could not determine remote timestamp of %1: %2")
                .arg(job.file.remoteFilePath()).arg(errorMsg));
        }
        saveDeploymentTimeStamp(job.file, job.result);
        tryFinish();
        break;
    default:
        QTC_CHECK(false);
        break;
    }
}

void GenericDirectUploadService::clearRunningProc()
{
    d->runningProc.clear();
    d->runningProcFile = DeployableFile();
    d->uploadJobRunning = false;
}

void GenericDirectUploadService::handleUploadProcFinished(int exitStatus)
{
    QTC_ASSERT(d->state == Uploading, setFinished(); return);
    QTC_ASSERT(d->uploadJobRunning, return);

    if (exitStatus != SshRemoteProcess::NormalExit || d->runningProc->exitCode() != 0)
        handleProcFailure();
    else
        runPostQueryOnProcResult();
}

void GenericDirectUploadService::handleProcFailure()
{
    emit errorMessage(tr("Failed to upload file \"%1\".")
                      .arg(d->runningProcFile.localFilePath().toUserOutput()));
    clearRunningProc();
    setFinished();
    handleDeploymentDone();
}

void GenericDirectUploadService::runPostQueryOnProcResult()
{
    const SftpJobId jobId = d->uploader->statFile(d->runningProcFile.remoteFilePath());
    if (jobId == SftpInvalidJob) {
        emit errorMessage(tr("SFTP stat query for %1 failed.")
                          .arg(d->runningProcFile.remoteFilePath()));
        saveDeploymentTimeStamp(d->runningProcFile, QDateTime());
    } else {
        d->runningJobs.insert(jobId, Job(d->runningProcFile, JobType::PostQuery));
    }
    clearRunningProc();
    uploadNextFile();
}

void GenericDirectUploadService::tryFinish()
{
    if (d->filesToUpload.isEmpty() && d->runningJobs.isEmpty() && d->runningProc.isNull()) {
        emit progressMessage(tr("All files successfully deployed."));
        setFinished();
        handleDeploymentDone();
    }
}

void GenericDirectUploadService::handleMkdirFinished(int exitStatus)
{
    QTC_ASSERT(d->state == Uploading, setFinished(); return);

    QFileInfo fi = d->runningProcFile.localFilePath().toFileInfo();
    if (exitStatus != SshRemoteProcess::NormalExit || d->runningProc->exitCode() != 0) {
        handleProcFailure();
    } else if (fi.isDir()) {
        runPostQueryOnProcResult();
    } else {
        const QString remoteFilePath = d->runningProcFile.remoteFilePath();
        if (fi.isSymLink()) {
             const QString target = fi.dir().relativeFilePath(fi.symLinkTarget()); // see QTBUG-5817.
             const QStringList args = QStringList() << QLatin1String("ln") << QLatin1String("-sf")
                                                    << target << remoteFilePath;
             const QString command = Utils::QtcProcess::joinArgs(args, Utils::OsTypeLinux);

             // See comment in SftpChannel::createLink as to why we can't use it.
             d->runningProc = connection()->createRemoteProcess(command.toUtf8());
             connect(d->runningProc.data(), &SshRemoteProcess::closed,
                     this, &GenericDirectUploadService::handleUploadProcFinished);
             connect(d->runningProc.data(), &SshRemoteProcess::readyReadStandardOutput,
                     this, &GenericDirectUploadService::handleStdOutData);
             connect(d->runningProc.data(), &SshRemoteProcess::readyReadStandardError,
                     this, &GenericDirectUploadService::handleStdErrData);
             connect(d->runningProc.data(), &SshRemoteProcess::readChannelFinished,
                     this, &GenericDirectUploadService::handleReadChannelFinished);
             d->runningProc->start();
        } else {
            const SftpJobId job = d->uploader->uploadFile(
                        d->runningProcFile.localFilePath().toString(), remoteFilePath,
                        SftpOverwriteExisting);
            if (job == SftpInvalidJob) {
                const QString message = tr("Failed to upload file \"%1\": "
                                           "Could not open for reading.")
                        .arg(d->runningProcFile.localFilePath().toUserOutput());
                clearRunningProc();
                if (d->ignoreMissingFiles) {
                    emit warningMessage(message);
                    uploadNextFile();
                } else {
                    emit errorMessage(message);
                    setFinished();
                    handleDeploymentDone();
                }
            } else {
                d->runningJobs[job] = Job(d->runningProcFile, JobType::Upload);
                clearRunningProc();
                d->uploadJobRunning = true;
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

QList<DeployableFile> GenericDirectUploadService::collectFilesToUpload(
        const DeployableFile &deployable) const
{
    QList<DeployableFile> collected({deployable});
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
    }
    return collected;
}

void GenericDirectUploadService::setFinished()
{
    d->state = Inactive;
    if (d->runningProc)
        disconnect(d->runningProc.data(), nullptr, this, nullptr);
    if (d->uploader) {
        disconnect(d->uploader.data(), nullptr, this, nullptr);
        d->uploader->closeChannel();
    }
    clearRunningProc();
    d->uploadJobRunning = false;
    d->runningJobs.clear();
    d->filesToUpload.clear();
}

void GenericDirectUploadService::uploadNextFile()
{
    QTC_ASSERT(!d->uploadJobRunning, return);

    if (d->filesToUpload.isEmpty()) {
        tryFinish();
        return;
    }

    const DeployableFile df = d->filesToUpload.takeFirst();

    QString dirToCreate = df.remoteDirectory();
    if (dirToCreate.isEmpty()) {
        emit warningMessage(tr("Warning: No remote path set for local file \"%1\". "
                               "Skipping upload.").arg(df.localFilePath().toUserOutput()));
        uploadNextFile();
        return;
    }

    QFileInfo fi = df.localFilePath().toFileInfo();
    if (fi.isDir())
        dirToCreate += QLatin1Char('/') + fi.fileName();
    const QString command = QLatin1String("mkdir -p ")
            + Utils::QtcProcess::quoteArgUnix(dirToCreate);
    QTC_CHECK(d->runningProc.isNull());
    d->runningProc = connection()->createRemoteProcess(command.toUtf8());
    connect(d->runningProc.data(), &SshRemoteProcess::closed,
            this, &GenericDirectUploadService::handleMkdirFinished);
    connect(d->runningProc.data(), &SshRemoteProcess::readyReadStandardOutput,
            this, &GenericDirectUploadService::handleStdOutData);
    connect(d->runningProc.data(), &SshRemoteProcess::readyReadStandardError,
            this, &GenericDirectUploadService::handleStdErrData);
    connect(d->runningProc.data(), &SshRemoteProcess::readChannelFinished,
            this, &GenericDirectUploadService::handleReadChannelFinished);
    emit progressMessage(tr("Uploading file \"%1\"...")
                         .arg(df.localFilePath().toUserOutput()));
    d->runningProcFile = df;
    d->runningProc->start();
    d->uploadJobRunning = true;
}

void GenericDirectUploadService::queryFiles()
{
    QTC_ASSERT(d->state == Uploading, return);

    for (const DeployableFile &file : d->deployableFiles) {
        if (!d->incremental || hasLocalFileChanged(file)) {
            d->filesToUpload.append(file);
            continue;
        }

        const SftpJobId jobId = d->uploader->statFile(file.remoteFilePath());
        if (jobId == SftpInvalidJob) {
            emit warningMessage(tr("SFTP stat query for %1 failed.").arg(file.remoteFilePath()));
            d->filesToUpload.append(file);
            continue;
        }

        d->runningJobs.insert(jobId, Job(file, JobType::PreQuery));
    }

    uploadNextFile();
}

} //namespace RemoteLinux
