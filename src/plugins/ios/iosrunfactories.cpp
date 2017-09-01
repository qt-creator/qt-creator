/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "iosrunfactories.h"

#include "iosconstants.h"
#include "iosrunconfiguration.h"
#include "iosmanager.h"

#include <debugger/analyzer/analyzermanager.h>
#include <debugger/debuggerconstants.h>

#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

#define IOS_PREFIX "Qt4ProjectManager.IosRunConfiguration"
#define IOS_RC_ID_PREFIX IOS_PREFIX ":"

static Utils::FileName pathFromId(Core::Id id)
{
    QString pathStr = id.toString();
    const QString prefix = QLatin1String(IOS_RC_ID_PREFIX);
    if (!pathStr.startsWith(prefix))
        return Utils::FileName();
    return Utils::FileName::fromString(pathStr.mid(prefix.size()));
}

IosRunConfigurationFactory::IosRunConfigurationFactory(QObject *parent)
    : QmakeRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("IosRunConfigurationFactory"));
}

bool IosRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return availableCreationIds(parent).contains(id);
}

bool IosRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    QString id = ProjectExplorer::idFromMap(map).toString();
    return id.startsWith(QLatin1String(IOS_RC_ID_PREFIX));
}

bool IosRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QList<Core::Id> IosRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    if (!IosManager::supportsIos(parent))
        return QList<Core::Id>();

    auto project = static_cast<QmakeProject *>(parent->project());
    return project->creationIds(IOS_RC_ID_PREFIX, mode, {ProjectType::ApplicationTemplate,
                                                         ProjectType::SharedLibraryTemplate,
                                                         ProjectType::AuxTemplate});
}

QString IosRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return pathFromId(id).toFileInfo().completeBaseName();
}

RunConfiguration *IosRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    return cloneHelper<IosRunConfiguration>(parent, source);
}

bool IosRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return IosManager::supportsIos(t);
}

QList<RunConfiguration *> IosRunConfigurationFactory::runConfigurationsForNode(Target *t, const Node *n)
{
    QList<RunConfiguration *> result;
    foreach (RunConfiguration *rc, t->runConfigurations()) {
        if (IosRunConfiguration *qt4c = qobject_cast<IosRunConfiguration *>(rc)) {
            if (qt4c->profilePath() == n->filePath())
                result << rc;
        }
    }
    return result;
}

RunConfiguration *IosRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    return createHelper<IosRunConfiguration>(parent, id, pathFromId(id));
}

RunConfiguration *IosRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    Core::Id id = ProjectExplorer::idFromMap(map);
    return createHelper<IosRunConfiguration>(parent, id, pathFromId(id));
}

} // namespace Internal
} // namespace Ios
