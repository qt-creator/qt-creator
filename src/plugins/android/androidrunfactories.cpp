/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunfactories.h"

#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androidanalyzesupport.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidmanager.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <debugger/debuggerconstants.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>


using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace Android {
namespace Internal {

static const char ANDROID_RC_ID_PREFIX[] = "Qt4ProjectManager.AndroidRunConfiguration:";

static QString pathFromId(const Core::Id id)
{
    return id.suffixAfter(ANDROID_RC_ID_PREFIX);
}

AndroidRunConfigurationFactory::AndroidRunConfigurationFactory(QObject *parent)
    : QmakeRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("AndroidRunConfigurationFactory"));
}

bool AndroidRunConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return availableCreationIds(parent).contains(id);
}

bool AndroidRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map).name().startsWith(ANDROID_RC_ID_PREFIX);
}

bool AndroidRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QList<Core::Id> AndroidRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (!AndroidManager::supportsAndroid(parent))
        return ids;
    QList<Qt4ProFileNode *> nodes = static_cast<Qt4Project *>(parent->project())->allProFiles();
    const Core::Id base = Core::Id(ANDROID_RC_ID_PREFIX);
    foreach (Qt4ProFileNode *node, nodes)
        if (node->projectType() == ApplicationTemplate || node->projectType() == LibraryTemplate)
            ids << base.withSuffix(node->targetInformation().target);
    return ids;
}

QString AndroidRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName();
}

RunConfiguration *AndroidRunConfigurationFactory::doCreate(Target *parent, const Core::Id id)
{
    return new AndroidRunConfiguration(parent, id, pathFromId(id));
}

RunConfiguration *AndroidRunConfigurationFactory::doRestore(Target *parent,
                                                            const QVariantMap &map)
{
    Core::Id id = ProjectExplorer::idFromMap(map);
    return new AndroidRunConfiguration(parent, id, pathFromId(id));
}

RunConfiguration *AndroidRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    AndroidRunConfiguration *old = static_cast<AndroidRunConfiguration *>(source);
    return new AndroidRunConfiguration(parent, old);
}

bool AndroidRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return AndroidManager::supportsAndroid(t);
}

QList<RunConfiguration *> AndroidRunConfigurationFactory::runConfigurationsForNode(Target *t, ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, t->runConfigurations())
        if (AndroidRunConfiguration *qt4c = qobject_cast<AndroidRunConfiguration *>(rc))
                if (qt4c->proFilePath() == n->path())
                    result << rc;
    return result;
}

// #pragma mark -- AndroidRunControlFactory

AndroidRunControlFactory::AndroidRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool AndroidRunControlFactory::canRun(RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const
{
    if (mode != NormalRunMode && mode != DebugRunMode && mode != QmlProfilerRunMode)
        return false;
    return qobject_cast<AndroidRunConfiguration *>(runConfiguration);
}

RunControl *AndroidRunControlFactory::create(RunConfiguration *runConfig,
                                        ProjectExplorer::RunMode mode, QString *errorMessage)
{
    Q_ASSERT(canRun(runConfig, mode));
    AndroidRunConfiguration *rc = qobject_cast<AndroidRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    switch (mode) {
    case NormalRunMode:
        return new AndroidRunControl(rc);
    case DebugRunMode:
        return AndroidDebugSupport::createDebugRunControl(rc, errorMessage);
    case QmlProfilerRunMode:
        return AndroidAnalyzeSupport::createAnalyzeRunControl(rc, mode, errorMessage);
    case NoRunMode:
    case DebugRunModeWithBreakOnMain:
    case CallgrindRunMode:
    case MemcheckRunMode:
    default:
        QTC_CHECK(false); // The other run modes are not supported
    }
    return 0;
}

} // namespace Internal
} // namespace Qt4ProjectManager
