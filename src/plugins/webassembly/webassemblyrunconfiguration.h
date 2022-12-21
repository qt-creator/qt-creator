// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

namespace WebAssembly {
namespace Internal {

class EmrunRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    EmrunRunConfigurationFactory();
};

ProjectExplorer::RunWorkerFactory::WorkerCreator makeEmrunWorker();

} // namespace Internal
} // namespace Webassembly
