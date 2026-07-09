// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Core { class PerspectivesView; }

namespace Profiler::Internal {

// The perspective view hosting the profiler tools, shown in the Profiler mode.
Core::PerspectivesView *profilerView();

void setupProfilerMode();
void destroyProfilerMode();

} // namespace Profiler::Internal
