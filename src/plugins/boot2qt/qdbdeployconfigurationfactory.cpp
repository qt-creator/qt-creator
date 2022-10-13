// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qdbdeployconfigurationfactory.h"

#include "qdbconstants.h"

#include <projectexplorer/deploymentdataview.h>
#include "projectexplorer/devicesupport/idevice.h"
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <remotelinux/remotelinux_constants.h>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

QdbDeployConfigurationFactory::QdbDeployConfigurationFactory()
{
    setConfigBaseId(Constants::QdbDeployConfigurationId);
    addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
    setDefaultDisplayName(QCoreApplication::translate("Qdb::Internal::QdbDeployConfiguration",
                                                      "Deploy to Boot2Qt target"));
    setUseDeploymentDataView();

    addInitialStep(RemoteLinux::Constants::MakeInstallStepId, [](Target *target) {
        const Project * const prj = target->project();
        return prj->deploymentKnowledge() == DeploymentKnowledge::Bad
                && prj->hasMakeInstallEquivalent();
    });
    addInitialStep(Qdb::Constants::QdbStopApplicationStepId);
    addInitialStep(RemoteLinux::Constants::RsyncDeployStepId, [](Target *target) {
        auto device = DeviceKitAspect::device(target->kit());
        return device && device->extraData(RemoteLinux::Constants::SupportsRSync).toBool();
    });
    addInitialStep(RemoteLinux::Constants::DirectUploadStepId, [](Target *target) {
        auto device = DeviceKitAspect::device(target->kit());
        return device && !device->extraData(RemoteLinux::Constants::SupportsRSync).toBool();
    });
}

} // namespace Internal
} // namespace Qdb
