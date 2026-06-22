// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Profiler::Internal {

class PerfProfilerTraceManager;

// The language a stack frame belongs to, used by the views to tell native C++
// frames apart from QML/JS ones in a merged "native mixed" stack.
enum class FrameKind {
    Native,
    Js
};

// THE seam for native-mixed frame classification. This prototype derives the
// kind from the Symbol "binary" convention (Constants::QmlFrameMarker). When the
// typed Symbol kind field lands, only this function's body changes; every caller
// (timeline colouring, flame graph, statistics) keeps working unchanged.
FrameKind frameKind(const PerfProfilerTraceManager &manager, int locationId);

} // namespace Profiler::Internal
