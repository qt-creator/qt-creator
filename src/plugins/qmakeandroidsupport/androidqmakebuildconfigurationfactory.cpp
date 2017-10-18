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
#include "qmakeandroidbuildapkstep.h"
#include "androidpackageinstallationstep.h"

#include <android/androidmanager.h>
#include <android/androidconfigurations.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakebuildinfo.h>
#include <qmakeprojectmanager/qmakeproject.h>

using namespace Android;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace QmakeAndroidSupport {
namespace Internal {

int AndroidQmakeBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    if (QmakeBuildConfigurationFactory::priority(k, projectPath) >= 0
            && Android::AndroidManager::supportsAndroid(k))
        return 1;
    return -1;
}

int AndroidQmakeBuildConfigurationFactory::priority(const Target *parent) const
{
    if (QmakeBuildConfigurationFactory::priority(parent) >= 0
            && Android::AndroidManager::supportsAndroid(parent))
        return 1;
    return -1;
}

BuildConfiguration *AndroidQmakeBuildConfigurationFactory::create(Target *parent,
                                                                  const BuildInfo *info) const
{
    auto qmakeInfo = static_cast<const QmakeBuildInfo *>(info);
    auto bc = new AndroidQmakeBuildConfiguration(parent);
    configureBuildConfiguration(parent, bc, qmakeInfo);

    BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    buildSteps->insertStep(2, new AndroidPackageInstallationStep(buildSteps));
    buildSteps->insertStep(3, new QmakeAndroidBuildApkStep(buildSteps));
    return bc;
}

BuildConfiguration *AndroidQmakeBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    auto *oldbc = static_cast<AndroidQmakeBuildConfiguration *>(source);
    return new AndroidQmakeBuildConfiguration(parent, oldbc);
}

BuildConfiguration *AndroidQmakeBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    auto bc = new AndroidQmakeBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}


AndroidQmakeBuildConfiguration::AndroidQmakeBuildConfiguration(Target *target)
    : QmakeBuildConfiguration(target)
{
    auto updateGrade = [this] { AndroidManager::updateGradleProperties(BuildConfiguration::target()); };

    auto project = qobject_cast<QmakeProject *>(target->project());
    if (project)
        connect(project, &QmakeProject::proFilesEvaluated, this, updateGrade);
    else
        connect(this, &AndroidQmakeBuildConfiguration::enabledChanged, this, updateGrade);
}

AndroidQmakeBuildConfiguration::AndroidQmakeBuildConfiguration(Target *target, AndroidQmakeBuildConfiguration *source)
    : QmakeBuildConfiguration(target, source)
{
}

AndroidQmakeBuildConfiguration::AndroidQmakeBuildConfiguration(Target *target, Core::Id id)
    : QmakeBuildConfiguration(target, id)
{
}

void AndroidQmakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    m_androidNdkPlatform = AndroidConfigurations::currentConfig().bestNdkPlatformMatch(AndroidManager::minimumSDK(target()));
    env.set(QLatin1String("ANDROID_NDK_PLATFORM"), m_androidNdkPlatform);
}

void AndroidQmakeBuildConfiguration::manifestSaved()
{
    QString androidNdkPlatform = AndroidConfigurations::currentConfig().bestNdkPlatformMatch(AndroidManager::minimumSDK(target()));
    if (m_androidNdkPlatform == androidNdkPlatform)
        return;

    updateCacheAndEmitEnvironmentChanged();

    QMakeStep *qs = qmakeStep();
    if (!qs)
        return;

    qs->setForced(true);

    BuildManager::buildList(stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN),
                            ProjectExplorerPlugin::displayNameForStepId(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    BuildManager::appendStep(qs, ProjectExplorerPlugin::displayNameForStepId(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    setSubNodeBuild(0);
}

} // namespace Internal
} // namespace QmakeAndroidSupport
