// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericdirectuploadstep.h"
#include "abstractremotelinuxdeploystep.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace RemoteLinux::Internal {

const int MaxConcurrentStatCalls = 10;

struct UploadStorage
{
    QList<DeployableFile> filesToUpload;
};

class GenericDirectUploadStep : public AbstractRemoteLinuxDeployStep
{
public:
    GenericDirectUploadStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        incremental.setSettingsKey("RemoteLinux.GenericDirectUploadStep.Incremental");
        incremental.setLabelText(Tr::tr("Incremental deployment"));
        incremental.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
        incremental.setDefaultValue(true);

        ignoreMissingFiles.setSettingsKey("RemoteLinux.GenericDirectUploadStep.IgnoreMissingFiles");
        ignoreMissingFiles.setLabelText(Tr::tr("Ignore missing files"));
        ignoreMissingFiles.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);

        setInternalInitializer([this] {
            return isDeploymentPossible();
        });
    }

    bool isDeploymentNecessary() const final;
    GroupItem deployRecipe() final;

    QDateTime timestampFromStat(const DeployableFile &file, Process *statProc);

    using FilesToStat = std::function<QList<DeployableFile>(UploadStorage *)>;
    using StatEndHandler
          = std::function<void(UploadStorage *, const DeployableFile &, const QDateTime &)>;
    GroupItem statTask(UploadStorage *storage, const DeployableFile &file,
                       StatEndHandler statEndHandler);
    GroupItem statTree(const TreeStorage<UploadStorage> &storage, FilesToStat filesToStat,
                       StatEndHandler statEndHandler);
    GroupItem uploadTask(const TreeStorage<UploadStorage> &storage);
    GroupItem chmodTask(const DeployableFile &file);
    GroupItem chmodTree(const TreeStorage<UploadStorage> &storage);

    mutable QList<DeployableFile> m_deployableFiles;

    BoolAspect incremental{this};
    BoolAspect ignoreMissingFiles{this};
};

static QList<DeployableFile> collectFilesToUpload(const DeployableFile &deployable)
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
    m_deployableFiles = target()->deploymentData().allFiles();
    QList<DeployableFile> collected;
    for (int i = 0; i < m_deployableFiles.count(); ++i)
        collected.append(collectFilesToUpload(m_deployableFiles.at(i)));

    QTC_CHECK(collected.size() >= m_deployableFiles.size());
    m_deployableFiles = collected;
    return !m_deployableFiles.isEmpty();
}

QDateTime GenericDirectUploadStep::timestampFromStat(const DeployableFile &file,
                                                     Process *statProc)
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
        addWarningMessage(Tr::tr("Failed to retrieve remote timestamp for file \"%1\". "
                                 "Incremental deployment will not work. Error message was: %2")
                              .arg(file.remoteFilePath(), error));
        return {};
    }
    const QByteArray output = statProc->readAllRawStandardOutput().trimmed();
    const QString warningString(Tr::tr("Unexpected stat output for remote file \"%1\": %2")
                                .arg(file.remoteFilePath()).arg(QString::fromUtf8(output)));
    if (!output.startsWith(file.remoteFilePath().toUtf8())) {
        addWarningMessage(warningString);
        return {};
    }
    const QByteArrayList columns = output.mid(file.remoteFilePath().toUtf8().size() + 1).split(' ');
    if (columns.size() < 14) { // Normal Linux stat: 16 columns in total, busybox stat: 15 columns
        addWarningMessage(warningString);
        return {};
    }
    bool isNumber;
    const qint64 secsSinceEpoch = columns.at(11).toLongLong(&isNumber);
    if (!isNumber) {
        addWarningMessage(warningString);
        return {};
    }
    return QDateTime::fromSecsSinceEpoch(secsSinceEpoch);
}

GroupItem GenericDirectUploadStep::statTask(UploadStorage *storage,
                                            const DeployableFile &file,
                                            StatEndHandler statEndHandler)
{
    const auto setupHandler = [=](Process &process) {
        // We'd like to use --format=%Y, but it's not supported by busybox.
        process.setCommand({deviceConfiguration()->filePath("stat"),
                            {"-t", Utils::ProcessArgs::quoteArgUnix(file.remoteFilePath())}});
    };
    const auto endHandler = [=](const Process &process) {
        Process *proc = const_cast<Process *>(&process);
        const QDateTime timestamp = timestampFromStat(file, proc);
        statEndHandler(storage, file, timestamp);
    };
    return ProcessTask(setupHandler, endHandler, endHandler);
}

GroupItem GenericDirectUploadStep::statTree(const TreeStorage<UploadStorage> &storage,
                                            FilesToStat filesToStat, StatEndHandler statEndHandler)
{
    const auto setupHandler = [=](TaskTree &tree) {
        UploadStorage *storagePtr = storage.activeStorage();
        const QList<DeployableFile> files = filesToStat(storagePtr);
        QList<GroupItem> statList{finishAllAndDone, parallelLimit(MaxConcurrentStatCalls)};
        for (const DeployableFile &file : std::as_const(files)) {
            QTC_ASSERT(file.isValid(), continue);
            statList.append(statTask(storagePtr, file, statEndHandler));
        }
        tree.setRecipe({statList});
    };
    return TaskTreeTask(setupHandler);
}

