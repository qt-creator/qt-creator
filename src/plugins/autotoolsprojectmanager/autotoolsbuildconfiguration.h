/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include <projectexplorer/buildconfiguration.h>

namespace Utils { class FileName; }

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsTarget;
class AutotoolsBuildConfigurationFactory;

class AutotoolsBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class AutotoolsBuildConfigurationFactory;

public:
    explicit AutotoolsBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget() override;

    BuildType buildType() const override;

protected:
    AutotoolsBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    AutotoolsBuildConfiguration(ProjectExplorer::Target *parent, AutotoolsBuildConfiguration *source);
};

class AutotoolsBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit AutotoolsBuildConfigurationFactory(QObject *parent = 0);

    int priority(const ProjectExplorer::Target *parent) const override;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const override;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const override;

    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const override;
    AutotoolsBuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) override;
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    AutotoolsBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    ProjectExplorer::BuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const Utils::FileName &buildDir) const;
};

} // namespace Internal
} // namespace AutotoolsProjectManager
