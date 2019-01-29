/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <projectexplorer/namedwidget.h>

namespace Utils {
class FileName;
class PathChooser;
} // namespace Utils

namespace GenericProjectManager {
namespace Internal {

class GenericBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;
    GenericBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    void initialize(const ProjectExplorer::BuildInfo &info) override;
    ProjectExplorer::NamedWidget *createConfigWidget() override;
    BuildType buildType() const override;
    void addToEnvironment(Utils::Environment &env) const final;
};

class GenericBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
    Q_OBJECT

public:
    GenericBuildConfigurationFactory();
    ~GenericBuildConfigurationFactory() override;

private:
    QList<ProjectExplorer::BuildInfo> availableBuilds(const ProjectExplorer::Target *parent) const override;
    QList<ProjectExplorer::BuildInfo> availableSetups(const ProjectExplorer::Kit *k,
                                                      const QString &projectPath) const override;

    ProjectExplorer::BuildInfo createBuildInfo(const ProjectExplorer::Kit *k, const Utils::FileName &buildDir) const;
};

class GenericBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    GenericBuildSettingsWidget(GenericBuildConfiguration *bc);

private:
    void buildDirectoryChanged();
    void environmentHasChanged();

    Utils::PathChooser *m_pathChooser;
    GenericBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace GenericProjectManager
