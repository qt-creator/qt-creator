// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericdirectuploadstep.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Utils::Tasking;

namespace RemoteLinux {

const int MaxConcurrentStatCalls = 10;

struct UploadStorage
{
    QList<DeployableFile> filesToUpload;
};

enum class IncrementalDeployment { Enabled, Disabled, NotSupported };

class GenericDirectUploadStepPrivate
{
public:
    GenericDirectUploadStepPrivate(GenericDirectUploadStep *parent)
        : q(parent)
    {}

    QDateTime timestampFromStat(const DeployableFile &file, QtcProcess *statProc);

    using FilesToStat = std::function<QList<DeployableFile>(UploadStorage *)>;
    using StatEndHandler
          = std::function<void(UploadStorage *, const DeployableFile &, const QDateTime &)>;
    TaskItem statTask(UploadStorage *storage, const DeployableFile &file,
                      StatEndHandler statEndHandler);
    TaskItem statTree(const TreeStorage<UploadStorage> &storage, FilesToStat filesToStat,
                      StatEndHandler statEndHandler);
    TaskItem uploadTask(const TreeStorage<UploadStorage> &storage);
    TaskItem chmodTask(const DeployableFile &file);
    TaskItem chmodTree(const TreeStorage<UploadStorage> &storage);

    GenericDirectUploadStep *q;
    IncrementalDeployment m_incremental = IncrementalDeployment::NotSupported;
    bool m_ignoreMissingFiles = false;
    mutable QList<DeployableFile> m_deployableFiles;
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

bool GenericDirectUploadStep::isDeploymentNecessary() const
{
    QList<DeployableFile> collected;
    for (int i = 0; i < d->m_deployableFiles.count(); ++i)
        collected.append(collectFilesToUpload(d->m_deployableFiles.at(i)));

    QTC_CHECK(collected.size() >= d->m_deployableFiles.size());
    d->m_deployableFiles = collected;
    return !d->m_deployableFiles.isEmpty();
}

QDateTime GenericDirectUploadStepPrivate::timestampFromStat(const DeployableFile &file,
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
        q->addWarningMessage(Tr::tr("Failed to retrieve remote timestamp for file \"%1\". "
                                   "Incremental deployment will not work. Error message was: %2")
                               .arg(file.remoteFilePath(), error));
        return {};
    }
    const QByteArray output = statProc->readAllRawStandardOutput().trimmed();
    const QString warningString(Tr::tr("Unexpected stat output for remote file \"%1\": %2")
                                .arg(file.remoteFilePath()).arg(QString::fromUtf8(output)));
    if (!output.startsWith(file.remoteFilePath().toUtf8())) {
        q->addWarningMessage(warningString);
        return {};
    }
    const QByteArrayList columns = output.mid(file.remoteFilePath().toUtf8().size() + 1).split(' ');
    if (columns.size() < 14) { // Normal Linux stat: 16 columns in total, busybox stat: 15 columns
        q->addWarningMessage(warningString);
        return {};
    }
    bool isNumber;
    const qint64 secsSinceEpoch = columns.at(11).toLongLong(&isNumber);
    if (!isNumber) {
        q->addWarningMessage(warningString);
        return {};
    }
    return QDateTime::fromSecsSinceEpoch(secsSinceEpoch);
}

TaskItem GenericDirectUploadStepPrivate::statTask(UploadStorage *storage,
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

TaskItem GenericDirectUploadStepPrivate::statTree(const TreeStorage<UploadStorage> &storage,
                                                  FilesToStat filesToStat, StatEndHandler statEndHandler)
{
    const auto setupHandler = [=](TaskTree &tree) {
        UploadStorage *storagePtr = storage.activeStorage();
        const QList<DeployableFile> files = filesToStat(storagePtr);
        QList<TaskItem> statList{optional, ParallelLimit(MaxConcurrentStatCalls)};
        for (const DeployableFile &file : std::as_const(files)) {
            QTC_ASSERT(file.isValid(), continue);
            statList.append(statTask(storagePtr, file, statEndHandler));
        }
        tree.setupRoot({statList});
    };
    return Tree(setupHandler);
}

TaskItem GenericDirectUploadStepPrivate::uploadTask(const TreeStorage<UploadStorage> &storage)
{
    const auto setupHandler = [this, storage](FileTransfer &transfer) {
        if (storage->filesToUpload.isEmpty()) {
            q->addProgressMessage(Tr::tr("No files need to be uploaded."));
            return TaskAction::StopWithDone;
        }
        q->addProgressMessage(Tr::tr("%n file(s) need to be uploaded.", "",
                                     storage->filesToUpload.size()));
        FilesToTransfer files;
        for (const DeployableFile &file : std::as_const(storage->filesToUpload)) {
            if (!file.localFilePath().exists()) {
                const QString message = Tr::tr("Local file \"%1\" does not exist.")
                                              .arg(file.localFilePath().toUserOutput());
                if (m_ignoreMissingFiles) {
                    q->addWarningMessage(message);
                    continue;
                }
                q->addErrorMessage(message);
                return TaskAction::StopWithError;
            }
            files.append({file.localFilePath(),
                          q->deviceConfiguration()->filePath(file.remoteFilePath())});
        }
        if (files.isEmpty()) {
            q->addProgressMessage(Tr::tr("No files need to be uploaded."));
            return TaskAction::StopWithDone;
        }
        transfer.setFilesToTransfer(files);
        QObject::connect(&transfer, &FileTransfer::progress,
                         q, &GenericDirectUploadStep::addProgressMessage);
        return TaskAction::Continue;
    };
    const auto errorHandler = [this](const FileTransfer &transfer) {
        q->addErrorMessage(transfer.resultData().m_errorString);
    };

    return Transfer(setupHandler, {}, errorHandler);
}

TaskItem GenericDirectUploadStepPrivate::chmodTask(const DeployableFile &file)
{
    const auto setupHandler = [=](QtcProcess &process) {
        process.setCommand({q->deviceConfiguration()->filePath("chmod"),
                {"a+x", Utils::ProcessArgs::quoteArgUnix(file.remoteFilePath())}});
    };
    const auto errorHandler = [=](const QtcProcess &process) {
        const QString error = process.errorString();
        if (!error.isEmpty()) {
            q->addWarningMessage(Tr::tr("Remote chmod failed for file \"%1\": %2")
                                     .arg(file.remoteFilePath(), error));
        } else if (process.exitCode() != 0) {
            q->addWarningMessage(Tr::tr("Remote chmod failed for file \"%1\": %2")
                                          .arg(file.remoteFilePath(), process.cleanedStdErr()));
        }
    };
    return Process(setupHandler, {}, errorHandler);
}

TaskItem GenericDirectUploadStepPrivate::chmodTree(const TreeStorage<UploadStorage> &storage)
{
    const auto setupChmodHandler = [=](TaskTree &tree) {
        QList<DeployableFile> filesToChmod;
        for (const DeployableFile &file : std::as_const(storage->filesToUpload)) {
            if (file.isExecutable())
                filesToChmod << file;
        }
        QList<TaskItem> chmodList{optional, ParallelLimit(MaxConcurrentStatCalls)};
        for (const DeployableFile &file : std::as_const(filesToChmod)) {
            QTC_ASSERT(file.isValid(), continue);
            chmodList.append(chmodTask(file));
        }
        tree.setupRoot({chmodList});
    };
    return Tree(setupChmodHandler);
}

Group GenericDirectUploadStep::deployRecipe()
{
    const auto preFilesToStat = [this](UploadStorage *storage) {
        QList<DeployableFile> filesToStat;
        for (const DeployableFile &file : std::as_const(d->m_deployableFiles)) {
            if (d->m_incremental != IncrementalDeployment::Enabled || hasLocalFileChanged(file)) {
                storage->filesToUpload.append(file);
                continue;
            }
            if (d->m_incremental == IncrementalDeployment::NotSupported)
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
        return d->m_incremental == IncrementalDeployment::NotSupported
               ? QList<DeployableFile>() : storage->filesToUpload;
    };
    const auto postStatEndHandler = [this](UploadStorage *storage, const DeployableFile &file,
                                           const QDateTime &timestamp) {
        Q_UNUSED(storage)
        if (timestamp.isValid())
            saveDeploymentTimeStamp(file, timestamp);
    };
    const auto doneHandler = [this] {
        addProgressMessage(Tr::tr("All files successfully deployed."));
    };

    const TreeStorage<UploadStorage> storage;
    const Group root {
        Storage(storage),
        d->statTree(storage, preFilesToStat, preStatEndHandler),
        d->uploadTask(storage),
        Group {
            d->chmodTree(storage),
            d->statTree(storage, postFilesToStat, postStatEndHandler)
        },
        OnGroupDone(doneHandler)
    };
    return root;
}

GenericDirectUploadStep::GenericDirectUploadStep(BuildStepList *bsl, Utils::Id id,
                                                 bool offerIncrementalDeployment)
    : AbstractRemoteLinuxDeployStep(bsl, id),
      d(new GenericDirectUploadStepPrivate(this))
{
    BoolAspect *incremental = nullptr;
    if (offerIncrementalDeployment) {
        incremental = addAspect<BoolAspect>();
        incremental->setSettingsKey("RemoteLinux.GenericDirectUploadStep.Incremental");
        incremental->setLabel(Tr::tr("Incremental deployment"),
                              BoolAspect::LabelPlacement::AtCheckBox);
        incremental->setValue(true);
        incremental->setDefaultValue(true);
    }

    auto ignoreMissingFiles = addAspect<BoolAspect>();
    ignoreMissingFiles->setSettingsKey("RemoteLinux.GenericDirectUploadStep.IgnoreMissingFiles");
    ignoreMissingFiles->setLabel(Tr::tr("Ignore missing files"),
                                 BoolAspect::LabelPlacement::AtCheckBox);
    ignoreMissingFiles->setValue(false);

    setInternalInitializer([this, incremental, ignoreMissingFiles] {
        if (incremental) {
            d->m_incremental = incremental->value()
                ? IncrementalDeployment::Enabled : IncrementalDeployment::Disabled;
        } else {
            d->m_incremental = IncrementalDeployment::NotSupported;
        }
        d->m_ignoreMissingFiles = ignoreMissingFiles->value();
        return isDeploymentPossible();
    });

    setRunPreparer([this] {
        d->m_deployableFiles = target()->deploymentData().allFiles();
    });
}

GenericDirectUploadStep::~GenericDirectUploadStep()
{
    delete d;
}

Utils::Id GenericDirectUploadStep::stepId()
{
    return Constants::DirectUploadStepId;
}

QString GenericDirectUploadStep::displayName()
{
    return Tr::tr("Upload files via SFTP");
}

} //namespace RemoteLinux
