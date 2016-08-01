/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include <projectexplorer/buildconfiguration.h>

namespace Nim {

class NimBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit NimBuildConfigurationFactory(QObject *parent = 0);

    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;

    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const override;

    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const override;

    bool canRestore(const ProjectExplorer::Target *parent,
                    const QVariantMap &map) const override;

    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;

    bool canClone(const ProjectExplorer::Target *parent,
                  ProjectExplorer::BuildConfiguration *product) const override;

    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent,
                                               ProjectExplorer::BuildConfiguration *product) override;

    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;

    int priority(const ProjectExplorer::Target *parent) const override;

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    static Utils::FileName defaultBuildDirectory(const ProjectExplorer::Kit *k,
                                                 const QString &projectPath,
                                                 const QString &bc, ProjectExplorer::BuildConfiguration::BuildType buildType);

    ProjectExplorer::BuildInfo *createBuildInfo(const ProjectExplorer::Kit *k,
                                                const QString &projectPath,
                                                ProjectExplorer::BuildConfiguration::BuildType buildType) const;
};

}
