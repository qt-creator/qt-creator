// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Profiler::Constants {

const char PerfProfilerTaskLoadPerf[]   =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.LoadPerf";
const char PerfProfilerTaskLoadTrace[]  =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.LoadTrace";
const char PerfProfilerTaskSaveTrace[]  =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.SaveTrace";
const char PerfProfilerTaskLimit[]      =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.LimitToRange";
const char PerfProfilerTaskFullRange[]  =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.ShowFullRange";
const char PerfProfilerTaskTracePoints[]  =
        "Analyzer.Menu.StartAnalyzer.PerfProfilerOptions.CreateTracePoints";

const char PerfProfilerTaskSkipDelay[]  = "Analyzer.Perf.SkipDelay";

const char TraceFileExtension[]         = ".data";

const char PerfProfilerPerspectiveId[]  = "PerfProfiler.Perspective";
const char PerfProfilerLocalActionId[]  = "PerfProfiler.Local";
const char AnalyzerSettingsGroupId[]    = "Analyzer";

const char PerfSettingsId[]             = "Analyzer.Perf.Settings";
const char PerfCallgraphDwarf[]         = "dwarf";
const char PerfRecordArgsId[]           = "PerfRecordArgsId";

const char PerfStreamMagic[] = "QPERFSTREAM";
const char PerfZqfileMagic[] = "PTQFILE4.10";

// Native-mixed prototype: a symbol whose "binary" equals this marker denotes a
// QML/JS frame rather than a native one. This is the convention-based stand-in
// for a future typed Symbol kind field; see perfnativemixed.h.
const char QmlFrameMarker[] = "[QML]";

// Real traces: QV4's JIT'd code lives in a memfd region named "JITCode:QtQml"
// (see qtdeclarative .../masm/wtf/OSAllocatorPosix.cpp). perf attributes JIT'd
// JS samples to that region, so its presence in a frame's "binary" marks the
// frame as QML/JS without any producer change. perf-map then supplies the JS
// function name; source file/line still needs producer-side work (jitdump).
const char QmlJitRegionMarker[] = "JITCode:QtQml";

} // namespace Profiler::Constants
