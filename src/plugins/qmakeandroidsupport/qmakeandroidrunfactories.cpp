/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

using namespace Android;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace QmakeAndroidSupport {
namespace Internal {

static const char ANDROID_RC_ID_PREFIX[] = "Qt4ProjectManager.AndroidRunConfiguration:";

static Utils::FileName pathFromId(const Core::Id id)
{
    return Utils::FileName::fromString(id.suffixAfter(ANDROID_RC_ID_PREFIX));
}

QmakeAndroidRunConfigurationFactory::QmakeAndroidRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

QString QmakeAndroidRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return pathFromId(id).toFileInfo().completeBaseName();
}

bool QmakeAndroidRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return availableCreationIds(parent).contains(id);
}

bool QmakeAndroidRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

bool QmakeAndroidRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QList<Core::Id> QmakeAndroidRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    QList<QmakeProFileNode *> nodes = project->allProFiles(QList<QmakeProjectType>()
                                                           << ApplicationTemplate
                                                           << SharedLibraryTemplate);

    if (mode == AutoCreate)
        nodes = QmakeProject::nodesWithQtcRunnable(nodes);

    const Core::Id base = Core::Id(ANDROID_RC_ID_PREFIX);
    return QmakeProject::idsForNodes(base, nodes);
}

RunConfiguration *QmakeAndroidRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    if (parent->project()->rootProjectNode())
        return new QmakeAndroidRunConfiguration(parent, id, pathFromId(id));
    return new QmakeAndroidRunConfiguration(parent, id);
}

RunConfiguration *QmakeAndroidRunConfigurationFactory::doRestore(Target *parent,
                                                            const QVariantMap &map)
{
    Core::Id id = ProjectExplorer::idFromMap(map);
    if (parent->project()->rootProjectNode())
        return new QmakeAndroidRunConfiguration(parent, id);
    return new QmakeAndroidRunConfiguration(parent, id);
}

RunConfiguration *QmakeAndroidRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    QmakeAndroidRunConfiguration *old = static_cast<QmakeAndroidRunConfiguration *>(source);
    return new QmakeAndroidRunConfiguration(parent, old);
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
