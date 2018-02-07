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

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace {
    QLatin1String PRO_FILE_KEY("QMakeProjectManager.QmakeAndroidRunConfiguration.ProFile");
}

using namespace ProjectExplorer;
using QmakeProjectManager::QmakeProject;

namespace QmakeAndroidSupport {
namespace Internal {

static const char ANDROID_RC_ID_PREFIX[] = "Qt4ProjectManager.AndroidRunConfiguration:";

QmakeAndroidRunConfiguration::QmakeAndroidRunConfiguration(Target *target)
    : AndroidRunConfiguration(target, ANDROID_RC_ID_PREFIX)
{
    connect(target->project(), &Project::parsingFinished, this, [this]() {
        updateDisplayName();
    });
}

QString QmakeAndroidRunConfiguration::extraId() const
{
    return m_proFilePath.toString();
}

bool QmakeAndroidRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!AndroidRunConfiguration::fromMap(map))
        return false;

    QmakeProject *project = qmakeProject();
    QTC_ASSERT(project, return false);
    const QDir projectDir = QDir(project->projectDirectory().toString());
    m_proFilePath = Utils::FileName::fromUserInput(projectDir.filePath(map.value(PRO_FILE_KEY).toString()));

    QString extraId = ProjectExplorer::idFromMap(map).suffixAfter(id());
    if (!extraId.isEmpty())
        m_proFilePath = Utils::FileName::fromString(extraId);

    return true;
}

QVariantMap QmakeAndroidRunConfiguration::toMap() const
{
    QVariantMap map(AndroidRunConfiguration::toMap());

    const QDir projectDir = QDir(target()->project()->projectDirectory().toString());
    map.insert(PRO_FILE_KEY, projectDir.relativeFilePath(m_proFilePath.toString()));

    return map;
}

void QmakeAndroidRunConfiguration::updateDisplayName()
{
    QmakeProject *project = qmakeProject();
    const QmakeProjectManager::QmakeProFileNode *root = project->rootProjectNode();
    if (root) {
        const QmakeProjectManager::QmakeProFileNode *node = root->findProFileFor(m_proFilePath);
        if (node) // should always be found
            setDefaultDisplayName(node->displayName());
    }
}

QString QmakeAndroidRunConfiguration::disabledReason() const
{
    if (qmakeProject()->isParsing())
        return tr("The .pro file \"%1\" is currently being parsed.")
                .arg(m_proFilePath.fileName());

    if (!qmakeProject()->hasParsingData())
        return qmakeProject()->disabledReasonForRunConfiguration(m_proFilePath);
    return QString();
}

QString QmakeAndroidRunConfiguration::buildSystemTarget() const
{
    return m_proFilePath.toString();
}

QmakeProject *QmakeAndroidRunConfiguration::qmakeProject() const
{
    Target *t = target();
    QTC_ASSERT(t, return nullptr);
    return static_cast<QmakeProject *>(t->project());
}

Utils::FileName QmakeAndroidRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

} // namespace Internal
} // namespace Android
