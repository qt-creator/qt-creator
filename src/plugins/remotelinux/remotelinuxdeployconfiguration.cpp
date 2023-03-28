// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdeployconfiguration.h"

#include "makeinstallstep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/filetransferinterface.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace RemoteLinux::Internal {

FileTransferMethod defaultTransferMethod(Kit *kit)
{
    auto runDevice = DeviceKitAspect::device(kit);
    auto buildDevice = BuildDeviceKitAspect::device(kit);

    if (runDevice != buildDevice) {
        // FIXME: That's not the full truth, we need support from the build
        // device, too.
        if (runDevice && runDevice->extraData(Constants::SupportsRSync).toBool())
            return FileTransferMethod::Rsync;
    }

    if (runDevice && runDevice->extraData(Constants::SupportsSftp).toBool())
        return FileTransferMethod::Sftp;

    return FileTransferMethod::GenericCopy;
}

RemoteLinuxDeployConfigurationFactory::RemoteLinuxDeployConfigurationFactory()
{
    setConfigBaseId(RemoteLinux::Constants::DeployToGenericLinux);
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
    setDefaultDisplayName(Tr::tr("Deploy to Remote Linux Host"));
    setUseDeploymentDataView();

    const auto needsMakeInstall = [](Target *target)
    {
        const Project * const prj = target->project();
        return prj->deploymentKnowledge() == DeploymentKnowledge::Bad
                && prj->hasMakeInstallEquivalent();
    };
    setPostRestore([needsMakeInstall](DeployConfiguration *dc, const QVariantMap &map) {
        // 4.9 -> 4.10. See QTCREATORBUG-22689.
        if (map.value("_checkMakeInstall").toBool() && needsMakeInstall(dc->target())) {
            auto step = new MakeInstallStep(dc->stepList(), MakeInstallStep::stepId());
            dc->stepList()->insertStep(0, step);
        }
    });

    addInitialStep(Constants::MakeInstallStepId, needsMakeInstall);
    addInitialStep(Constants::KillAppStepId);

    // Todo: Check: Instead of having three different steps here, have one
    // and shift the logic into the implementation there?
    addInitialStep(Constants::RsyncDeployStepId, [](Target *target) {
        return defaultTransferMethod(target->kit()) == FileTransferMethod::Rsync;
    });
    addInitialStep(Constants::DirectUploadStepId, [](Target *target) {
        return defaultTransferMethod(target->kit()) == FileTransferMethod::Sftp;
    });
    addInitialStep(ProjectExplorer::Constants::COPY_FILE_STEP, [](Target *target) {
        return defaultTransferMethod(target->kit()) == FileTransferMethod::GenericCopy;
    });
}

} // RemoteLinux::Internal
