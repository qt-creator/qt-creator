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

#include "qmakeandroidrunconfiguration.h"

#include <android/androidconstants.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtkitinformation.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

namespace {
    QLatin1String PRO_FILE_KEY("QMakeProjectManager.QmakeAndroidRunConfiguration.ProFile");
}

using namespace ProjectExplorer;
using QmakeProjectManager::QmakeProject;

namespace QmakeAndroidSupport {
namespace Internal {

QmakeAndroidRunConfiguration::QmakeAndroidRunConfiguration(Target *target, Core::Id id)
    : AndroidRunConfiguration(target, id)
{
    connect(target->project(), &Project::parsingFinished, this, [this]() {
        updateDisplayName();
    });
}

bool QmakeAndroidRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!AndroidRunConfiguration::fromMap(map))
        return false;

    return true;
}

QVariantMap QmakeAndroidRunConfiguration::toMap() const
{
    QVariantMap map(AndroidRunConfiguration::toMap());

    // FIXME: Remove, only left for compatibility in 4.7 development cycle.
    const QDir projectDir = QDir(target()->project()->projectDirectory().toString());
    map.insert(PRO_FILE_KEY, projectDir.relativeFilePath(proFilePath().toString()));

    return map;
}

void QmakeAndroidRunConfiguration::updateDisplayName()
{
    QmakeProject *project = qmakeProject();
    const QmakeProjectManager::QmakeProFileNode *root = project->rootProjectNode();
    if (root) {
        const QmakeProjectManager::QmakeProFileNode *node = root->findProFileFor(proFilePath());
        if (node) { // should always be found
            setDisplayName(node->displayName());
            setDefaultDisplayName(node->displayName());
        }
    }
}

QString QmakeAndroidRunConfiguration::disabledReason() const
{
    if (qmakeProject()->isParsing())
        return tr("The .pro file \"%1\" is currently being parsed.")
                .arg(proFilePath().fileName());

    if (!qmakeProject()->hasParsingData()) {
        if (!proFilePath().exists())
            return tr("The .pro file \"%1\" does not exist.")
                    .arg(proFilePath().fileName());

        QmakeProjectManager::QmakeProFileNode *rootProjectNode = qmakeProject()->rootProjectNode();
        if (!rootProjectNode) // Shutting down
            return QString();

        if (!rootProjectNode->findProFileFor(proFilePath()))
            return tr("The .pro file \"%1\" is not part of the project.")
                    .arg(proFilePath().fileName());

        return tr("The .pro file \"%1\" could not be parsed.")
                .arg(proFilePath().fileName());
    }

    return QString();
}

QmakeProject *QmakeAndroidRunConfiguration::qmakeProject() const
{
    Target *t = target();
    QTC_ASSERT(t, return nullptr);
    return static_cast<QmakeProject *>(t->project());
}

Utils::FileName QmakeAndroidRunConfiguration::proFilePath() const
{
    return Utils::FileName::fromString(buildKey());
}


// QmakeAndroidRunConfigurationFactory

QmakeAndroidRunConfigurationFactory::QmakeAndroidRunConfigurationFactory()
{
    registerRunConfiguration<QmakeAndroidRunConfiguration>
            ("Qt4ProjectManager.AndroidRunConfiguration:");
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    addSupportedTargetDeviceType(Android::Constants::ANDROID_DEVICE_TYPE);
}

} // namespace Internal
} // namespace Android
