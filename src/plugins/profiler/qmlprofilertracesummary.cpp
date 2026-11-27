// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertracesummary.h"

#include "qmlprofilerdashboardstats.h"
#include "qmlprofilersettings.h"

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventtype.h>

#include <QFutureInterface>
#include <QJsonArray>

#include <algorithm>
#include <iterator>

using namespace QmlDebug;
using namespace std::chrono;

namespace Profiler::Internal {

using SceneGraph = SceneGraphTimelineModel;

// Machine-readable names: the summary is read by tools, not shown in the UI.
static const char *rangeTypeName(RangeType type)
{
    switch (type) {
    case Compiling: return "compiling";
    case Creating: return "creating";
    case Binding: return "binding";
    case HandlingSignal: return "handling_signal";
    case Javascript: return "javascript";
    default: return "other";
    }
}

static const char *sceneGraphStageNames[] = {
    "polish",
    "wait",
    "gui_thread_sync",
    "animations",
    "render_thread_sync",
    "render",
    "swap",
    "render_preprocess",
    "render_update",
    "render_bind",
    "render_render",
    "material_compile",
    "glyph_render",
    "glyph_upload",
    "texture_bind",
    "texture_convert",
    "texture_swizzle",
    "texture_upload",
    "texture_mipmap",
    "texture_delete",
};
static_assert(std::size(sceneGraphStageNames) == SceneGraph::MaximumSceneGraphStage);

// In the order of Quick3DModel::MessageType, up to EventData.
static const char *quick3DStageNames[] = {
    "render_frame",
    "synchronize_frame",
    "prepare_frame",
    "mesh_load",
    "custom_mesh_load",
    "texture_load",
    "generate_shader",
    "load_shader",
    "particle_update",
    "render_call",
    "render_pass",
};
static_assert(std::size(quick3DStageNames) == Quick3DModel::EventData);

static bool isStatisticsRange(RangeType type)
{
    return type == Compiling || type == Creating || type == Binding || type == HandlingSignal
           || type == Javascript;
}

void QmlTraceSummary::StageStats::add(nanoseconds duration)
{
    ++count;
    total += duration;
    maximum = std::max(maximum, duration);
}

QmlTraceSummary::QmlTraceSummary(const QmlProfilerModelManager &manager,
                                 const QmlTraceSummaryOptions &options)
    : m_manager(manager)
    , m_options(options)
    , m_traceStart(manager.traceStart())
    , m_restricted(options.start || options.end)
{
    const nanoseconds traceEnd = manager.traceEnd();
    m_rangeStart = m_restricted ? std::clamp(m_traceStart + options.start.value_or(0ns),
                                             m_traceStart, traceEnd)
                                : m_traceStart;
    m_rangeEnd = m_restricted ? std::clamp(options.end ? m_traceStart + *options.end : traceEnd,
                                           m_rangeStart, traceEnd)
                              : traceEnd;
    m_options.targetFrameRate = std::clamp(options.targetFrameRate, 1, kMaxTargetFrameRate);
}

/*!
    Returns the loader to replay the trace's events to, restricted to the range the options
    ask for. It may run in another thread, as long as result() is only called once it is done.
*/
QmlProfilerModelManager::QmlEventLoader QmlTraceSummary::loader()
{
    const auto filter = m_restricted ? m_manager.rangeFilter(m_rangeStart, m_rangeEnd)
                                     : m_manager.rangeFilter(-1ns, -1ns);
    return filter([this](const QmlEvent &event, const QmlEventType &type) { load(event, type); });
}

void QmlTraceSummary::loadSceneGraph(const QmlEvent &event, int detailType)
{
    const auto add = [&](int number, SceneGraph::SceneGraphStage stage) {
        // Like the timeline, a stage that took no time did not run.
        if (const nanoseconds duration(event.number<qint64>(number)); duration > 0ns)
            m_sceneGraph[stage].add(duration);
    };
    // The same stages as SceneGraphTimelineModel::loadEvent() shows.
    switch (detailType) {
    case SceneGraphRendererFrame:
        add(0, SceneGraph::RenderPreprocess);
        add(1, SceneGraph::RenderUpdate);
        add(2, SceneGraph::RenderBind);
        add(3, SceneGraph::RenderRender);
        break;
    case SceneGraphAdaptationLayerFrame:
        add(1, SceneGraph::GlyphRender);
        add(2, SceneGraph::GlyphStore);
        break;
    case SceneGraphContextFrame:
        add(0, SceneGraph::Material);
        break;
    case SceneGraphRenderLoopFrame:
        add(0, SceneGraph::RenderThreadSync);
        add(1, SceneGraph::Render);
        add(2, SceneGraph::Swap);
        break;
    case SceneGraphTexturePrepare:
        add(0, SceneGraph::TextureBind);
        add(1, SceneGraph::TextureConvert);
        add(2, SceneGraph::TextureSwizzle);
        add(3, SceneGraph::TextureUpload);
        add(4, SceneGraph::TextureMipmap);
        break;
    case SceneGraphTextureDeletion:
        add(0, SceneGraph::TextureDeletion);
        break;
    case SceneGraphPolishAndSync:
        add(0, SceneGraph::Polish);
        add(1, SceneGraph::Wait);
        add(2, SceneGraph::GUIThreadSync);
        add(3, SceneGraph::Animations);
        break;
    case SceneGraphWindowsAnimations:
        add(0, SceneGraph::Animations);
        break;
    case SceneGraphPolishFrame:
        add(0, SceneGraph::Polish);
        break;
    default:
        break;
    }
}

void QmlTraceSummary::load(const QmlEvent &event, const QmlEventType &type)
{
    if (isStatisticsRange(type.rangeType())) {
        m_ranges.load(event, type, m_manager.numEventTypes());
        return;
    }
    switch (type.feature()) {
    case ProfileSceneGraph:
        loadSceneGraph(event, type.detailType());
        break;
    case ProfileQuick3D:
        if (type.detailType() >= 0 && type.detailType() < Quick3DModel::EventData) {
            const auto numbers = event.numbers<QList<quint64>>();
            if (!numbers.isEmpty())
                m_quick3D[type.detailType()].add(nanoseconds(numbers.first()));
        }
        break;
    case ProfileAnimations: {
        if (type.message() != Event || type.detailType() != AnimationFrame)
            break;
        // As QmlProfilerDashboardStats::loadEvent(): only the GUI thread's animation frames
        // are what ends up on screen.
        if (static_cast<AnimationThread>(event.number<qint32>(2)) != GuiThread)
            break;
        const qint32 framesPerSecond = event.number<qint32>(0);
        ++m_frameRates[framesPerSecond];
        const FrameTime frameTime = frameDuration(framesPerSecond);
        if (frameTime > m_slowestFrameTime) {
            m_slowestFrameTime = frameTime;
            m_slowestFrameTimestamp = event.timestamp() - m_traceStart;
        }
        break;
    }
    default:
        break;
    }
}

static qint64 sortKey(const QmlEventStats &stats, QmlTraceSummaryOptions::SortBy sortBy)
{
    switch (sortBy) {
    case QmlTraceSummaryOptions::TotalTime: return stats.totalNonRecursive().count();
    case QmlTraceSummaryOptions::SelfTime: return stats.self.count();
    case QmlTraceSummaryOptions::Calls: return stats.calls;
    case QmlTraceSummaryOptions::MaxTime: return stats.maximum.count();
    case QmlTraceSummaryOptions::MedianTime: return stats.median.count();
    }
    return 0;
}

static double percentOf(nanoseconds part, nanoseconds whole)
{
    return whole.count() > 0 ? 100.0 * double(part.count()) / double(whole.count()) : 0.0;
}

template<size_t Count>
static QJsonArray stageArray(const std::array<QmlTraceSummary::StageStats, Count> &stages,
                             const char *const (&names)[Count])
{
    QJsonArray result;
    for (size_t i = 0; i < Count; ++i) {
        const QmlTraceSummary::StageStats &stage = stages[i];
        if (stage.count == 0)
            continue;
        result.append(QJsonObject{
            {"stage", QString::fromLatin1(names[i])},
            {"count", stage.count},
            {"total_ns", qint64(stage.total.count())},
            {"average_ns", qint64(stage.total.count() / stage.count)},
            {"max_ns", qint64(stage.maximum.count())},
        });
    }
    return result;
}

static QJsonObject frameSummary(const QMap<qint32, int> &frameRates, int targetFrameRate,
                                FrameTime slowestFrameTime, nanoseconds slowestFrameTimestamp)
{
    QList<std::pair<FrameTime, int>> frameTimes;
    qint64 total = 0;
    for (auto it = frameRates.cbegin(), end = frameRates.cend(); it != end; ++it) {
        frameTimes.append({frameDuration(it.key()), it.value()});
        total += it.value();
    }

    QJsonObject result{{"count", total}, {"target_fps", targetFrameRate}};
    if (total == 0)
        return result;

    // Slowest first.
    std::sort(frameTimes.begin(), frameTimes.end(), [](const auto &a, const auto &b) {
        return a.first > b.first;
    });
    // The frame time that the given fraction of all frames stay within.
    const auto frameTimeAt = [&](double fraction) {
        const qint64 fromFastest = std::min(total - 1, qint64(fraction * double(total)));
        const qint64 fromSlowest = total - 1 - fromFastest;
        qint64 seen = 0;
        for (const auto &[time, count] : std::as_const(frameTimes)) {
            seen += count;
            if (seen > fromSlowest)
                return time.count();
        }
        return frameTimes.last().first.count();
    };

    const FrameRating rating = rateFrames(frameRates, targetFrameRate);
    result.insert("median_fps", medianFrameRate(frameRates));
    result.insert("on_target_frame_ms", frameDuration(targetFrameRate).count());
    result.insert("on_target", rating.onTarget);
    result.insert("near_target", rating.nearTarget);
    result.insert("failed", rating.failed);
    result.insert("stuttering", rating.stuttering);
    result.insert("frame_time_p50_ms", frameTimeAt(0.5));
    result.insert("frame_time_p90_ms", frameTimeAt(0.9));
    result.insert("frame_time_p99_ms", frameTimeAt(0.99));
    result.insert("slowest_frame_ms", slowestFrameTime.count());
    result.insert("slowest_frame_timestamp_ns", qint64(slowestFrameTimestamp.count()));
    return result;
}

/*!
    Summarizes the events replayed to loader() as the Statistics view, the frame rate
    dashboard and the scene graph and Quick3D timelines show them, restricted and sorted as
    the options say.
*/
QJsonObject QmlTraceSummary::result()
{
    m_ranges.finalize();
    const QList<QmlEventStats> &stats = m_ranges.stats();

    struct Row
    {
        int typeIndex;
        qint64 key;
    };
    QList<Row> rows;
    for (int typeIndex = 0; typeIndex < stats.size(); ++typeIndex) {
        if (stats.at(typeIndex).calls > 0)
            rows.append({typeIndex, sortKey(stats.at(typeIndex), m_options.sortBy)});
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
        return a.key > b.key;
    });

