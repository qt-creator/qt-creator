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

QStringList QmakeAndroidSupport::targetData(Core::Id role, const Target *target) const
{
    RunConfiguration *rc = target->activeRunConfiguration();
    if (!rc)
        return QStringList();

    const FileName projectFilePath = FileName::fromString(rc->buildKey());
    const QmakeProject *pro = qobject_cast<QmakeProject *>(target->project());
    QTC_ASSERT(pro, return {});
    QTC_ASSERT(pro->rootProjectNode(), return {});
    const QmakeProFileNode *profileNode = pro->rootProjectNode()->findProFileFor(projectFilePath);
    QTC_ASSERT(profileNode, return {});

    Variable var = {};
    if (role == Android::Constants::AndroidPackageSourceDir)
        var = Variable::AndroidPackageSourceDir;
    else if (role == Android::Constants::AndroidDeploySettingsFile)
        var = Variable::AndroidDeploySettingsFile;
    else if (role == Android::Constants::AndroidExtraLibs)
        var = Variable::AndroidExtraLibs;
    else
        QTC_CHECK(false);

    return profileNode->variableValue(var);
}

QString QmakeAndroidSupport::targetDataItem(Core::Id role, const Target *target) const
{
    const QStringList data = targetData(role, target);
    return data.isEmpty() ? QString() : data.first();
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

        const QString jsonFile = targetDataItem(Android::Constants::AndroidDeploySettingsFile, target);
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

void QmakeAndroidSupport::manifestSaved(const ProjectExplorer::Target *target)
{
    ProjectExplorer::BuildConfiguration *bc = target->activeBuildConfiguration();
    if (auto qbc = qobject_cast<AndroidQmakeBuildConfiguration *>(bc))
        qbc->manifestSaved();
}

} // namespace Internal
} // namespace QmakeAndroidSupport
