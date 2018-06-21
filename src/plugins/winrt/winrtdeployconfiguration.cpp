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

#include "winrtdeployconfiguration.h"
#include "winrtpackagedeploymentstep.h"
#include "winrtconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

struct WinRtAppDeployConfiguration : DeployConfiguration
{
    WinRtAppDeployConfiguration(Target *target, Core::Id id) : DeployConfiguration(target, id) {}
    void initialize() { stepList()->appendStep(new WinRtPackageDeploymentStep(stepList())); }
};

WinRtAppDeployConfigurationFactory::WinRtAppDeployConfigurationFactory()
{
    registerDeployConfiguration<WinRtAppDeployConfiguration>("WinRTAppxDeployConfiguration");
    setDefaultDisplayName(QCoreApplication::translate("WinRt::Internal::WinRtDeployConfiguration",
                                                      "Run windeployqt"));
    addSupportedTargetDeviceType(Constants::WINRT_DEVICE_TYPE_LOCAL);
}

struct WinRtPhoneDeployConfiguration : DeployConfiguration
{
    WinRtPhoneDeployConfiguration(Target *target, Core::Id id) : DeployConfiguration(target, id) {}
    void initialize() { stepList()->appendStep(new WinRtPackageDeploymentStep(stepList())); }
};

WinRtPhoneDeployConfigurationFactory::WinRtPhoneDeployConfigurationFactory()
{
    registerDeployConfiguration<WinRtPhoneDeployConfiguration>("WinRTPhoneDeployConfiguration");
    setDefaultDisplayName(QCoreApplication::translate("WinRt::Internal::WinRtDeployConfiguration",
                                                      "Deploy to Windows Phone"));
    addSupportedTargetDeviceType(Constants::WINRT_DEVICE_TYPE_PHONE);
}

struct WinRtEmulatorDeployConfiguration : DeployConfiguration
{
    WinRtEmulatorDeployConfiguration(Target *target, Core::Id id) : DeployConfiguration(target, id) {}
    void initialize() { stepList()->appendStep(new WinRtPackageDeploymentStep(stepList())); }
};

WinRtEmulatorDeployConfigurationFactory::WinRtEmulatorDeployConfigurationFactory()
{
    registerDeployConfiguration<WinRtEmulatorDeployConfiguration>("WinRTEmulatorDeployConfiguration");
    setDefaultDisplayName(QCoreApplication::translate("WinRt::Internal::WinRtDeployConfiguration",
                                                      "Deploy to Windows Phone Emulator"));
    addSupportedTargetDeviceType(Constants::WINRT_DEVICE_TYPE_EMULATOR);
}

WinRtDeployStepFactory::WinRtDeployStepFactory()
{
    registerStep<WinRtPackageDeploymentStep>(Constants::WINRT_BUILD_STEP_DEPLOY);
    setDisplayName(QCoreApplication::translate("WinRt::Internal::WinRtDeployStepFactory", "Run windeployqt"));
    setFlags(BuildStepInfo::Unclonable);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    setRepeatable(false);
}

} // namespace Internal
} // namespace WinRt
