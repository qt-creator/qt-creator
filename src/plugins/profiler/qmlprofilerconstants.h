// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Profiler::Constants {

inline constexpr char TASK_LOAD[] = "QmlProfiler.TaskLoad";
inline constexpr char TASK_SAVE[] = "QmlProfiler.TaskSave";
inline constexpr char SETTINGS[] = "Analyzer.QmlProfiler.Settings";
inline constexpr char ANALYZER[] = "Analyzer";
inline constexpr char TEXT_MARK_CATEGORY[] = "Analyzer.QmlProfiler.TextMark";

inline constexpr int QML_MIN_LEVEL = 1; // Set to 0 to remove the empty line between models in the timeline

inline constexpr char QtdFileExtension[] = ".qtd";
inline constexpr char QztFileExtension[] = ".qzt";

inline constexpr char QmlProfilerPerspectiveId[]  = "QmlProfiler.Perspective";

inline constexpr char QmlProfilerLoadActionId[] =
        "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.LoadQMLTrace";
inline constexpr char QmlProfilerSaveActionId[] =
        "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.SaveQMLTrace";

} // namespace Profiler::Constants
