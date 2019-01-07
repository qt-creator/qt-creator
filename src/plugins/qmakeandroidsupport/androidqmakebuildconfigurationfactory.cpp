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

#include "androidqmakebuildconfigurationfactory.h"

#include <android/androidbuildapkstep.h>
#include <android/androidconfigurations.h>
#include <android/androidconstants.h>
#include <android/androidmanager.h>
#include <android/androidpackageinstallationstep.h>

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace Android;
using namespace ProjectExplorer;

namespace QmakeAndroidSupport {
namespace Internal {

// AndroidQmakeBuildConfigurationFactory

AndroidQmakeBuildConfigurationFactory::AndroidQmakeBuildConfigurationFactory()
{
    registerBuildConfiguration<AndroidQmakeBuildConfiguration>(QmakeProjectManager::Constants::QMAKE_BC_ID);
    setSupportedTargetDeviceTypes({Android::Constants::ANDROID_DEVICE_TYPE});
    setBasePriority(1);
}

// AndroidQmakeBuildConfiguration

AndroidQmakeBuildConfiguration::AndroidQmakeBuildConfiguration(Target *target, Core::Id id)
    : QmakeBuildConfiguration(target, id)
{
    updateCacheAndEmitEnvironmentChanged();
    connect(target->project(), &Project::parsingFinished, this, [this] {
        AndroidManager::updateGradleProperties(BuildConfiguration::target());
    });
}

void AndroidQmakeBuildConfiguration::initialize(const BuildInfo *info)
{
    QmakeBuildConfiguration::initialize(info);

    BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    buildSteps->appendStep(new AndroidPackageInstallationStep(buildSteps));
    buildSteps->appendStep(new Android::AndroidBuildApkStep(buildSteps));

    updateCacheAndEmitEnvironmentChanged();
}

void AndroidQmakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    QString androidNdkPlatform = AndroidConfigurations::currentConfig().bestNdkPlatformMatch(
                qMax(AndroidManager::minimumNDK(target()), AndroidManager::minimumSDK(target())));
    env.set(QLatin1String("ANDROID_NDK_PLATFORM"), androidNdkPlatform);
}

} // namespace Internal
} // namespace QmakeAndroidSupport
