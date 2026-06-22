// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfnativemixed.h"

#include "perfprofilerconstants.h"
#include "perfprofilertracemanager.h"

namespace Profiler::Internal {

FrameKind frameKind(const PerfProfilerTraceManager &manager, int locationId)
{
    if (locationId < 0)
        return FrameKind::Native;

    // Resolve the symbol the same way the views do: when addresses are not
    // aggregated the symbol lives on the location reached via symbolLocation(),
    // not on the raw sample location itself.
    const qint32 symbolLocationId = manager.aggregateAddresses()
            ? locationId : manager.symbolLocation(locationId);
    const qint32 binary = manager.symbol(symbolLocationId).binary;
    if (binary < 0)
        return FrameKind::Native;

    return manager.string(binary) == Constants::QmlFrameMarker ? FrameKind::Js
                                                               : FrameKind::Native;
}

} // namespace Profiler::Internal
