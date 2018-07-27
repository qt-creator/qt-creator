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

#include "qmakeandroidsupport.h"
#include "androidqmakebuildconfigurationfactory.h"

#include <android/androidbuildapkstep.h>
#include <android/androidconstants.h>
#include <android/androidglobal.h>

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakeproject.h>

#include <utils/qtcassert.h>

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

QVariant QmakeAndroidSupport::targetData(Core::Id role, const Target *target) const
{
    RunConfiguration *rc = target->activeRunConfiguration();
    if (!rc)
        return {};

    const FileName projectFilePath = FileName::fromString(rc->buildKey());
    const QmakeProject *pro = qobject_cast<QmakeProject *>(target->project());
    QTC_ASSERT(pro, return {});
    QTC_ASSERT(pro->rootProjectNode(), return {});
    const QmakeProFileNode *profileNode = pro->rootProjectNode()->findProFileFor(projectFilePath);
    QTC_ASSERT(profileNode, return {});

    if (role == Android::Constants::AndroidPackageSourceDir)
        return profileNode->singleVariableValue(Variable::AndroidPackageSourceDir);
    if (role == Android::Constants::AndroidDeploySettingsFile)
        return profileNode->singleVariableValue(Variable::AndroidDeploySettingsFile);
    if (role == Android::Constants::AndroidExtraLibs)
        return profileNode->variableValue(Variable::AndroidExtraLibs);
    if (role == Android::Constants::AndroidArch)
        return profileNode->singleVariableValue(Variable::AndroidArch);

    QTC_CHECK(false);
    return {};
}

static QmakeProFile *applicationProFile(const Target *target)
{
    ProjectExplorer::RunConfiguration *rc = target->activeRunConfiguration();
    if (!rc)
        return nullptr;
    auto project = static_cast<QmakeProject *>(target->project());
    return project->rootProFile()->findProFile(FileName::fromString(rc->buildKey()));
}

bool QmakeAndroidSupport::parseInProgress(const Target *target) const
{
    QmakeProjectManager::QmakeProFile *pro = applicationProFile(target);
    return !pro || pro->parseInProgress();
}

bool QmakeAndroidSupport::validParse(const Target *target) const
{
    QmakeProjectManager::QmakeProFile *pro = applicationProFile(target);
    return pro->validParse() && pro->projectType() == ProjectType::ApplicationTemplate;
}

bool QmakeAndroidSupport::extraLibraryEnabled(const Target *target) const
{
    QmakeProFile *pro = applicationProFile(target);
    return pro && !pro->parseInProgress();
}

FileName QmakeAndroidSupport::projectFilePath(const Target *target) const
{
    QmakeProFile *pro = applicationProFile(target);
    return pro ? pro->filePath() : FileName();
}

bool QmakeAndroidSupport::setTargetData(Core::Id role, const QVariant &value, const Target *target) const
{
    QmakeProFile *pro = applicationProFile(target);
    if (!pro)
        return false;

    const QString arch = pro->singleVariableValue(Variable::AndroidArch);
    const QString scope = "contains(ANDROID_TARGET_ARCH," + arch + ')';
    auto flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues
               | QmakeProjectManager::Internal::ProWriter::MultiLine;

    if (role == Android::Constants::AndroidExtraLibs)
        return pro->setProVariable("ANDROID_EXTRA_LIBS", value.toStringList(), scope, flags);
    if (role == Android::Constants::AndroidPackageSourceDir)
        return pro->setProVariable("ANDROID_PACKAGE_SOURCE_DIR", {value.toString()}, scope, flags);

    return false;
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

        const QString jsonFile = targetData(Android::Constants::AndroidDeploySettingsFile, target).toString();
        QFile deploymentSettings(jsonFile);
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

void QmakeAndroidSupport::addFiles(const ProjectExplorer::Target *target,
                                   const QString &buildKey,
                                   const QStringList &addedFiles) const
{
    QmakeProject *project = static_cast<QmakeProject *>(target->project());
    QmakeProFile *currentRunNode = project->rootProFile()->findProFile(FileName::fromString(buildKey));
    QTC_ASSERT(currentRunNode, return);
    currentRunNode->addFiles(addedFiles);
}

} // namespace Internal
} // namespace QmakeAndroidSupport
