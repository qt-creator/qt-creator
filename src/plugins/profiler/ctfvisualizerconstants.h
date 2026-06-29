// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <string>

namespace Profiler::Constants {

inline constexpr char CtfVisualizerMenuId[] = "Analyzer.Menu.CtfVisualizer";
inline constexpr char CtfVisualizerTaskLoadJson[] =
        "Analyzer.Menu.StartAnalyzer.CtfVisualizer.LoadTrace";
inline constexpr char CtfVisualizerTaskLoadCtf2[] =
        "Analyzer.Menu.StartAnalyzer.CtfVisualizer.LoadCtf2Trace";

inline constexpr char CtfVisualizerPerspectiveId[] = "CtfVisualizer.Perspective";

inline constexpr char CtfTraceEventsKey[] = "traceEvents";

inline constexpr char CtfEventNameKey[] = "name";
inline constexpr char CtfEventCategoryKey[] = "cat";
inline constexpr char CtfEventPhaseKey[] = "ph";
inline constexpr char CtfTracingClockTimestampKey[] = "ts";
inline constexpr char CtfProcessIdKey[] = "pid";
inline constexpr char CtfThreadIdKey[] = "tid";
inline constexpr char CtfDurationKey[] = "dur";

inline constexpr char CtfEventTypeBegin[] = "B";
inline constexpr char CtfEventTypeEnd[] = "E";
inline constexpr char CtfEventTypeComplete[] = "X";
inline constexpr char CtfEventTypeMetadata[] = "M";
inline constexpr char CtfEventTypeInstant[] = "i";
inline constexpr char CtfEventTypeInstantDeprecated[] = "I";
inline constexpr char CtfEventTypeCounter[] = "C";

} // namespace Profiler::Constants
