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

#pragma once

#include <android/androidrunfactories.h>
#include <projectexplorer/runconfiguration.h>
#include <qmakeprojectmanager/qmakerunconfigurationfactory.h>

namespace ProjectExplorer {
class RunControl;
class RunConfigWidget;
class Target;
class Node;
} // namespace ProjectExplorer

namespace QmakeAndroidSupport {
namespace Internal {

class QmakeAndroidRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit QmakeAndroidRunConfigurationFactory(QObject *parent = 0);

    QString displayNameForId(Core::Id id) const override;
    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode = UserCreate) const override;

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;

    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;

    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) override;

    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        ProjectExplorer::Node *n);

private:
    bool canHandle(ProjectExplorer::Target *t) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;
};

} // namespace Internal
} // namespace QmakeAndroidSupport
