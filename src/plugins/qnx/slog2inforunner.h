// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

#include <solutions/tasking/tasktreerunner.h>

namespace Qnx::Internal {

class Slog2InfoRunner : public ProjectExplorer::RunWorker
{
public:
    explicit Slog2InfoRunner(ProjectExplorer::RunControl *runControl);

    void start() override;
    void stop() override;

private:
    Tasking::TaskTreeRunner m_taskTreeRunner;
    Tasking::Group m_recipe;
};

} // Qnx::Internal
