// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>

namespace Nim {

class NimCompilerBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    enum DefaultBuildOptions { Empty = 0, Debug, Release};

    NimCompilerBuildStep(ProjectExplorer::BuildStepList *parentList, Utils::Id id);

    void setBuildType(ProjectExplorer::BuildConfiguration::BuildType buildType);
    Utils::FilePath outFilePath() const;

private:
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    QWidget *createConfigWidget() override;

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    void updateTargetNimFile();
    Utils::CommandLine commandLine();

    DefaultBuildOptions m_defaultOptions;
    QStringList m_userCompilerOptions;
    Utils::FilePath m_targetNimFile;
};

class NimCompilerBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    NimCompilerBuildStepFactory();
};

} // Nim
