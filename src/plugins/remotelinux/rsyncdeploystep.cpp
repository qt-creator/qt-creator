// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rsyncdeploystep.h"

#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Utils::Tasking;

namespace RemoteLinux {

// RsyncDeployStep

RsyncDeployStep::RsyncDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto flags = addAspect<StringAspect>();
    flags->setDisplayStyle(StringAspect::LineEditDisplay);
    flags->setSettingsKey("RemoteLinux.RsyncDeployStep.Flags");
    flags->setLabelText(Tr::tr("Flags:"));
    flags->setValue(FileTransferSetupData::defaultRsyncFlags());

    auto ignoreMissingFiles = addAspect<BoolAspect>();
    ignoreMissingFiles->setSettingsKey("RemoteLinux.RsyncDeployStep.IgnoreMissingFiles");
    ignoreMissingFiles->setLabel(Tr::tr("Ignore missing files:"),
                                 BoolAspect::LabelPlacement::InExtraLabel);
    ignoreMissingFiles->setValue(false);

    setInternalInitializer([this, ignoreMissingFiles, flags] {
        if (BuildDeviceKitAspect::device(kit()) == DeviceKitAspect::device(kit())) {
            // rsync transfer on the same device currently not implemented
            // and typically not wanted.
            return CheckResult::failure(
                Tr::tr("rsync is only supported for transfers between different devices."));
        }
        m_ignoreMissingFiles = ignoreMissingFiles->value();
        m_flags = flags->value();
        return isDeploymentPossible();
    });

    setRunPreparer([this] {
        const QList<DeployableFile> files = target()->deploymentData().allFiles();
        m_files.clear();
        for (const DeployableFile &f : files)
            m_files.append({f.localFilePath(), deviceConfiguration()->filePath(f.remoteFilePath())});
    });
}

RsyncDeployStep::~RsyncDeployStep() = default;

bool RsyncDeployStep::isDeploymentNecessary() const
{
    if (m_ignoreMissingFiles)
        Utils::erase(m_files, [](const FileToTransfer &file) { return !file.m_source.exists(); });
    return !m_files.empty();
}

TaskItem RsyncDeployStep::mkdirTask()
{
    const auto setupHandler = [this](QtcProcess &process) {
        QStringList remoteDirs;
        for (const FileToTransfer &file : std::as_const(m_files))
            remoteDirs << file.m_target.parentDir().path();
        remoteDirs.sort();
        remoteDirs.removeDuplicates();
        process.setCommand({deviceConfiguration()->filePath("mkdir"),
                            QStringList("-p") + remoteDirs});
        connect(&process, &QtcProcess::readyReadStandardError, this, [this, proc = &process] {
            handleStdErrData(QString::fromLocal8Bit(proc->readAllRawStandardError()));
        });
    };
    const auto errorHandler = [this](const QtcProcess &process) {
        QString finalMessage = process.errorString();
        const QString stdErr = process.cleanedStdErr();
        if (!stdErr.isEmpty()) {
            if (!finalMessage.isEmpty())
                finalMessage += '\n';
            finalMessage += stdErr;
        }
        addErrorMessage(Tr::tr("Deploy via rsync: failed to create remote directories:")
                        + '\n' + finalMessage);
    };
    return Process(setupHandler, {}, errorHandler);
}

TaskItem RsyncDeployStep::transferTask()
{
    const auto setupHandler = [this](FileTransfer &transfer) {
        transfer.setTransferMethod(FileTransferMethod::Rsync);
        transfer.setRsyncFlags(m_flags);
        transfer.setFilesToTransfer(m_files);
        connect(&transfer, &FileTransfer::progress,
                this, &AbstractRemoteLinuxDeployStep::handleStdOutData);
    };
    const auto errorHandler = [this](const FileTransfer &transfer) {
        const ProcessResultData result = transfer.resultData();
        if (result.m_error == QProcess::FailedToStart) {
            addErrorMessage(Tr::tr("rsync failed to start: %1").arg(result.m_errorString));
        } else if (result.m_exitStatus == QProcess::CrashExit) {
            addErrorMessage(Tr::tr("rsync crashed."));
        } else if (result.m_exitCode != 0) {
            addErrorMessage(Tr::tr("rsync failed with exit code %1.").arg(result.m_exitCode)
                            + "\n" + result.m_errorString);
        }
    };
    return Transfer(setupHandler, {}, errorHandler);
}

Group RsyncDeployStep::deployRecipe()
{
    return Group { mkdirTask(), transferTask() };
}

Utils::Id RsyncDeployStep::stepId()
{
    return Constants::RsyncDeployStepId;
}

QString RsyncDeployStep::displayName()
{
    return Tr::tr("Deploy files via rsync");
}

} // RemoteLinux
