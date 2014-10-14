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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSRUNFACTORIES_H
#define IOSRUNFACTORIES_H

#include <projectexplorer/runconfiguration.h>
#include <qmakeprojectmanager/qmakerunconfigurationfactory.h>

namespace ProjectExplorer {
class RunControl;
class RunConfigWidget;
class Target;
class Node;
} // namespace ProjectExplorer

namespace Ios {
namespace Internal {

class IosRunConfigurationFactory : public QmakeProjectManager::QmakeRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit IosRunConfigurationFactory(QObject *parent = 0);

    QString displayNameForId(Core::Id id) const Q_DECL_OVERRIDE;
    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode = UserCreate) const Q_DECL_OVERRIDE;

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const Q_DECL_OVERRIDE;

    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const Q_DECL_OVERRIDE;

    bool canClone(ProjectExplorer::Target *parent,
                                  ProjectExplorer::RunConfiguration *source) const Q_DECL_OVERRIDE;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                                             ProjectExplorer::RunConfiguration *source) Q_DECL_OVERRIDE;

    bool canHandle(ProjectExplorer::Target *t) const Q_DECL_OVERRIDE;
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        const ProjectExplorer::Node *n
                                                                        ) Q_DECL_OVERRIDE;
private:
    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent,
                                                Core::Id id) Q_DECL_OVERRIDE;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) Q_DECL_OVERRIDE;
};

class IosRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    explicit IosRunControlFactory(QObject *parent = 0);

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const Q_DECL_OVERRIDE;
    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                       ProjectExplorer::RunMode mode,
                       QString *errorMessage) Q_DECL_OVERRIDE;
private:
    mutable QMap<Core::Id, QPointer<ProjectExplorer::RunControl> > m_activeRunControls;
};

} // namespace Internal
} // namespace Ios

#endif  // IOSRUNFACTORIES_H
