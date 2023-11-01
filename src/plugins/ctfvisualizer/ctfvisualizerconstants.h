// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <string>

namespace CtfVisualizer::Constants {

const char CtfVisualizerMenuId[] = "Analyzer.Menu.CtfVisualizer";
const char CtfVisualizerTaskLoadJson[] =
        "Analyzer.Menu.StartAnalyzer.CtfVisualizer.LoadTrace";

const char CtfVisualizerPerspectiveId[] = "CtfVisualizer.Perspective";

const char CtfTraceEventsKey[] = "traceEvents";

const char CtfEventNameKey[] = "name";
const char CtfEventCategoryKey[] = "cat";
const char CtfEventPhaseKey[] = "ph";
const char CtfTracingClockTimestampKey[] = "ts";
const char CtfProcessIdKey[] = "pid";
const char CtfThreadIdKey[] = "tid";
const char CtfDurationKey[] = "dur";

const char CtfEventTypeBegin[] = "B";
const char CtfEventTypeEnd[] = "E";
const char CtfEventTypeComplete[] = "X";
const char CtfEventTypeMetadata[] = "M";
const char CtfEventTypeInstant[] = "i";
const char CtfEventTypeInstantDeprecated[] = "I";
const char CtfEventTypeCounter[] = "C";

} // namespace CtfVisualizer::Constants
