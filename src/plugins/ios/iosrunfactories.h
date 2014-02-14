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
#ifndef IOSRUNFACTORIES_H
#define IOSRUNFACTORIES_H

#include <projectexplorer/runconfiguration.h>
#include <qmakeprojectmanager/qmakerunconfigurationfactory.h>
#include <utils/qtcoverride.h>

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

    QString displayNameForId(const Core::Id id) const QTC_OVERRIDE;
    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const QTC_OVERRIDE;

    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const QTC_OVERRIDE;

    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const QTC_OVERRIDE;

    bool canClone(ProjectExplorer::Target *parent,
                                  ProjectExplorer::RunConfiguration *source) const QTC_OVERRIDE;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                                             ProjectExplorer::RunConfiguration *source) QTC_OVERRIDE;

    bool canHandle(ProjectExplorer::Target *t) const QTC_OVERRIDE;
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        ProjectExplorer::Node *n
                                                                        ) QTC_OVERRIDE;
private:
    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent,
                                                const Core::Id id) QTC_OVERRIDE;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) QTC_OVERRIDE;
};

class IosRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    explicit IosRunControlFactory(QObject *parent = 0);

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const QTC_OVERRIDE;
    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                       ProjectExplorer::RunMode mode,
                       QString *errorMessage) QTC_OVERRIDE;
private:
    mutable QMap<Core::Id, QPointer<ProjectExplorer::RunControl> > m_activeRunControls;
};

} // namespace Internal
} // namespace Ios

#endif  // IOSRUNFACTORIES_H
