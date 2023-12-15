// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerdeployconfigurationfactory.h"

#include "appmanagerconstants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <remotelinux/remotelinux_constants.h>

using namespace ProjectExplorer;

namespace AppManager {
namespace Internal {

static bool isNecessaryToDeploy(const Target *target)
{
    auto device = DeviceKitAspect::device(target->kit());
    return device && device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

AppManagerDeployConfigurationFactory::AppManagerDeployConfigurationFactory()
{
    setConfigBaseId(Constants::DEPLOYCONFIGURATION_ID);
    setDefaultDisplayName(QCoreApplication::translate("AppManager", "Deploy to AM Device"));
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
    addInitialStep(Constants::MAKE_INSTALL_STEP_ID);
    addInitialStep(Constants::CREATE_PACKAGE_STEP_ID);
    addInitialStep(Constants::DEPLOY_PACKAGE_STEP_ID, isNecessaryToDeploy);
    addInitialStep(Constants::INSTALL_PACKAGE_STEP_ID);
}

} // namespace Internal
} // namespace AppManager

