// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Profiler::Internal {

// A frame slower than half the target frame rate is a perceptible stutter.
constexpr int kStutterFrameBudgets = 2;

// The target frame rate is a whole number up to this, so a frame's verdict can be computed
// for every possible target ahead of time.
constexpr int kMaxTargetFrameRate = 1000;

// The time a frame has at the target frame rate.
inline qint64 frameBudgetNs(int targetFrameRate)
{
    return 1000000000ll / targetFrameRate;
}

class QmlProfilerSettings : public Utils::AspectContainer
{
public:
    QmlProfilerSettings();

    Utils::BoolAspect flushEnabled{this};
    Utils::IntegerAspect flushInterval{this};
    Utils::FilePathAspect lastTraceFile{this};
    Utils::BoolAspect aggregateTraces{this};

    // The frame rate the application is meant to reach. The dashboard rates frames against
    // it, and the findings derive their frame budgets from it.
    Utils::IntegerAspect targetFrameRate{this};

    // Thresholds for the findings rules. Defaults suit desktop traces; embedded targets
    // will want them raised.
    Utils::IntegerAspect findingsCompileThresholdMs{this};
    Utils::IntegerAspect findingsSyncLoadThresholdMs{this};
    Utils::IntegerAspect findingsPeriodicMinCount{this};
    Utils::IntegerAspect findingsPeriodicDeviationPercent{this};
    Utils::DoubleAspect findingsPixmapMegapixels{this};
    Utils::IntegerAspect findingsPerFrameCostPercent{this};
    Utils::IntegerAspect findingsBlockingCallThresholdMs{this};
    Utils::IntegerAspect findingsMemoryChurnKb{this};
    Utils::IntegerAspect findingsCreationMinCount{this};
    Utils::IntegerAspect findingsBindingMinCount{this};
    Utils::IntegerAspect findingsPixmapReloadMinCount{this};
};

QmlProfilerSettings &globalSettings();

QList<int> targetFrameRatePresets();

} // namespace Profiler::Internal
