/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QMLPROFILERCONSTANTS_H
#define QMLPROFILERCONSTANTS_H

namespace QmlProfiler {
namespace Constants {

const char ATTACH[] = "Menu.Analyzer.Attach";
const char TASK_LOAD[] = "QmlProfiler.TaskLoad";
const char TASK_SAVE[] = "QmlProfiler.TaskSave";
const char FLUSH_ENABLED[] = "Analyzer.QmlProfiler.FlushEnabled";
const char FLUSH_INTERVAL[] = "Analyzer.QmlProfiler.FlushInterval";
const char LAST_TRACE_FILE[] = "Analyzer.QmlProfiler.LastTraceFile";
const char AGGREGATE_TRACES[] = "Analyzer.QmlProfiler.AggregateTraces";
const char SETTINGS[] = "Analyzer.QmlProfiler.Settings";
const char ANALYZER[] = "Analyzer";

const char TraceFileExtension[] = ".qtd";

const char QmlProfilerPerspectiveId[]  = "QmlProfiler.Perspective";
const char QmlProfilerTimelineDockId[] = "QmlProfiler.Timeline.Dock";
const char QmlProfilerLocalActionId[]  = "QmlProfiler.Local";
const char QmlProfilerRemoteActionId[] = "QmlProfiler.Remote";

const char QmlProfilerLoadActionId[] =
        "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.LoadQMLTrace";
const char QmlProfilerSaveActionId[] =
        "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.SaveQMLTrace";

} // namespace Constants
} // namespace QmlProfiler

#endif // QMLPROFILERCONSTANTS_H
