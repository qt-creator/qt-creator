// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rsyncdeploystep.h"

#include "abstractremotelinuxdeploystep.h"
#include "abstractremotelinuxdeployservice.h"
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

class RsyncDeployService : public AbstractRemoteLinuxDeployService
{
public:
    void setDeployableFiles(const QList<DeployableFile> &files);
    void setIgnoreMissingFiles(bool ignore) { m_ignoreMissingFiles = ignore; }
    void setFlags(const QString &flags) { m_flags = flags; }

private:
    bool isDeploymentNecessary() const final;
    Group deployRecipe() final;
    TaskItem mkdirTask();
    TaskItem transferTask();

    mutable FilesToTransfer m_files;
    bool m_ignoreMissingFiles = false;
    QString m_flags;
};

void RsyncDeployService::setDeployableFiles(const QList<DeployableFile> &files)
{
    m_files.clear();
    for (const DeployableFile &f : files)
        m_files.append({f.localFilePath(), deviceConfiguration()->filePath(f.remoteFilePath())});
}

bool RsyncDeployService::isDeploymentNecessary() const
{
    if (m_ignoreMissingFiles)
        Utils::erase(m_files, [](const FileToTransfer &file) { return !file.m_source.exists(); });
    return !m_files.empty();
}

TaskItem RsyncDeployService::mkdirTask()
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
            emit stdErrData(QString::fromLocal8Bit(proc->readAllStandardError()));
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
        emit errorMessage(Tr::tr("Deploy via rsync: failed to create remote directories:")
                          + '\n' + finalMessage);
    };
    return Process(setupHandler, {}, errorHandler);
}

TaskItem RsyncDeployService::transferTask()
{
    const auto setupHandler = [this](FileTransfer &transfer) {
        transfer.setTransferMethod(FileTransferMethod::Rsync);
        transfer.setRsyncFlags(m_flags);
        transfer.setFilesToTransfer(m_files);
        connect(&transfer, &FileTransfer::progress,
                this, &AbstractRemoteLinuxDeployService::stdOutData);
    };
    const auto errorHandler = [this](const FileTransfer &transfer) {
        const ProcessResultData result = transfer.resultData();
        if (result.m_error == QProcess::FailedToStart)
            emit errorMessage(Tr::tr("rsync failed to start: %1").arg(result.m_errorString));
        else if (result.m_exitStatus == QProcess::CrashExit)
            emit errorMessage(Tr::tr("rsync crashed."));
        else if (result.m_exitCode != 0)
            emit errorMessage(Tr::tr("rsync failed with exit code %1.").arg(result.m_exitCode));
    };
    return Transfer(setupHandler, {}, errorHandler);
}

Group RsyncDeployService::deployRecipe()
{
    return Group { mkdirTask(), transferTask() };
}

// RsyncDeployStep

RsyncDeployStep::RsyncDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = new RsyncDeployService;
    setDeployService(service);

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

    setInternalInitializer([this, service, flags, ignoreMissingFiles] {
        if (BuildDeviceKitAspect::device(kit()) == DeviceKitAspect::device(kit())) {
            // rsync transfer on the same device currently not implemented
            // and typically not wanted.
            return CheckResult::failure(
                Tr::tr("rsync is only supported for transfers between different devices."));
        }
        service->setIgnoreMissingFiles(ignoreMissingFiles->value());
        service->setFlags(flags->value());
        return service->isDeploymentPossible();
    });

    setRunPreparer([this, service] {
        service->setDeployableFiles(target()->deploymentData().allFiles());
    });
}

RsyncDeployStep::~RsyncDeployStep() = default;

Utils::Id RsyncDeployStep::stepId()
{
    return Constants::RsyncDeployStepId;
}

QString RsyncDeployStep::displayName()
{
    return Tr::tr("Deploy files via rsync");
}

} // RemoteLinux
