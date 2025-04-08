// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericdeploystep.h"

#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/qtcprocess.h>
#include <utils/processinterface.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace RemoteLinux::Internal {

// GenericDeployStep

class GenericDeployStep final : public AbstractRemoteLinuxDeployStep
{
public:
    GenericDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        flags.setDisplayStyle(StringAspect::LineEditDisplay);
        flags.setSettingsKey("RemoteLinux.RsyncDeployStep.Flags");
        flags.setLabelText(Tr::tr("Flags for rsync:"));
        flags.setValue(FileTransferSetupData::defaultRsyncFlags());

        ignoreMissingFiles.setSettingsKey("RemoteLinux.RsyncDeployStep.IgnoreMissingFiles");
        ignoreMissingFiles.setLabelText(Tr::tr("Ignore missing files:"));
        ignoreMissingFiles.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);

        method.setSettingsKey("RemoteLinux.RsyncDeployStep.TransferMethod");
        method.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
        method.setDisplayName(Tr::tr("Transfer method:"));
        method.addOption(Tr::tr("Use rsync or sftp if available, but prefer rsync. "
                                "Otherwise use default transfer."));
        method.addOption(Tr::tr("Use sftp if available. Otherwise use default transfer."));
        method.addOption(Tr::tr("Use default transfer. This might be slow."));

        setInternalInitializer([this]() -> expected_str<void> {
            return isDeploymentPossible();
        });
    }

private:
    GroupItem deployRecipe() final;
    GroupItem mkdirTask(const Storage<FilesToTransfer> &storage);
    GroupItem transferTask(const Storage<FilesToTransfer> &storage);

    StringAspect flags{this};
    BoolAspect ignoreMissingFiles{this};
    SelectionAspect method{this};
    bool m_emittedDowngradeWarning = false;
};

GroupItem GenericDeployStep::mkdirTask(const Storage<FilesToTransfer> &storage)
{
    const auto onSetup = [storage](Async<Result<>> &async) {
        FilePaths remoteDirs;
        for (const FileToTransfer &file : *storage)
            remoteDirs << file.m_target.parentDir();

        FilePath::sort(remoteDirs);
        FilePath::removeDuplicates(remoteDirs);

        async.setConcurrentCallData([remoteDirs](QPromise<Result<>> &promise) {
            for (const FilePath &dir : remoteDirs) {
                const Result<> result = dir.ensureWritableDir();
                promise.addResult(result);
                if (!result)
                    promise.future().cancel();
            }
        });
    };

    const auto onError = [this](const Async<Result<>> &async) {
        const int numResults = async.future().resultCount();
        if (numResults == 0) {
            addErrorMessage(
                Tr::tr("Unknown error occurred while trying to create remote directories.") + '\n');
            return;
        }

        for (int i = 0; i < numResults; ++i) {
            const Result<> result = async.future().resultAt(i);
            if (!result)
                addErrorMessage(result.error());
        }
    };

    return AsyncTask<Result<>>(onSetup, onError, CallDoneIf::Error);
}

static FileTransferMethod effectiveTransferMethodFor(const FileToTransfer &fileToTransfer,
                                                      FileTransferMethod preferred)
{
    auto sourceDevice = ProjectExplorer::DeviceManager::deviceForPath(fileToTransfer.m_source);
    auto targetDevice = ProjectExplorer::DeviceManager::deviceForPath(fileToTransfer.m_target);
    if (!sourceDevice || !targetDevice)
        return FileTransferMethod::GenericCopy;
    if (sourceDevice == targetDevice)
        return FileTransferMethod::GenericCopy;

    const auto devicesSupportMethod = [&](Id method) {
        return sourceDevice->extraData(method).toBool() && targetDevice->extraData(method).toBool();
    };
    if (preferred == FileTransferMethod::Rsync
        && !devicesSupportMethod(ProjectExplorer::Constants::SUPPORTS_RSYNC)) {
        preferred = FileTransferMethod::Sftp;
    }
    if (preferred == FileTransferMethod::Sftp
        && !devicesSupportMethod(ProjectExplorer::Constants::SUPPORTS_SFTP)) {
        preferred = FileTransferMethod::GenericCopy;
    }
    return preferred;
}

