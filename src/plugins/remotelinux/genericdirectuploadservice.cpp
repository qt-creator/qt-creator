// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericdirectuploadservice.h"

#include "remotelinuxtr.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDateTime>
#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Utils::Tasking;

namespace RemoteLinux {
namespace Internal {

const int MaxConcurrentStatCalls = 10;

struct UploadStorage
{
    QList<DeployableFile> filesToUpload;
    FilesToTransfer filesToTransfer;
};

class GenericDirectUploadServicePrivate
{
public:
    GenericDirectUploadServicePrivate(GenericDirectUploadService *service) : q(service) {}

    QDateTime timestampFromStat(const DeployableFile &file, QtcProcess *statProc);

    using FilesToStat = std::function<QList<DeployableFile>(UploadStorage *)>;
    using StatEndHandler
          = std::function<void(UploadStorage *, const DeployableFile &, const QDateTime &)>;
    TaskItem statTask(UploadStorage *storage, const DeployableFile &file,
                      StatEndHandler statEndHandler);
    TaskItem statTree(const TreeStorage<UploadStorage> &storage, FilesToStat filesToStat,
                      StatEndHandler statEndHandler);
    TaskItem uploadGroup(const TreeStorage<UploadStorage> &storage);
    TaskItem chmodTask(const DeployableFile &file);
    TaskItem chmodTree(const TreeStorage<UploadStorage> &storage);

    GenericDirectUploadService *q = nullptr;
    IncrementalDeployment incremental = IncrementalDeployment::NotSupported;
    bool ignoreMissingFiles = false;
    QList<DeployableFile> deployableFiles;
};

QList<DeployableFile> collectFilesToUpload(const DeployableFile &deployable)
{
    QList<DeployableFile> collected;
    FilePath localFile = deployable.localFilePath();
    if (localFile.isDir()) {
        const FilePaths files = localFile.dirEntries(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        const QString remoteDir = deployable.remoteDirectory() + '/' + localFile.fileName();
        for (const FilePath &localFilePath : files)
            collected.append(collectFilesToUpload(DeployableFile(localFilePath, remoteDir)));
    } else {
        collected << deployable;
    }
    return collected;
}

static Group packIntoOptionalParallelGroups(const QList<TaskItem> &tasks)
{
    QList<TaskItem> groups;
    int i = 0;
    while (i < tasks.size()) {
        const QList<TaskItem> subTasks = tasks.mid(i, MaxConcurrentStatCalls);
        i += subTasks.size();
        groups.append(Group { QList<TaskItem> {optional, parallel} + subTasks });
    }
    return Group { QList<TaskItem> {optional} + groups };
}

} // namespace Internal

using namespace Internal;

GenericDirectUploadService::GenericDirectUploadService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent), d(new GenericDirectUploadServicePrivate(this))
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
    QList<DeployableFile> collected;
    for (int i = 0; i < d->deployableFiles.count(); ++i)
        collected.append(collectFilesToUpload(d->deployableFiles.at(i)));

    QTC_CHECK(collected.size() >= d->deployableFiles.size());
    d->deployableFiles = collected;
    return !d->deployableFiles.isEmpty();
}

