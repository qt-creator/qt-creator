// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace PerfProfiler {
namespace Constants {

const char PerfOptionsMenuId[]          = "Analyzer.Menu.PerfOptions";
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

const char PerfStreamMagic[] = "QPERFSTREAM";
const char PerfZqfileMagic[] = "PTQFILE4.10";

} // namespace Constants
} // namespace PerfProfiler
