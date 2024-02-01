// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>

namespace Python::Internal {

class PySideUicExtraCompiler;

class PythonRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    PythonRunConfigurationFactory();
};

void setupPythonOutputParser();

} // Python::Internal