QDateTime GenericDirectUploadServicePrivate::timestampFromStat(const DeployableFile &file,
                                                               QtcProcess *statProc)
{
    bool succeeded = false;
    QString error;
    if (statProc->error() == QProcess::FailedToStart) {
        error = Tr::tr("Failed to start \"stat\": %1").arg(statProc->errorString());
    } else if (statProc->exitStatus() == QProcess::CrashExit) {
        error = Tr::tr("\"stat\" crashed.");
    } else if (statProc->exitCode() != 0) {
        error = Tr::tr("\"stat\" failed with exit code %1: %2")
                .arg(statProc->exitCode()).arg(statProc->cleanedStdErr());
    } else {
        succeeded = true;
    }
    if (!succeeded) {
        emit q->warningMessage(Tr::tr("Failed to retrieve remote timestamp for file \"%1\". "
                                      "Incremental deployment will not work. Error message was: %2")
                               .arg(file.remoteFilePath(), error));
        return {};
    }
    const QByteArray output = statProc->readAllStandardOutput().trimmed();
    const QString warningString(Tr::tr("Unexpected stat output for remote file \"%1\": %2")
                                .arg(file.remoteFilePath()).arg(QString::fromUtf8(output)));
    if (!output.startsWith(file.remoteFilePath().toUtf8())) {
        emit q->warningMessage(warningString);
        return {};
    }
    const QByteArrayList columns = output.mid(file.remoteFilePath().toUtf8().size() + 1).split(' ');
    if (columns.size() < 14) { // Normal Linux stat: 16 columns in total, busybox stat: 15 columns
        emit q->warningMessage(warningString);
        return {};
    }
    bool isNumber;
    const qint64 secsSinceEpoch = columns.at(11).toLongLong(&isNumber);
    if (!isNumber) {
        emit q->warningMessage(warningString);
        return {};
    }
    return QDateTime::fromSecsSinceEpoch(secsSinceEpoch);
}

TaskItem GenericDirectUploadServicePrivate::statTask(UploadStorage *storage,
                                                     const DeployableFile &file,
                                                     StatEndHandler statEndHandler)
{
    const auto setupHandler = [=](QtcProcess &process) {
        // We'd like to use --format=%Y, but it's not supported by busybox.
        process.setCommand({q->deviceConfiguration()->filePath("stat"),
                            {"-t", Utils::ProcessArgs::quoteArgUnix(file.remoteFilePath())}});
    };
    const auto endHandler = [=](const QtcProcess &process) {
        QtcProcess *proc = const_cast<QtcProcess *>(&process);
        const QDateTime timestamp = timestampFromStat(file, proc);
        statEndHandler(storage, file, timestamp);
    };
    return Process(setupHandler, endHandler, endHandler);
}

TaskItem GenericDirectUploadServicePrivate::statTree(const TreeStorage<UploadStorage> &storage,
                                            FilesToStat filesToStat, StatEndHandler statEndHandler)
{
    const auto setupHandler = [=](TaskTree &tree) {
        UploadStorage *storagePtr = storage.activeStorage();
        const QList<DeployableFile> files = filesToStat(storagePtr);
        QList<TaskItem> statList;
        for (const DeployableFile &file : std::as_const(files)) {
            QTC_ASSERT(file.isValid(), continue);
            statList.append(statTask(storagePtr, file, statEndHandler));
        }
        tree.setupRoot(packIntoOptionalParallelGroups(statList));
    };
    return Tree(setupHandler);
}

TaskItem GenericDirectUploadServicePrivate::uploadGroup(const TreeStorage<UploadStorage> &storage)
{
    const auto groupSetupHandler = [=] {
        if (storage->filesToUpload.isEmpty()) {
            emit q->progressMessage(Tr::tr("No files need to be uploaded."));
            return GroupConfig{GroupAction::StopWithDone};
        }
        emit q->progressMessage(Tr::tr("%n file(s) need to be uploaded.", "",
                                       storage->filesToUpload.size()));
        FilesToTransfer files;
        for (const DeployableFile &file : std::as_const(storage->filesToUpload)) {
            if (!file.localFilePath().exists()) {
                const QString message = Tr::tr("Local file \"%1\" does not exist.")
                                              .arg(file.localFilePath().toUserOutput());
                if (ignoreMissingFiles) {
                    emit q->warningMessage(message);
                    continue;
                }
                emit q->errorMessage(message);
                return GroupConfig{GroupAction::StopWithError};
            }
            files.append({file.localFilePath(),
                          q->deviceConfiguration()->filePath(file.remoteFilePath())});
        }
        storage->filesToTransfer = files;
        if (storage->filesToTransfer.isEmpty()) {
            emit q->progressMessage(Tr::tr("No files need to be uploaded."));
            return GroupConfig{GroupAction::StopWithDone};
        }
        return GroupConfig();
    };

    const auto setupHandler = [this, storage](FileTransfer &transfer) {
        transfer.setFilesToTransfer(storage->filesToTransfer);
        QObject::connect(&transfer, &FileTransfer::progress,
                         q, &GenericDirectUploadService::progressMessage);
    };
    const auto errorHandler = [this](const FileTransfer &transfer) {
        emit q->errorMessage(transfer.resultData().m_errorString);
    };

    const Group group {
        DynamicSetup(groupSetupHandler),
        Transfer(setupHandler, {}, errorHandler)
    };
    return group;
}

