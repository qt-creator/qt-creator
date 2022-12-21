// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlProfiler {
namespace Constants {

const char TASK_LOAD[] = "QmlProfiler.TaskLoad";
const char TASK_SAVE[] = "QmlProfiler.TaskSave";
const char SETTINGS[] = "Analyzer.QmlProfiler.Settings";
const char ANALYZER[] = "Analyzer";
const char TEXT_MARK_CATEGORY[] = "Analyzer.QmlProfiler.TextMark";

const int QML_MIN_LEVEL = 1; // Set to 0 to remove the empty line between models in the timeline

const char QtdFileExtension[] = ".qtd";
const char QztFileExtension[] = ".qzt";

const char QmlProfilerPerspectiveId[]  = "QmlProfiler.Perspective";

const char QmlProfilerLoadActionId[] =
        "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.LoadQMLTrace";
const char QmlProfilerSaveActionId[] =
        "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.SaveQMLTrace";

} // namespace Constants
} // namespace QmlProfiler