GroupItem GenericDeployStep::transferTask(const Storage<FilesToTransfer> &storage)
{
    const auto onSetup = [this, storage](FileTransfer &transfer) {
        FileTransferMethod preferredTransferMethod = FileTransferMethod::GenericCopy;
        if (method() == 0)
            preferredTransferMethod = FileTransferMethod::Rsync;
        else if (method() == 1)
            preferredTransferMethod = FileTransferMethod::Sftp;

        FileTransferMethod transferMethod = preferredTransferMethod;
        if (transferMethod != FileTransferMethod::GenericCopy) {
            for (const FileToTransfer &fileToTransfer : *storage) {
                transferMethod = effectiveTransferMethodFor(fileToTransfer, transferMethod);
                if (transferMethod == FileTransferMethod::GenericCopy)
                    break;
            }
        }
        if (!m_emittedDowngradeWarning && transferMethod != preferredTransferMethod) {
            const QString message
                = Tr::tr("Transfer method was downgraded from \"%1\" to \"%2\". If "
                         "this is unexpected, please re-test device \"%3\".")
                      .arg(FileTransfer::transferMethodName(preferredTransferMethod),
                           FileTransfer::transferMethodName(transferMethod),
                           deviceConfiguration()->displayName());
            addProgressMessage(message);
            m_emittedDowngradeWarning = true;
        }
        transfer.setTransferMethod(transferMethod);

        transfer.setRsyncFlags(flags());
        transfer.setFilesToTransfer(*storage);
        connect(&transfer, &FileTransfer::progress, this, &GenericDeployStep::handleStdOutData);
    };
    const auto onError = [this](const FileTransfer &transfer) {
        const ProcessResultData result = transfer.resultData();
        if (result.m_error == QProcess::FailedToStart) {
            addErrorMessage(Tr::tr("%1 failed to start: %2")
                                .arg(transfer.transferMethodName(), result.m_errorString));
        } else if (result.m_exitStatus == QProcess::CrashExit) {
            addErrorMessage(Tr::tr("%1 crashed.").arg(transfer.transferMethodName()));
        } else if (result.m_exitCode != 0) {
            addErrorMessage(
                Tr::tr("%1 failed with exit code %2.")
                    .arg(transfer.transferMethodName())
                    .arg(result.m_exitCode)
                + "\n" + result.m_errorString);
        }
    };
    return FileTransferTask(onSetup, onError, CallDoneIf::Error);
}

GroupItem GenericDeployStep::deployRecipe()
{
    const Storage<FilesToTransfer> storage;

    const auto onSetup = [this, storage] {
        const QList<DeployableFile> deployableFiles = buildSystem()->deploymentData().allFiles();
        FilesToTransfer &files = *storage;
        for (const DeployableFile &file : deployableFiles) {
            if (!ignoreMissingFiles() || file.localFilePath().exists()) {
                const FilePermissions permissions = file.isExecutable()
                    ? FilePermissions::ForceExecutable : FilePermissions::Default;
                files.append({file.localFilePath(),
                              deviceConfiguration()->filePath(file.remoteFilePath()), permissions});
            }
        }
        if (files.isEmpty()) {
            addSkipDeploymentMessage();
            return SetupResult::StopWithSuccess;
        }
        return SetupResult::Continue;
    };

    return Group {
        storage,
        onGroupSetup(onSetup),
        mkdirTask(storage),
        transferTask(storage)
    };
}

// Factory

class GenericDeployStepFactory final : public BuildStepFactory
{
public:
    GenericDeployStepFactory()
    {
        registerStep<GenericDeployStep>(Constants::GenericDeployStepId);
        setDisplayName(Tr::tr("Deploy files"));
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
    }
};

void setupGenericDeployStep()
{
    static GenericDeployStepFactory theGenericDeployStepFactory;
}

} // RemoteLinux::Internal
