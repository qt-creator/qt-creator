/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
const char PerfSampleModeId[]           = "Analyzer.Perf.SampleMode";
const char PerfFrequencyId[]            = "Analyzer.Perf.Frequency";
const char PerfStackSizeId[]            = "Analyzer.Perf.StackSize";
const char PerfCallgraphModeId[]        = "Analyzer.Perf.CallgraphMode";
const char PerfEventsId[]               = "Analyzer.Perf.Events";
const char PerfExtraArgumentsId[]       = "Analyzer.Perf.ExtraArguments";
const char PerfSettingsId[]             = "Analyzer.Perf.Settings";
const char PerfRecordArgumentsId[]      = "Analyzer.Perf.RecordArguments";

const unsigned int PerfDefaultPeriod    = 250;
const unsigned int PerfDefaultStackSize = 4096;
const char PerfCallgraphDwarf[]         = "dwarf";
const char PerfCallgraphFP[]            = "fp";
const char PerfCallgraphLBR[]           = "lbr";
const char PerfSampleFrequency[]        = "-F";
const char PerfSampleCount[]            = "-c";

const char PerfStreamMagic[] = "QPERFSTREAM";
const char PerfZqfileMagic[] = "PTQFILE4.10";

} // namespace Constants
} // namespace PerfProfiler
