// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <utils/aspects.h>

namespace Haskell {
namespace Internal {

class HaskellRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    HaskellRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);

private:
    ProjectExplorer::Runnable runnable() const final;
};

class HaskellRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    HaskellRunConfigurationFactory();
};

class HaskellExecutableAspect : public Utils::StringAspect
{
    Q_OBJECT

public:
    HaskellExecutableAspect();
};

} // namespace Internal
} // namespace Haskell
