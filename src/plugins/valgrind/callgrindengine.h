// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "valgrindengine.h"

namespace Valgrind::Internal {

class CallgrindToolRunner : public ValgrindToolRunner
{
public:
    explicit CallgrindToolRunner(ProjectExplorer::RunControl *runControl);

    void start() override;

protected:
    void addToolArguments(Utils::CommandLine &cmd) const override;
};

} // namespace Valgrind::Internal
