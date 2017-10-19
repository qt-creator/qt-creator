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

class GenericTarget;
class GenericBuildConfigurationFactory;
class GenericBuildSettingsWidget;

class GenericBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class GenericBuildConfigurationFactory;

public:
    explicit GenericBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget() override;

    BuildType buildType() const override;

    void addToEnvironment(Utils::Environment &env) const final;

protected:
    GenericBuildConfiguration(ProjectExplorer::Target *parent, GenericBuildConfiguration *source);
    GenericBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    friend class GenericBuildSettingsWidget;
};

class GenericBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit GenericBuildConfigurationFactory(QObject *parent = nullptr);
    ~GenericBuildConfigurationFactory();

    int priority(const ProjectExplorer::Target *parent) const override;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const override;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const override;

    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const override;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) override;
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    ProjectExplorer::BuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const Utils::FileName &buildDir) const;
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
