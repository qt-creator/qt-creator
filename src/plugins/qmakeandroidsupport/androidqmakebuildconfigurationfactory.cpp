/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidqmakebuildconfigurationfactory.h"
#include "qmakeandroidbuildapkstep.h"
#include "androidpackageinstallationstep.h"

#include <android/androidmanager.h>
#include <android/androidconfigurations.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qmakeprojectmanager/qmakebuildinfo.h>

using namespace QmakeAndroidSupport::Internal;

int AndroidQmakeBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    if (QmakeBuildConfigurationFactory::priority(k, projectPath) >= 0
            && Android::AndroidManager::supportsAndroid(k))
        return 1;
    return -1;
}

int AndroidQmakeBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    if (QmakeBuildConfigurationFactory::priority(parent) >= 0
            && Android::AndroidManager::supportsAndroid(parent))
        return 1;
    return -1;
}

ProjectExplorer::BuildConfiguration *AndroidQmakeBuildConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                                   const ProjectExplorer::BuildInfo *info) const
{
    auto qmakeInfo = static_cast<const QmakeProjectManager::QmakeBuildInfo *>(info);
    AndroidQmakeBuildConfiguration *bc = new AndroidQmakeBuildConfiguration(parent);
    configureBuildConfiguration(parent, bc, qmakeInfo);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    buildSteps->insertStep(2, new AndroidPackageInstallationStep(buildSteps));
    buildSteps->insertStep(3, new QmakeAndroidBuildApkStep(buildSteps));
    return bc;
}

ProjectExplorer::BuildConfiguration *AndroidQmakeBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    AndroidQmakeBuildConfiguration *oldbc(static_cast<AndroidQmakeBuildConfiguration *>(source));
    return new AndroidQmakeBuildConfiguration(parent, oldbc);
}

ProjectExplorer::BuildConfiguration *AndroidQmakeBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AndroidQmakeBuildConfiguration *bc = new AndroidQmakeBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}


AndroidQmakeBuildConfiguration::AndroidQmakeBuildConfiguration(ProjectExplorer::Target *target)
    : QmakeProjectManager::QmakeBuildConfiguration(target)
{

}

AndroidQmakeBuildConfiguration::AndroidQmakeBuildConfiguration(ProjectExplorer::Target *target, AndroidQmakeBuildConfiguration *source)
    : QmakeProjectManager::QmakeBuildConfiguration(target, source)
{

}

AndroidQmakeBuildConfiguration::AndroidQmakeBuildConfiguration(ProjectExplorer::Target *target, Core::Id id)
    : QmakeProjectManager::QmakeBuildConfiguration(target, id)
{

}

void AndroidQmakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    env.set(QLatin1String("ANDROID_NDK_PLATFORM"),
            Android::AndroidConfigurations::currentConfig().bestNdkPlatformMatch(Android::AndroidManager::minimumSDK(target())));
}
