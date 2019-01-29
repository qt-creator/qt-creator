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
#include <projectexplorer/target.h>

namespace Nim {

class NimCompilerBuildStep;

class NimBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;
    NimBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);

    void initialize(const ProjectExplorer::BuildInfo &info) override;
    ProjectExplorer::NamedWidget *createConfigWidget() override;
    ProjectExplorer::BuildConfiguration::BuildType buildType() const override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

public:
    Utils::FileName cacheDirectory() const;
    Utils::FileName outFilePath() const;

signals:
    void outFilePathChanged(const Utils::FileName &outFilePath);

private:
    void setupBuild(const ProjectExplorer::BuildInfo *info);
    const NimCompilerBuildStep *nimCompilerBuildStep() const;
};


class NimBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
    Q_OBJECT

public:
    NimBuildConfigurationFactory();

private:
    QList<ProjectExplorer::BuildInfo> availableBuilds(const ProjectExplorer::Target *parent) const override;

    QList<ProjectExplorer::BuildInfo> availableSetups(const ProjectExplorer::Kit *k,
                                                      const QString &projectPath) const override;

    ProjectExplorer::BuildInfo createBuildInfo(const ProjectExplorer::Kit *k,
                                               ProjectExplorer::BuildConfiguration::BuildType buildType) const;

    QString displayName(ProjectExplorer::BuildConfiguration::BuildType buildType) const;
};

}
