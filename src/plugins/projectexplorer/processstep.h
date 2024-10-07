// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractprocessstep.h"
#include "buildstep.h"

namespace ProjectExplorer::Internal {

class ProcessStep final : public AbstractProcessStep
{
public:
    ProcessStep(BuildStepList *bsl, Utils::Id id);

    void setCommand(const Utils::FilePath &command);
    void setArguments(const QStringList &arguments);
    void setWorkingDirectory(
        const Utils::FilePath &workingDirectory,
        const Utils::FilePath &relativeBasePath = Utils::FilePath());

private:
    void setupOutputFormatter(Utils::OutputFormatter *formatter) final;

    Utils::FilePathAspect m_command{this};
    Utils::StringAspect m_arguments{this};
    Utils::FilePathAspect m_workingDirectory{this};
    Utils::FilePathAspect m_workingDirRelativeBasePath{this};
};

class ProcessStepFactory final : public BuildStepFactory
{
public:
    ProcessStepFactory();
};

} // ProjectExplorer::Internal
