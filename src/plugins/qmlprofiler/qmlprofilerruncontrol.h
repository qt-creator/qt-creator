// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>
#include <projectexplorer/projectexplorerconstants.h>

namespace QmlProfiler::Internal {

class QmlProfilerRunner : public ProjectExplorer::RunWorker
{
public:
    QmlProfilerRunner(ProjectExplorer::RunControl *runControl);

private:
    void start() override;
    void stop() override;
};

ProjectExplorer::RunWorker *createLocalQmlProfilerWorker(ProjectExplorer::RunControl *runControl);
void setupQmlProfilerRunning();

} // QmlProfiler::Internal
