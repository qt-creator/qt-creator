// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace PerfProfiler::Internal {

class PerfParserWorker;
class PerfRecordWorker;

class PerfProfilerRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT
public:
    explicit PerfProfilerRunner(ProjectExplorer::RunControl *runControl);

    void start() override;

private:
    PerfParserWorker *m_perfParserWorker = nullptr;
    ProjectExplorer::RunWorker *m_perfRecordWorker = nullptr;
};

class PerfProfilerRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    PerfProfilerRunWorkerFactory();
};

} // PerfProfiler::Internal
