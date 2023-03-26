// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>

namespace Haskell {
namespace Internal {

class HaskellBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
public:
    HaskellBuildConfigurationFactory();
};

class HaskellBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    HaskellBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

    ProjectExplorer::NamedWidget *createConfigWidget() override;
    BuildType buildType() const override;
    void setBuildType(BuildType type);

private:
    BuildType m_buildType = BuildType::Release;
};

class HaskellBuildConfigurationWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    HaskellBuildConfigurationWidget(HaskellBuildConfiguration *bc);

private:
    HaskellBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace Haskell
