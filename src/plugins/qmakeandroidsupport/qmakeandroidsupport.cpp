/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidpackageinstallationstep.h"
#include "qmakeandroidsupport.h"

#include <android/androidconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakestep.h>

using namespace QmakeProjectManager;

namespace QmakeAndroidSupport {
namespace Internal {

bool QmakeAndroidSupport::canHandle(const ProjectExplorer::Target *target) const
{
    return qobject_cast<QmakeProject*>(target->project());
}

QStringList QmakeAndroidSupport::soLibSearchPath(const ProjectExplorer::Target *target) const
{
    QStringList res;
    QmakeBuildConfiguration *bc = qobject_cast<QmakeBuildConfiguration*>(target->activeBuildConfiguration());
    QmakeProject *project = qobject_cast<QmakeProject*>(target->project());
    Q_ASSERT(project);
    if (!project)
        return res;

    foreach (QmakeProFileNode *node, project->allProFiles()) {
        res  << node->buildDir(bc);
    }

    return res;
}

QStringList QmakeAndroidSupport::projectTargetApplications(const ProjectExplorer::Target *target) const
{
    QStringList apps;
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(target->project());
    if (!qmakeProject)
        return apps;
    foreach (QmakeProFileNode *proFile, qmakeProject->applicationProFiles()) {
        if (proFile->projectType() == ApplicationTemplate) {
            if (proFile->targetInformation().target.startsWith(QLatin1String("lib"))
                    && proFile->targetInformation().target.endsWith(QLatin1String(".so")))
                apps << proFile->targetInformation().target.mid(3, proFile->targetInformation().target.lastIndexOf(QLatin1Char('.')) - 3);
            else
                apps << proFile->targetInformation().target;
        }
    }
    apps.sort();
    return apps;

}

Utils::FileName QmakeAndroidSupport::apkPath(ProjectExplorer::Target *target, AndroidQtSupport::BuildType buildType) const
{
    QString packageName = QLatin1String("QtApp");
    QString buildTypeName;
    if (buildType == DebugBuild)
        buildTypeName = QLatin1String("debug");
    else if (buildType == ReleaseBuildUnsigned)
        buildTypeName =QLatin1String("release-unsigned");
    else
        buildTypeName = QLatin1String("release");

    return target->activeBuildConfiguration()->buildDirectory()
            .appendPath(QLatin1String(Android::Constants::ANDROID_BUILDDIRECTORY))
            .appendPath(QLatin1String("bin"))
            .appendPath(QString::fromLatin1("%1-%2.apk")
                        .arg(packageName)
                        .arg(buildTypeName));
}

void QmakeAndroidSupport::resetBuild(const ProjectExplorer::Target *target)
{
    QmakeBuildConfiguration *bc = qobject_cast<QmakeBuildConfiguration*>(target->activeBuildConfiguration());
    if (!bc)
        return;

    QMakeStep *qs = bc->qmakeStep();
    if (!qs)
        return;

    qs->setForced(true);

    ProjectExplorer::BuildManager::buildList(bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN),
                  ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    ProjectExplorer::BuildManager::appendStep(qs, ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    bc->setSubNodeBuild(0);
    // Make the buildconfiguration emit a evironmentChanged() signal
    // TODO find a better way
    bool use = bc->useSystemEnvironment();
    bc->setUseSystemEnvironment(!use);
    bc->setUseSystemEnvironment(use);
}

} // namespace Internal
} // namespace QmakeProjectManager
