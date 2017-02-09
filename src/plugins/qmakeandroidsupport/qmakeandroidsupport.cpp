/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidpackageinstallationstep.h"
#include "qmakeandroidbuildapkstep.h"
#include "qmakeandroidsupport.h"
#include "androidqmakebuildconfigurationfactory.h"
#include "qmakeandroidrunconfiguration.h"

#include <android/androidconstants.h>
#include <android/androidglobal.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakeproject.h>

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
    QmakeProject *project = qobject_cast<QmakeProject*>(target->project());
    Q_ASSERT(project);
    if (!project)
        return res;

    foreach (QmakeProFileNode *node, project->allProFiles()) {
        TargetInformation info = node->targetInformation();
        res << info.buildDir;
        QString destDir = info.destDir;
        if (!destDir.isEmpty()) {
            if (QFileInfo(destDir).isRelative())
                destDir = QDir::cleanPath(info.buildDir + QLatin1Char('/') + destDir);
            res << destDir;
        }
    }

    return res;
}

QStringList QmakeAndroidSupport::androidExtraLibs(const ProjectExplorer::Target *target) const
{
    ProjectExplorer::RunConfiguration *rc = target->activeRunConfiguration();
    QmakeAndroidRunConfiguration *qarc = qobject_cast<QmakeAndroidRunConfiguration *>(rc);
    if (!qarc)
        return QStringList();
    auto project = static_cast<QmakeProject *>(target->project());
    QmakeProFileNode *node = project->rootProjectNode()->findProFileFor(qarc->proFilePath());
    return node->variableValue(QmakeProjectManager::Variable::AndroidExtraLibs);
}

QStringList QmakeAndroidSupport::projectTargetApplications(const ProjectExplorer::Target *target) const
{
    QStringList apps;
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(target->project());
    if (!qmakeProject)
        return apps;
    foreach (QmakeProFileNode *proFile, qmakeProject->applicationProFiles()) {
        if (proFile->projectType() == ProjectType::ApplicationTemplate) {
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

Utils::FileName QmakeAndroidSupport::androiddeployqtPath(ProjectExplorer::Target *target) const
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!version)
        return Utils::FileName();

    QString command = version->qmakeProperty("QT_HOST_BINS");
    if (!command.endsWith(QLatin1Char('/')))
        command += QLatin1Char('/');
    command += Utils::HostOsInfo::withExecutableSuffix(QLatin1String("androiddeployqt"));
    return Utils::FileName::fromString(command);
}

Utils::FileName QmakeAndroidSupport::androiddeployJsonPath(ProjectExplorer::Target *target) const
{
    const auto *pro = static_cast<QmakeProject *>(target->project());
    QmakeAndroidBuildApkStep *buildApkStep
        = Android::AndroidGlobal::buildStep<QmakeAndroidBuildApkStep>(target->activeBuildConfiguration());

    if (!buildApkStep) // should never happen
        return Utils::FileName();

    const QmakeProFileNode *node =
            pro->rootProjectNode()->findProFileFor(buildApkStep->proFilePathForInputFile());
    if (!node) // should never happen
        return Utils::FileName();

    QString inputFile = node->singleVariableValue(Variable::AndroidDeploySettingsFile);
    if (inputFile.isEmpty()) // should never happen
        return Utils::FileName();

    return Utils::FileName::fromString(inputFile);
}

void QmakeAndroidSupport::manifestSaved(const ProjectExplorer::Target *target)
{
    ProjectExplorer::BuildConfiguration *bc = target->activeBuildConfiguration();
    if (auto qbc = qobject_cast<AndroidQmakeBuildConfiguration *>(bc))
        qbc->manifestSaved();
}

Utils::FileName QmakeAndroidSupport::manifestSourcePath(const ProjectExplorer::Target *target)
{
    ProjectExplorer::RunConfiguration *rc = target->activeRunConfiguration();
    if (auto qrc = qobject_cast<QmakeAndroidRunConfiguration *>(rc)) {
        Utils::FileName proFilePath = qrc->proFilePath();
        const auto project = static_cast<QmakeProjectManager::QmakeProject *>(target->project());
        const QmakeProFileNode *node = project->rootProjectNode()->findProFileFor(proFilePath);
        if (node) {
            QString packageSource = node->singleVariableValue(Variable::AndroidPackageSourceDir);
            if (!packageSource.isEmpty()) {
                Utils::FileName manifest = Utils::FileName::fromUserInput(packageSource + QLatin1String("/AndroidManifest.xml"));
                if (manifest.exists())
                    return manifest;
            }
        }
    }
    return Utils::FileName();
}

} // namespace Internal
} // namespace QmakeAndroidSupport
