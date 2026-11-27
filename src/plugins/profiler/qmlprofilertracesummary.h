// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatisticsmodel.h"
#include "quick3dmodel.h"
#include "scenegraphtimelinemodel.h"

#include <QJsonObject>
#include <QMap>

#include <array>
#include <chrono>
#include <optional>

namespace Profiler::Internal {

class QmlTraceSummaryOptions
{
public:
    enum SortBy { TotalTime, SelfTime, Calls, MaxTime, MedianTime };

    // Relative to the start of the trace. Unset means the whole trace.
    std::optional<std::chrono::nanoseconds> start;
    std::optional<std::chrono::nanoseconds> end;
    int limit = 20;
    SortBy sortBy = TotalTime;
    int targetFrameRate = 60;
};

class QmlTraceSummary
{
public:
    QmlTraceSummary(const QmlProfilerModelManager &manager, const QmlTraceSummaryOptions &options);

    QmlProfilerModelManager::QmlEventLoader loader();
    QJsonObject result();

    class StageStats
    {
    public:
        void add(std::chrono::nanoseconds duration);

        qint64 count = 0;
        std::chrono::nanoseconds total{0};
        std::chrono::nanoseconds maximum{0};
    };

private:
    void load(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type);
    void loadSceneGraph(const QmlDebug::QmlEvent &event, int detailType);

    const QmlProfilerModelManager &m_manager;
    QmlTraceSummaryOptions m_options;
    std::chrono::nanoseconds m_traceStart{0};
    std::chrono::nanoseconds m_rangeStart{0};
    std::chrono::nanoseconds m_rangeEnd{0};
    bool m_restricted = false;

    QmlRangeStatistics m_ranges;
    std::array<StageStats, SceneGraphTimelineModel::MaximumSceneGraphStage> m_sceneGraph;
    std::array<StageStats, Quick3DModel::EventData> m_quick3D;
    QMap<qint32, int> m_frameRates;
    std::chrono::duration<double, std::milli> m_slowestFrameTime{0};
    std::chrono::nanoseconds m_slowestFrameTimestamp{-1};
};

// Replays the trace in this thread.
QJsonObject qmlTraceSummary(const QmlProfilerModelManager &manager,
                            const QmlTraceSummaryOptions &options);

} // namespace Profiler::Internal
