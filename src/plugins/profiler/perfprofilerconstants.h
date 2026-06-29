// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Profiler::Constants {

inline constexpr char PerfProfilerTaskLoadPerf[]   =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.LoadPerf";
inline constexpr char PerfProfilerTaskLoadTrace[]  =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.LoadTrace";
inline constexpr char PerfProfilerTaskSaveTrace[]  =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.SaveTrace";
inline constexpr char PerfProfilerTaskLimit[]      =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.LimitToRange";
inline constexpr char PerfProfilerTaskFullRange[]  =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.ShowFullRange";
inline constexpr char PerfProfilerTaskTracePoints[]  =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.CreateTracePoints";

inline constexpr char PerfProfilerTaskSkipDelay[]  = "Analyzer.Perf.SkipDelay";

inline constexpr char TraceFileExtension[]         = ".data";

inline constexpr char PerfProfilerPerspectiveId[]  = "PerfProfiler.Perspective";
inline constexpr char PerfProfilerLocalActionId[]  = "PerfProfiler.Local";
inline constexpr char AnalyzerSettingsGroupId[]    = "Analyzer";

inline constexpr char PerfSettingsId[]             = "Analyzer.Perf.Settings";
inline constexpr char PerfCallgraphDwarf[]         = "dwarf";
inline constexpr char PerfRecordArgsId[]           = "PerfRecordArgsId";

inline constexpr char PerfStreamMagic[] = "QPERFSTREAM";
inline constexpr char PerfZqfileMagic[] = "PTQFILE4.10";

// Native-mixed prototype: a symbol whose "binary" equals this marker denotes a
// QML/JS frame rather than a native one. This is the convention-based stand-in
// for a future typed Symbol kind field; see perfnativemixed.h.
inline constexpr char QmlFrameMarker[] = "[QML]";

// Real traces: QV4's JIT'd code lives in a memfd region named "JITCode:QtQml"
// (see qtdeclarative .../masm/wtf/OSAllocatorPosix.cpp). perf attributes JIT'd
// JS samples to that region, so its presence in a frame's "binary" marks the
// frame as QML/JS without any producer change. perf-map then supplies the JS
// function name; source file/line still needs producer-side work (jitdump).
inline constexpr char QmlJitRegionMarker[] = "JITCode:QtQml";

} // namespace Profiler::Constants
