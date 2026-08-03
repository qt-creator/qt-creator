// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Profiler::Internal {

class QmlProfilerSettings : public Utils::AspectContainer
{
public:
    QmlProfilerSettings();

    Utils::BoolAspect flushEnabled{this};
    Utils::IntegerAspect flushInterval{this};
    Utils::FilePathAspect lastTraceFile{this};
    Utils::BoolAspect aggregateTraces{this};

    // Thresholds for the findings rules. Defaults suit desktop traces; embedded targets
    // will want them raised.
    Utils::IntegerAspect findingsCompileThresholdMs{this};
    Utils::IntegerAspect findingsSyncLoadThresholdMs{this};
    Utils::IntegerAspect findingsPeriodicMinCount{this};
    Utils::IntegerAspect findingsPeriodicDeviationPercent{this};
    Utils::DoubleAspect findingsPixmapMegapixels{this};
    Utils::IntegerAspect findingsPerFrameBudgetUs{this};
};

QmlProfilerSettings &globalSettings();

} // namespace Profiler::Internal
