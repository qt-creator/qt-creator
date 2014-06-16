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

#include "iosrunfactories.h"

#include "iosconstants.h"
#include "iosdebugsupport.h"
#include "iosrunconfiguration.h"
#include "iosruncontrol.h"
#include "iosmanager.h"
#include "iosanalyzesupport.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <debugger/debuggerconstants.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <coreplugin/id.h>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

#define IOS_PREFIX "Qt4ProjectManager.IosRunConfiguration"
#define IOS_RC_ID_PREFIX IOS_PREFIX ":"

static QString pathFromId(const Core::Id id)
{
    return id.suffixAfter(IOS_RC_ID_PREFIX);
}

IosRunConfigurationFactory::IosRunConfigurationFactory(QObject *parent)
    : QmakeRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("IosRunConfigurationFactory"));
}

bool IosRunConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
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

QList<Core::Id> IosRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (!IosManager::supportsIos(parent))
        return ids;
    Core::Id baseId(IOS_RC_ID_PREFIX);
    QmakeProject *project = static_cast<QmakeProject *>(parent->project());

    QList<QmakeProFileNode *> nodes = project->allProFiles(QList<QmakeProjectType>()
                                                           << ApplicationTemplate
                                                           << LibraryTemplate
                                                           << AuxTemplate);

    return QmakeProject::idsForNodes(baseId, nodes);
}

QString IosRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName();
}

RunConfiguration *IosRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    IosRunConfiguration *old = qobject_cast<IosRunConfiguration *>(source);
    return new IosRunConfiguration(parent, old);
}

bool IosRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return IosManager::supportsIos(t);
}

QList<RunConfiguration *> IosRunConfigurationFactory::runConfigurationsForNode(Target *t, const Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, t->runConfigurations())
        if (IosRunConfiguration *qt4c = qobject_cast<IosRunConfiguration *>(rc))
                if (qt4c->profilePath() == n->path())
                    result << rc;
    return result;
}

RunConfiguration *IosRunConfigurationFactory::doCreate(Target *parent, const Core::Id id)
{
    return new IosRunConfiguration(parent, id, pathFromId(id));
}

RunConfiguration *IosRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    Core::Id id = ProjectExplorer::idFromMap(map);
    return new IosRunConfiguration(parent, id, pathFromId(id));
}

IosRunControlFactory::IosRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool IosRunControlFactory::canRun(RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const
{
    if (mode != NormalRunMode && mode != DebugRunMode && mode != QmlProfilerRunMode
            && mode != DebugRunModeWithBreakOnMain)
        return false;
    return qobject_cast<IosRunConfiguration *>(runConfiguration);
}

RunControl *IosRunControlFactory::create(RunConfiguration *runConfig,
                                        ProjectExplorer::RunMode mode, QString *errorMessage)
{
    Q_ASSERT(canRun(runConfig, mode));
    IosRunConfiguration *rc = qobject_cast<IosRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    RunControl *res = 0;
    Core::Id devId = ProjectExplorer::DeviceKitInformation::deviceId(rc->target()->kit());
    // The device can only run an application at a time, if an app is running stop it.
    if (m_activeRunControls.contains(devId)) {
        if (QPointer<ProjectExplorer::RunControl> activeRunControl = m_activeRunControls[devId])
            activeRunControl->stop();
        m_activeRunControls.remove(devId);
    }
    if (mode == NormalRunMode)
        res = new Ios::Internal::IosRunControl(rc);
    else if (mode == QmlProfilerRunMode)
        res = IosAnalyzeSupport::createAnalyzeRunControl(rc, errorMessage);
    else
        res = IosDebugSupport::createDebugRunControl(rc, errorMessage);
    if (devId.isValid())
        m_activeRunControls[devId] = res;
    return res;
}

} // namespace Internal
} // namespace Ios
