// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ProjectExplorer { class RunControl; }
namespace Tasking { class Group; }

namespace QmlProfiler::Internal {

Tasking::Group qmlProfilerRecipe(ProjectExplorer::RunControl *runControl);
Tasking::Group localQmlProfilerRecipe(ProjectExplorer::RunControl *runControl);

void setupQmlProfilerRunning();

} // QmlProfiler::Internal
