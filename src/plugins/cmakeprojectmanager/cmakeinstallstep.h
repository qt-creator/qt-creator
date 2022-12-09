// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeabstractprocessstep.h"

namespace Utils {
class CommandLine;
class StringAspect;
} // namespace Utils

namespace CMakeProjectManager::Internal {

class CMakeInstallStep : public CMakeAbstractProcessStep
{
    Q_OBJECT

public:
    CMakeInstallStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

private:
    Utils::CommandLine cmakeCommand() const;

    void processFinished(bool success) override;

    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    QWidget *createConfigWidget() override;

    friend class CMakeInstallStepConfigWidget;
    Utils::StringAspect *m_cmakeArguments = nullptr;
};

class CMakeInstallStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    CMakeInstallStepFactory();
};

} // namespace CMakeProjectManager::Internal
