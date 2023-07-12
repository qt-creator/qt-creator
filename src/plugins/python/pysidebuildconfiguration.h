// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>

namespace Python::Internal {

class PySideBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    PySideBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    void updatePySideProjectPath(const Utils::FilePath &pySideProjectPath);

private:
    Tasking::GroupItem runRecipe() final;

    Utils::FilePathAspect m_pysideProject{this};
};

class PySideBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    PySideBuildStepFactory();
};

class PySideBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
public:
    PySideBuildConfigurationFactory();
};

} // Python::Internal
