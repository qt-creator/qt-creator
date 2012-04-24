/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmakerunconfigurationfactory.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/target.h>
#include <qtsupport/customexecutablerunconfiguration.h>

namespace Qt4ProjectManager {

QmakeRunConfigurationFactory::QmakeRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{ }

QmakeRunConfigurationFactory *QmakeRunConfigurationFactory::find(ProjectExplorer::Target *t)
{
    if (!t)
        return 0;

    QList<QmakeRunConfigurationFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<QmakeRunConfigurationFactory>();
    foreach (QmakeRunConfigurationFactory *factory, factories) {
        if (factory->canHandle(t))
            return factory;
    }
    return 0;
}

void QmakeRunConfigurationFactory::removeUnconfiguredCustomExectutableRunConfigurations(ProjectExplorer::Target *t)
{
    QList<ProjectExplorer::RunConfiguration*> toRemove;
    // Remove all run configurations which the new project wizard created
    foreach (ProjectExplorer::RunConfiguration * rc, t->runConfigurations()) {
        QtSupport::CustomExecutableRunConfiguration *cerc
                = qobject_cast<QtSupport::CustomExecutableRunConfiguration *>(rc);
        if (cerc && !cerc->isConfigured())
            toRemove.append(rc);
    }
    foreach (ProjectExplorer::RunConfiguration *rc, toRemove)
        t->removeRunConfiguration(rc);
}

} // namespace Qt4ProjectManager