TaskItem GenericDirectUploadServicePrivate::chmodTask(const DeployableFile &file)
{
    const auto setupHandler = [=](QtcProcess &process) {
        process.setCommand({q->deviceConfiguration()->filePath("chmod"),
                {"a+x", Utils::ProcessArgs::quoteArgUnix(file.remoteFilePath())}});
    };
    const auto errorHandler = [=](const QtcProcess &process) {
        const QString error = process.errorString();
        if (!error.isEmpty()) {
            emit q->warningMessage(Tr::tr("Remote chmod failed for file \"%1\": %2")
                                   .arg(file.remoteFilePath(), error));
        } else if (process.exitCode() != 0) {
            emit q->warningMessage(Tr::tr("Remote chmod failed for file \"%1\": %2")
                                   .arg(file.remoteFilePath(), process.cleanedStdErr()));
        }
    };
    return Process(setupHandler, {}, errorHandler);
}

TaskItem GenericDirectUploadServicePrivate::chmodTree(const TreeStorage<UploadStorage> &storage)
{
    const auto setupChmodHandler = [=](TaskTree &tree) {
        QList<DeployableFile> filesToChmod;
        for (const DeployableFile &file : std::as_const(storage->filesToUpload)) {
            if (file.isExecutable())
                filesToChmod << file;
        }
        QList<TaskItem> chmodList;
        for (const DeployableFile &file : std::as_const(filesToChmod)) {
            QTC_ASSERT(file.isValid(), continue);
            chmodList.append(chmodTask(file));
        }
        tree.setupRoot(packIntoOptionalParallelGroups(chmodList));
    };
    return Tree {setupChmodHandler};
}

Group GenericDirectUploadService::deployRecipe()
{
    const auto preFilesToStat = [this](UploadStorage *storage) {
        QList<DeployableFile> filesToStat;
        for (const DeployableFile &file : std::as_const(d->deployableFiles)) {
            if (d->incremental != IncrementalDeployment::Enabled || hasLocalFileChanged(file)) {
                storage->filesToUpload.append(file);
                continue;
            }
            if (d->incremental == IncrementalDeployment::NotSupported)
                continue;
            filesToStat << file;
        }
        return filesToStat;
    };
    const auto preStatEndHandler = [this](UploadStorage *storage, const DeployableFile &file,
                                          const QDateTime &timestamp) {
        if (!timestamp.isValid() || hasRemoteFileChanged(file, timestamp))
            storage->filesToUpload.append(file);
    };

    const auto postFilesToStat = [this](UploadStorage *storage) {
        return d->incremental == IncrementalDeployment::NotSupported
               ? QList<DeployableFile>() : storage->filesToUpload;
    };
    const auto postStatEndHandler = [this](UploadStorage *storage, const DeployableFile &file,
                                           const QDateTime &timestamp) {
        Q_UNUSED(storage)
        if (timestamp.isValid())
            saveDeploymentTimeStamp(file, timestamp);
    };
    const auto doneHandler = [this] {
        emit progressMessage(Tr::tr("All files successfully deployed."));
    };

    const TreeStorage<UploadStorage> storage;
    const Group root {
        Storage(storage),
        d->statTree(storage, preFilesToStat, preStatEndHandler),
        d->uploadGroup(storage),
        Group {
            d->chmodTree(storage),
            d->statTree(storage, postFilesToStat, postStatEndHandler)
        },
        OnGroupDone(doneHandler)
    };
    return root;
}

} //namespace RemoteLinux