GroupItem GenericDirectUploadStep::uploadTask(const TreeStorage<UploadStorage> &storage)
{
    const auto setupHandler = [this, storage](FileTransfer &transfer) {
        if (storage->filesToUpload.isEmpty()) {
            addProgressMessage(Tr::tr("No files need to be uploaded."));
            return SetupResult::StopWithDone;
        }
        addProgressMessage(Tr::tr("%n file(s) need to be uploaded.", "",
                                  storage->filesToUpload.size()));
        FilesToTransfer files;
        for (const DeployableFile &file : std::as_const(storage->filesToUpload)) {
            if (!file.localFilePath().exists()) {
                const QString message = Tr::tr("Local file \"%1\" does not exist.")
                                              .arg(file.localFilePath().toUserOutput());
                if (ignoreMissingFiles()) {
                    addWarningMessage(message);
                    continue;
                }
                addErrorMessage(message);
                return SetupResult::StopWithError;
            }
            files.append({file.localFilePath(),
                          deviceConfiguration()->filePath(file.remoteFilePath())});
        }
        if (files.isEmpty()) {
            addProgressMessage(Tr::tr("No files need to be uploaded."));
            return SetupResult::StopWithDone;
        }
        transfer.setFilesToTransfer(files);
        QObject::connect(&transfer, &FileTransfer::progress,
                         this, &GenericDirectUploadStep::addProgressMessage);
        return SetupResult::Continue;
    };
    const auto errorHandler = [this](const FileTransfer &transfer) {
        addErrorMessage(transfer.resultData().m_errorString);
    };

    return FileTransferTask(setupHandler, {}, errorHandler);
}

GroupItem GenericDirectUploadStep::chmodTask(const DeployableFile &file)
{
    const auto setupHandler = [=](Process &process) {
        process.setCommand({deviceConfiguration()->filePath("chmod"),
                {"a+x", Utils::ProcessArgs::quoteArgUnix(file.remoteFilePath())}});
    };
    const auto errorHandler = [=](const Process &process) {
        const QString error = process.errorString();
        if (!error.isEmpty()) {
            addWarningMessage(Tr::tr("Remote chmod failed for file \"%1\": %2")
                                  .arg(file.remoteFilePath(), error));
        } else if (process.exitCode() != 0) {
            addWarningMessage(Tr::tr("Remote chmod failed for file \"%1\": %2")
                                  .arg(file.remoteFilePath(), process.cleanedStdErr()));
        }
    };
    return ProcessTask(setupHandler, {}, errorHandler);
}

GroupItem GenericDirectUploadStep::chmodTree(const TreeStorage<UploadStorage> &storage)
{
    const auto setupChmodHandler = [=](TaskTree &tree) {
        QList<DeployableFile> filesToChmod;
        for (const DeployableFile &file : std::as_const(storage->filesToUpload)) {
            if (file.isExecutable())
                filesToChmod << file;
        }
        QList<GroupItem> chmodList{finishAllAndDone, parallelLimit(MaxConcurrentStatCalls)};
        for (const DeployableFile &file : std::as_const(filesToChmod)) {
            QTC_ASSERT(file.isValid(), continue);
            chmodList.append(chmodTask(file));
        }
        tree.setRecipe({chmodList});
    };
    return TaskTreeTask(setupChmodHandler);
}

GroupItem GenericDirectUploadStep::deployRecipe()
{
    const auto preFilesToStat = [this](UploadStorage *storage) {
        QList<DeployableFile> filesToStat;
        for (const DeployableFile &file : std::as_const(m_deployableFiles)) {
            if (!incremental() || hasLocalFileChanged(file)) {
                storage->filesToUpload.append(file);
                continue;
            }
            filesToStat << file;
        }
        return filesToStat;
    };
    const auto preStatEndHandler = [this](UploadStorage *storage, const DeployableFile &file,
                                          const QDateTime &timestamp) {
        if (!timestamp.isValid() || hasRemoteFileChanged(file, timestamp))
            storage->filesToUpload.append(file);
    };

    const auto postFilesToStat = [](UploadStorage *storage) {
        return storage->filesToUpload;
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
        Tasking::Storage(storage),
        statTree(storage, preFilesToStat, preStatEndHandler),
        uploadTask(storage),
        Group {
            chmodTree(storage),
            statTree(storage, postFilesToStat, postStatEndHandler)
        },
        onGroupDone(doneHandler)
    };
    return root;
}

// Factory

GenericDirectUploadStepFactory::GenericDirectUploadStepFactory()
{
    registerStep<GenericDirectUploadStep>(Constants::DirectUploadStepId);
    setDisplayName(Tr::tr("Upload files via SFTP"));
}

} // RemoteLinux::Internal
