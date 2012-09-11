/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef MAEMORUNFACTORIES_H
#define MAEMORUNFACTORIES_H

#include <qt4projectmanager/qmakerunconfigurationfactory.h>

namespace ProjectExplorer {
class RunControl;
class Target;
} // namespace ProjectExplorer

using ProjectExplorer::IRunConfigurationFactory;
using ProjectExplorer::IRunControlFactory;
using ProjectExplorer::RunConfiguration;
using ProjectExplorer::RunConfigWidget;
using ProjectExplorer::RunControl;
using ProjectExplorer::Target;

namespace Madde {
namespace Internal {

class MaemoRunConfigurationFactory : public Qt4ProjectManager::QmakeRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit MaemoRunConfigurationFactory(QObject *parent = 0);
    ~MaemoRunConfigurationFactory();

    QString displayNameForId(const Core::Id id) const;
    QList<Core::Id> availableCreationIds(Target *parent) const;

    bool canCreate(Target *parent, const Core::Id id) const;
    RunConfiguration *create(Target *parent, const Core::Id id);

    bool canRestore(Target *parent, const QVariantMap &map) const;
    RunConfiguration *restore(Target *parent, const QVariantMap &map);

    bool canClone(Target *parent, RunConfiguration *source) const;
    RunConfiguration *clone(Target *parent, RunConfiguration *source);

    bool canHandle(ProjectExplorer::Target *t) const;
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        ProjectExplorer::Node *n);
};

class MaemoRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    explicit MaemoRunControlFactory(QObject *parent = 0);
    ~MaemoRunControlFactory();

    QString displayName() const;

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode) const;
    RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                       ProjectExplorer::RunMode mode,
                       QString *errorMessage);
};

    } // namespace Internal
} // namespace Madde

#endif  // MAEMORUNFACTORIES_H