    QJsonArray events;
    const int limit = std::max(m_options.limit, 0);
    const nanoseconds rootDuration = m_ranges.rootDuration();
    for (const Row &row : std::as_const(rows)) {
        if (events.size() >= limit)
            break;
        const QmlEventStats &typeStats = stats.at(row.typeIndex);
        const QmlEventType &type = m_manager.eventType(row.typeIndex);
        const QmlEventLocation location = type.location();
        QJsonObject entry{
            {"type", QString::fromLatin1(rangeTypeName(type.rangeType()))},
            {"name", type.displayName()},
            {"details", type.data()},
            {"calls", typeStats.calls},
            {"total_ns", qint64(typeStats.totalNonRecursive().count())},
            {"total_percent", percentOf(typeStats.totalNonRecursive(), rootDuration)},
            {"self_ns", qint64(typeStats.self.count())},
            {"self_percent", percentOf(typeStats.self, rootDuration)},
            {"average_ns", qint64(typeStats.average().count())},
            {"median_ns", qint64(typeStats.median.count())},
            {"min_ns", qint64(typeStats.minimum.count())},
            {"max_ns", qint64(typeStats.maximum.count())},
        };
        if (!location.filename().isEmpty()) {
            entry.insert("file", location.filename());
            entry.insert("line", location.line());
            entry.insert("column", location.column());
        }
        events.append(entry);
    }

    return {
        {"range",
         QJsonObject{{"start_ns", qint64((m_rangeStart - m_traceStart).count())},
                     {"end_ns", qint64((m_rangeEnd - m_traceStart).count())},
                     {"duration_ns", qint64((m_rangeEnd - m_rangeStart).count())}}},
        {"num_event_types", int(rows.size())},
        {"events", events},
        {"frames",
         frameSummary(m_frameRates, m_options.targetFrameRate, m_slowestFrameTime,
                      m_slowestFrameTimestamp)},
        {"scene_graph", stageArray(m_sceneGraph, sceneGraphStageNames)},
        {"quick3d", stageArray(m_quick3D, quick3DStageNames)},
    };
}

QJsonObject qmlTraceSummary(const QmlProfilerModelManager &manager,
                            const QmlTraceSummaryOptions &options)
{
    QmlTraceSummary summary(manager, options);
    QFutureInterface<void> future;
    QString replayError;
    manager.replayQmlEvents(summary.loader(), {}, {},
                            [&replayError](const QString &message) { replayError = message; },
                            future);
    if (!replayError.isEmpty())
        return {{"error", replayError}};
    return summary.result();
}

} // namespace Profiler::Internal
