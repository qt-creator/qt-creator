// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

namespace McuSupport {
namespace Internal {

class McuSupportRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    McuSupportRunConfigurationFactory();
};

ProjectExplorer::RunWorkerFactory::WorkerCreator makeFlashAndRunWorker();

} // namespace Internal
} // namespace McuSupport
