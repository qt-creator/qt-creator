// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>

namespace Python::Internal {

class PySideBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
public:
    PySideBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
};

class PySideBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
public:
    PySideBuildConfigurationFactory();
};

class PySideBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    PySideBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    void updatePySideProjectPath(const Utils::FilePath &pySideProjectPath);

private:
    Utils::StringAspect *m_pysideProject;

private:
    void doRun() override;
};

class PySideBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    PySideBuildStepFactory();
};

} // Python::Internal
