/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
** info@kdab.com, author Tim Henning <tim.henning@kdab.com>
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

#include <string>

namespace CtfVisualizer {
namespace Constants {

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

} // namespace Constants
} // namespace CtfVisualizer
