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

#include "qmakeandroidbuildapkstep.h"
#include "qmakeandroidsupport.h"
#include "androidqmakebuildconfigurationfactory.h"

#include <android/androidconstants.h>
#include <android/androidglobal.h>

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakeproject.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Utils;

namespace QmakeAndroidSupport {
namespace Internal {

bool QmakeAndroidSupport::canHandle(const ProjectExplorer::Target *target) const
{
    return qobject_cast<QmakeProject*>(target->project());
}

QStringList QmakeAndroidSupport::soLibSearchPath(const ProjectExplorer::Target *target) const
{
    QSet<QString> res;
    QmakeProject *project = qobject_cast<QmakeProject*>(target->project());
    Q_ASSERT(project);
    if (!project)
        return {};

    foreach (QmakeProFile *file, project->allProFiles()) {
        TargetInformation info = file->targetInformation();
        res.insert(info.buildDir.toString());
        Utils::FileName destDir = info.destDir;
        if (!destDir.isEmpty()) {
            if (destDir.toFileInfo().isRelative())
                destDir = Utils::FileName::fromString(QDir::cleanPath(info.buildDir.toString()
                                                                      + '/' + destDir.toString()));
            res.insert(destDir.toString());
        }
        QFile deploymentSettings(androiddeployJsonPath(target).toString());
        if (deploymentSettings.open(QIODevice::ReadOnly)) {
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(deploymentSettings.readAll(), &error);
            if (error.error != QJsonParseError::NoError)
                continue;

            auto rootObj = doc.object();
            auto it = rootObj.find("stdcpp-path");
            if (it != rootObj.constEnd())
                res.insert(QFileInfo(it.value().toString()).absolutePath());
        }
    }
    return res.toList();
}

QStringList QmakeAndroidSupport::androidExtraLibs(const ProjectExplorer::Target *target) const
{
    ProjectExplorer::RunConfiguration *rc = target->activeRunConfiguration();
    if (!rc)
        return QStringList();
    auto project = static_cast<QmakeProject *>(target->project());
    QmakeProFileNode *node =
            project->rootProjectNode()->findProFileFor(Utils::FileName::fromString(rc->buildKey()));
    return node->variableValue(QmakeProjectManager::Variable::AndroidExtraLibs);
}

QStringList QmakeAndroidSupport::projectTargetApplications(const ProjectExplorer::Target *target) const
{
    QStringList apps;
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(target->project());
    if (!qmakeProject)
        return apps;
    for (QmakeProFile *proFile : qmakeProject->applicationProFiles()) {
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

Utils::FileName QmakeAndroidSupport::androiddeployqtPath(const ProjectExplorer::Target *target) const
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

Utils::FileName QmakeAndroidSupport::androiddeployJsonPath(const ProjectExplorer::Target *target) const
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
    if (ProjectExplorer::RunConfiguration *rc = target->activeRunConfiguration()) {
        const auto project = static_cast<QmakeProjectManager::QmakeProject *>(target->project());
        if (project->rootProjectNode()) {
            const QmakeProFileNode *node =
                    project->rootProjectNode()->findProFileFor(Utils::FileName::fromString(rc->buildKey()));
            if (node) {
                QString packageSource = node->singleVariableValue(Variable::AndroidPackageSourceDir);
                if (!packageSource.isEmpty()) {
                    const auto manifest = Utils::FileName::fromUserInput(packageSource +
                                                                         "/AndroidManifest.xml");
                    if (manifest.exists())
                        return manifest;
                }
            }
        }
    }
    return Utils::FileName();
}

static QmakeProFileNode *activeNodeForTarget(const Target *target)
{
    FileName proFilePathForInputFile;
    if (RunConfiguration *rc = target->activeRunConfiguration())
        proFilePathForInputFile = FileName::fromString(rc->buildKey());
    const auto pro = static_cast<QmakeProject *>(target->project());
    return  pro->rootProjectNode()->findProFileFor(proFilePathForInputFile);
}

QString QmakeAndroidSupport::deploySettingsFile(const Target *target) const
{
    if (QmakeProFileNode *node = activeNodeForTarget(target))
        return node->singleVariableValue(Variable::AndroidDeploySettingsFile);
    return QString();
}

FileName QmakeAndroidSupport::packageSourceDir(const Target *target) const
{
    if (QmakeProFileNode *node = activeNodeForTarget(target)) {
        QFileInfo sourceDirInfo(node->singleVariableValue(Variable::AndroidPackageSourceDir));
        return FileName::fromString(sourceDirInfo.canonicalFilePath());
    }
    return FileName();
}

} // namespace Internal
} // namespace QmakeAndroidSupport
