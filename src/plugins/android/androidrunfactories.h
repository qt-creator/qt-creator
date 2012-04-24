/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDRUNFACTORIES_H
#define ANDROIDRUNFACTORIES_H

#include <projectexplorer/runconfiguration.h>
#include <qt4projectmanager/qmakerunconfigurationfactory.h>

namespace ProjectExplorer {
    class RunControl;
    class RunConfigWidget;
    class Target;
}

using ProjectExplorer::IRunControlFactory;
using ProjectExplorer::RunConfiguration;
using ProjectExplorer::RunControl;
using ProjectExplorer::RunConfigWidget;
using ProjectExplorer::Target;

namespace ProjectExplorer { class Node; }

namespace Android {
namespace Internal {

class AndroidRunConfigurationFactory : public Qt4ProjectManager::QmakeRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit AndroidRunConfigurationFactory(QObject *parent = 0);
    ~AndroidRunConfigurationFactory();

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

class AndroidRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    explicit AndroidRunControlFactory(QObject *parent = 0);
    ~AndroidRunControlFactory();

    QString displayName() const;

    bool canRun(RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const;
    RunControl *create(RunConfiguration *runConfiguration,
                       ProjectExplorer::RunMode mode);
};

} // namespace Internal
} // namespace Android

#endif  // ANDROIDRUNFACTORIES_H
