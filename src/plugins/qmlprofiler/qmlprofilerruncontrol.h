// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ProjectExplorer {
class RunControl;
class RunWorker;
}
namespace Tasking { class Group; }

namespace QmlProfiler::Internal {

Tasking::Group qmlProfilerRecipe(ProjectExplorer::RunControl *runControl);

ProjectExplorer::RunWorker *createLocalQmlProfilerWorker(ProjectExplorer::RunControl *runControl);

void setupQmlProfilerRunning();

} // QmlProfiler::Internal
