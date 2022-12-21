// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildconfiguration.h>

namespace GenericProjectManager {
namespace Internal {

class GenericBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;
    GenericBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

    void addToEnvironment(Utils::Environment &env) const final;
};

class GenericBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    GenericBuildConfigurationFactory();
};

} // namespace Internal
} // namespace GenericProjectManager
