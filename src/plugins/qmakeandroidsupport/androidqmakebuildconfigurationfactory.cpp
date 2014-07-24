/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidqmakebuildconfigurationfactory.h"
#include "qmakeandroidbuildapkstep.h"
#include "androidpackageinstallationstep.h"

#include <android/androidmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

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

ProjectExplorer::BuildConfiguration *AndroidQmakeBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const
{
    ProjectExplorer::BuildConfiguration *bc = QmakeBuildConfigurationFactory::create(parent, info);
    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    buildSteps->insertStep(2, new AndroidPackageInstallationStep(buildSteps));
    buildSteps->insertStep(3, new QmakeAndroidBuildApkStep(buildSteps));

    return bc;
}

// should the buildconfiguration have its own id?
// and implement restore/clone then?


