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

#include "qmakeandroidrunfactories.h"
#include "qmakeandroidrunconfiguration.h"

#include <android/androidmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <debugger/debuggerconstants.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

using namespace Android;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace QmakeAndroidSupport {
namespace Internal {

static const char ANDROID_RC_ID_PREFIX[] = "Qt4ProjectManager.AndroidRunConfiguration:";

QmakeAndroidRunConfigurationFactory::QmakeAndroidRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

QString QmakeAndroidRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return QmakeAndroidRunConfiguration::displayNameForId(id);
}

bool QmakeAndroidRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return availableCreationIds(parent).contains(id);
}

bool QmakeAndroidRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map).name().startsWith(ANDROID_RC_ID_PREFIX);
}

bool QmakeAndroidRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QList<Core::Id> QmakeAndroidRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    auto project = static_cast<QmakeProject *>(parent->project());
    return project->creationIds(ANDROID_RC_ID_PREFIX, mode,
                                {ProjectType::ApplicationTemplate, ProjectType::SharedLibraryTemplate});
}

RunConfiguration *QmakeAndroidRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    return createHelper<QmakeAndroidRunConfiguration>(parent, id);
}

RunConfiguration *QmakeAndroidRunConfigurationFactory::doRestore(Target *parent,
                                                            const QVariantMap &map)
{
    Core::Id id = ProjectExplorer::idFromMap(map);
    return createHelper<QmakeAndroidRunConfiguration>(parent, id);
}

RunConfiguration *QmakeAndroidRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return cloneHelper<QmakeAndroidRunConfiguration>(parent, source);
}

bool QmakeAndroidRunConfigurationFactory::canHandle(Target *t) const
{
    return t->project()->supportsKit(t->kit())
            && AndroidManager::supportsAndroid(t)
            && qobject_cast<QmakeProject *>(t->project());
}

#ifdef Q_CC_GCC
#  warning FIX ME !!!
#endif
QList<RunConfiguration *> QmakeAndroidRunConfigurationFactory::runConfigurationsForNode(Target *t, Node *n)
{
    QList<RunConfiguration *> result;
    foreach (RunConfiguration *rc, t->runConfigurations())
        if (QmakeAndroidRunConfiguration *qt4c = qobject_cast<QmakeAndroidRunConfiguration *>(rc))
                if (qt4c->proFilePath() == n->filePath())
                    result << rc;
    return result;
}

} // namespace Internal
} // namespace Android
