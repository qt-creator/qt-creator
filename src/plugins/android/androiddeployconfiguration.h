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

#include <projectexplorer/deployconfiguration.h>

namespace Android {
namespace Internal {

class AndroidDeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
    friend class AndroidDeployConfigurationFactory;

public:
    AndroidDeployConfiguration(ProjectExplorer::Target *parent, Core::Id id);

protected:
    AndroidDeployConfiguration(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source);

};

class AndroidDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit AndroidDeployConfigurationFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, Core::Id id) override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const override;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) override;

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const override;
    // used to translate the ids to names to display to the user
    QString displayNameForId(Core::Id id) const override;
};

} // namespace Internal
} // namespace Android
