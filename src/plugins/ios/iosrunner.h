// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace Ios::Internal {

class IosRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    IosRunWorkerFactory();
};

class IosDebugWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    IosDebugWorkerFactory();
};

class IosQmlProfilerWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    IosQmlProfilerWorkerFactory();
};

} // Ios::Internal
