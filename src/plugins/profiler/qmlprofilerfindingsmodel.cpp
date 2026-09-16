// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerfindingsmodel.h"

#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersettings.h"

#include <tracing/timelineformatdata.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QSet>
#include <QStack>

#include <cmath>
#include <optional>

using namespace QmlDebug;

namespace Profiler::Internal {

// Ranges nest, so a RangeEnd belongs to the RangeStart that opened the same type. A trace
// can begin inside a range, so the match is not necessarily on top of the stack: opens left
// above it are dropped, the way QmlProfilerStatisticsModel does.
class RangeStack
{
public:
    struct Frame
    {
        int typeIndex = -1;
        RangeType rangeType = UndefinedRangeType;
        qint64 startTime = 0;
    };

    void push(int typeIndex, RangeType rangeType, qint64 startTime)
    {
        m_frames.push({typeIndex, rangeType, startTime});
    }

    std::optional<Frame> pop(int typeIndex)
    {
        for (int i = m_frames.size() - 1; i >= 0; --i) {
            if (m_frames.at(i).typeIndex != typeIndex)
                continue;
            const Frame frame = m_frames.at(i);
            m_frames.resize(i);
            return frame;
        }
        return {};
    }

    bool isEmpty() const { return m_frames.isEmpty(); }
    const Frame &top() const { return m_frames.top(); }
    void clear() { m_frames.clear(); }

private:
    QStack<Frame> m_frames;
};

// Sums the time spent in ranges of one type, per event type. The rules that only need
// "how much, how often" differ in nothing else but the range type they watch.
class RangeTotals
{
public:
    struct Total
    {
        qint64 totalNs = 0;
        int count = 0;
    };

    void loadEvent(const QmlEvent &event, RangeType rangeType)
    {
        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(typeIndex, rangeType, event.timestamp());
            break;
        case RangeEnd: {
            const std::optional<RangeStack::Frame> frame = m_stack.pop(typeIndex);
            if (!frame)
                break;

            Total &total = m_totals[typeIndex];
            total.totalNs += event.timestamp() - frame->startTime;
            ++total.count;
            break;
        }
        default:
            break;
        }
    }

    QHash<int, Total>::const_iterator begin() const { return m_totals.constBegin(); }
    QHash<int, Total>::const_iterator end() const { return m_totals.constEnd(); }

    void clear()
    {
        m_totals.clear();
        m_stack.clear();
    }

private:
    QHash<int, Total> m_totals;
    RangeStack m_stack;
};

// Picks one of the threads that report frames. The GUI and the render thread both report
// a frame event for the same rendering, so following the one that reported first keeps
// the gap between consecutive events equal to a frame duration and their number equal to
// the number of frames; interleaving the two halves the one and doubles the other.
class FrameThread
{
public:
    bool accepts(const QmlEvent &event)
    {
        const int thread = event.number<qint32>(2);
        if (m_thread == -1)
            m_thread = thread;
        return thread == m_thread;
    }

    void clear() { m_thread = -1; }

private:
    int m_thread = -1;
};

// Reports QML files whose first-use compilation dominates startup. Ahead-of-time caching
// does not remove this: what is left in a Compiling range is loading the compilation unit
// and resolving types and imports, which scales with file size and import surface.
class FirstUseCompileRule final : public FindingRule
{
public:
    explicit FirstUseCompileRule(qint64 thresholdNs) : m_thresholdNs(thresholdNs) {}

    quint64 features() const override { return 1ULL << ProfileCompiling; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.rangeType() == Compiling)
            m_totals.loadEvent(event, Compiling);
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_totals.begin(), end = m_totals.end(); it != end; ++it) {
            if (it->totalNs < m_thresholdNs)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);
            const QmlEventType &type = manager->eventType(typeIndex);

            Finding finding;
            finding.ruleId = "first-use-compile";
            finding.severity = it->totalNs >= 4 * m_thresholdNs ? Finding::Warning : Finding::Info;
            finding.location = type.location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Compiling this file costs %1 ms on first use.")
                               .arg(it->totalNs / 1000000);
            finding.why = Tr::tr("Even with ahead-of-time QML caching, first use still loads the "
                                 "compilation unit and resolves the file's types and imports. The "
                                 "cost grows with file size and with the number of imports pulled "
                                 "in before the file can be instantiated.");
            finding.suggestion = Tr::tr("Split the file into smaller components, each compiled and "
                                        "loaded on its own, and move imports that only a lazily "
                                        "loaded part needs into that part.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_totals.clear(); }

private:
    RangeTotals m_totals;
    const qint64 m_thresholdNs;
};

// Reports images the engine failed to load. Each failure is a defect in the application,
// and because a failing source is usually requested again, it also burns repeated work.
class PixmapLoadErrorRule final : public FindingRule
{
public:
    quint64 features() const override { return 1ULL << ProfilePixmapCache; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() != PixmapCacheEvent || type.detailType() != PixmapLoadingError)
            return;
        ++m_errors[event.typeIndex()];
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_errors.constBegin(), end = m_errors.constEnd(); it != end; ++it) {
            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);
            const QmlEventType &type = manager->eventType(typeIndex);

            Finding finding;
            finding.ruleId = "pixmap-load-error";
            finding.severity = Finding::Critical;
            finding.location = type.location();
            finding.typeIndex = typeIndex;
            finding.occurrences = it.value();
            finding.what = it.value() == 1
                               ? Tr::tr("Image failed to load.")
                               : Tr::tr("Image failed to load %1 times.").arg(it.value());
            finding.why = Tr::tr("The engine could not load this source, so nothing is painted for "
                                 "it. Repeated attempts mean the failure is retried rather than "
                                 "cached, so each one costs another request.");
            finding.suggestion = Tr::tr("Check that the source resolves: a missing resource path, a "
                                        "typo in the URL, or an image provider that rejects the "
                                        "request are the usual causes.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_errors.clear(); }

private:
    QHash<int, int> m_errors;
};

// Reports handlers that build items while the caller waits. Assigning a Loader's source,
// or instantiating a view from a signal handler, runs the whole creation before the
// handler returns, and nothing can repaint until it does.
class SyncViewLoadRule final : public FindingRule
{
public:
    explicit SyncViewLoadRule(qint64 thresholdNs) : m_thresholdNs(thresholdNs) {}

    quint64 features() const override
    {
        return (1ULL << ProfileCreating) | (1ULL << ProfileHandlingSignal)
               | (1ULL << ProfileJavaScript);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        const RangeType rangeType = type.rangeType();
        if (rangeType != Creating && rangeType != HandlingSignal && rangeType != Javascript)
            return;

        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(typeIndex, rangeType, event.timestamp());
            break;
        case RangeEnd: {
            const std::optional<RangeStack::Frame> frame = m_stack.pop(typeIndex);
            if (!frame || frame->rangeType != Creating || m_stack.isEmpty())
                break;

            // Attribute the creation to the handler that waited for it: that is where the
            // load has to be made asynchronous.
            const RangeStack::Frame &parent = m_stack.top();
            if (parent.rangeType != HandlingSignal && parent.rangeType != Javascript)
                break;

            Data &data = m_data[parent.typeIndex];
            data.totalNs += event.timestamp() - frame->startTime;
            ++data.count;
            break;
        }
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->totalNs < m_thresholdNs)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "sync-view-load";
            finding.severity = Finding::Warning;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Building items inside this handler costs %1 ms over %2 calls.")
                               .arg(it->totalNs / 1000000).arg(it->count);
            finding.why = Tr::tr("The items are created while the handler runs, so the interface "
                                 "cannot repaint until the whole component is built. The longer "
                                 "the component, the longer the interface stands still.");
            finding.suggestion = Tr::tr("Load the component asynchronously, for example by setting "
                                        "asynchronous on the Loader, and show a placeholder while "
                                        "it is loading.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_stack.clear();
    }

private:
    struct Data {
        qint64 totalNs = 0;
        int count = 0;
    };

    QHash<int, Data> m_data;
    RangeStack m_stack;
    const qint64 m_thresholdNs;
};

// Reports handlers that run at a steady interval. Whether that is wasted work depends on
// what the handler does when nothing is visible, which the trace cannot say - so this
// points at the pattern and leaves the judgement to the reader.
class PeriodicHandlerRule final : public FindingRule
{
public:
    PeriodicHandlerRule(int minCount, double maxRelativeDeviation)
        : m_minCount(minCount)
        , m_maxRelativeDeviation(maxRelativeDeviation)
    {}

    quint64 features() const override { return 1ULL << ProfileHandlingSignal; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.rangeType() != HandlingSignal || event.rangeStage() != RangeStart)
            return;

        Data &data = m_data[event.typeIndex()];
        if (data.count > 0) {
            // Keep running sums instead of every timestamp: a handler can fire thousands
            // of times in a trace.
            const qint64 interval = event.timestamp() - data.lastStart;
            data.intervalSum += interval;
            data.intervalSquareSum += double(interval) * double(interval);
            ++data.intervals;
        }
        data.lastStart = event.timestamp();
        ++data.count;
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->count < m_minCount || it->intervals < 2)
                continue;

            const double mean = double(it->intervalSum) / it->intervals;
            if (mean <= 0)
                continue;
            const double variance = it->intervalSquareSum / it->intervals - mean * mean;
            const double deviation = variance > 0 ? std::sqrt(variance) : 0;
            if (deviation / mean > m_maxRelativeDeviation)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "periodic-handler";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Runs %1 times, about every %2 ms.")
                               .arg(it->count).arg(qRound(mean / 1000000.0));
            finding.why = Tr::tr("A handler driven by a timer keeps running at that interval for "
                                 "as long as the timer does, whether or not anything comes of it.");
            finding.suggestion = Tr::tr("Check what this does when its target is not visible or "
                                        "has no data to show. If the answer is nothing, stop the "
                                        "timer under the same condition that hides the target.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_data.clear(); }

private:
    struct Data {
        qint64 lastStart = 0;
        qint64 intervalSum = 0;
        double intervalSquareSum = 0;
        int intervals = 0;
        int count = 0;
    };

    QHash<int, Data> m_data;
    const int m_minCount;
    const double m_maxRelativeDeviation;
};

// Reports images decoded at a size the interface is unlikely to need. The cache holds the
// decoded image at the size it was loaded, whatever size it is drawn at.
class OversizedPixmapRule final : public FindingRule
{
public:
    explicit OversizedPixmapRule(qint64 pixelThreshold) : m_pixelThreshold(pixelThreshold) {}

    quint64 features() const override { return 1ULL << ProfilePixmapCache; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() != PixmapCacheEvent || type.detailType() != PixmapSizeKnown)
            return;

        const qint64 pixels = qint64(event.number<qint32>(0)) * qint64(event.number<qint32>(1));
        Data &data = m_data[event.typeIndex()];
        if (pixels > data.pixels) {
            data.pixels = pixels;
            data.width = event.number<qint32>(0);
            data.height = event.number<qint32>(1);
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->pixels < m_pixelThreshold)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "oversized-pixmap";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.occurrences = 1;
            finding.what = Tr::tr("Image is decoded at %1x%2.").arg(it->width).arg(it->height);
            finding.why = Tr::tr("The cache keeps the image at the size it was decoded, no matter "
                                 "how small it is drawn, and decoding itself costs more the "
                                 "larger the image is.");
            finding.suggestion = Tr::tr("Set sourceSize on the Image to the size it is actually "
                                        "drawn at, so the image is decoded at that size.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_data.clear(); }

private:
    struct Data {
        qint64 pixels = 0;
        int width = 0;
        int height = 0;
    };

    QHash<int, Data> m_data;
    const qint64 m_pixelThreshold;
};

// Reports bindings and functions whose cost, spread over the frames that were rendered,
// eats a noticeable part of a frame.
class PerFrameCostRule final : public FindingRule
{
public:
    explicit PerFrameCostRule(qint64 budgetNs) : m_budgetNs(budgetNs) {}

    quint64 features() const override
    {
        return (1ULL << ProfileBinding) | (1ULL << ProfileJavaScript)
               | (1ULL << ProfileAnimations);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() == Event && type.detailType() == AnimationFrame) {
            if (m_frameThread.accepts(event))
                ++m_frames;
            return;
        }

        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript)
            return;

        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            m_starts[typeIndex].push(event.timestamp());
            break;
        case RangeEnd: {
            auto it = m_starts.find(typeIndex);
            if (it == m_starts.end() || it->isEmpty())
                break;
            m_data[typeIndex] += event.timestamp() - it->pop();
            break;
        }
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        if (m_frames == 0)
            return findings;

        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            const qint64 perFrame = it.value() / m_frames;
            if (perFrame < m_budgetNs)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "per-frame-cost";
            finding.severity = Finding::Warning;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it.value();
            finding.occurrences = m_frames;
            finding.what = Tr::tr("Costs %1 ms per rendered frame (%2 ms over %3 frames).")
                               .arg(perFrame / 1000000.0, 0, 'f', 2)
                               .arg(it.value() / 1000000).arg(m_frames);
            finding.why = Tr::tr("Work of this size, repeated for every frame, leaves less of the "
                                 "frame for everything else that has to happen in it.");
            finding.suggestion = Tr::tr("Look for a value that is recomputed although it did not "
                                        "change, and keep the result instead of deriving it again "
                                        "each frame.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_starts.clear();
        m_frameThread.clear();
        m_frames = 0;
    }

private:
    QHash<int, qint64> m_data;
    QHash<int, QStack<qint64>> m_starts;
    FrameThread m_frameThread;
    int m_frames = 0;
    const qint64 m_budgetNs;
};

// Reports a single call that ran long enough to be noticed on its own. The per-frame rule
// needs rendered frames to divide by and says nothing about a trace without them; a call
// that holds the thread for a quarter of a second is a stall either way.
class BlockingCallRule final : public FindingRule
{
public:
    explicit BlockingCallRule(qint64 thresholdNs) : m_thresholdNs(thresholdNs) {}

    quint64 features() const override
    {
        return (1ULL << ProfileBinding) | (1ULL << ProfileJavaScript)
               | (1ULL << ProfileHandlingSignal);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript && rangeType != HandlingSignal)
            return;

        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(typeIndex, rangeType, event.timestamp());
            break;
        case RangeEnd: {
            const std::optional<RangeStack::Frame> frame = m_stack.pop(typeIndex);
            // Only the outermost call is reported. What nests inside it is part of the
            // same block, and naming every level of it states the same stall repeatedly.
            if (!frame || !m_stack.isEmpty())
                break;

            const qint64 durationNs = event.timestamp() - frame->startTime;
            if (durationNs < m_thresholdNs)
                break;

            Data &data = m_data[typeIndex];
            data.longestNs = qMax(data.longestNs, durationNs);
            ++data.count;
            break;
        }
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "blocking-call";
            finding.severity = Finding::Warning;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->longestNs;
            finding.occurrences = it->count;
            finding.what = it->count == 1
                               ? Tr::tr("Blocks for %1 ms in one call.")
                                     .arg(it->longestNs / 1000000)
                               : Tr::tr("Blocks for up to %1 ms, in %2 calls.")
                                     .arg(it->longestNs / 1000000).arg(it->count);
            finding.why = Tr::tr("Nothing else runs on this thread while the call does, so no "
                                 "input is answered and no frame is drawn until it returns.");
            finding.suggestion = Tr::tr("Split the work so that each part fits in a frame, move "
                                        "it to C++ where it can run on another thread, or hand "
                                        "it to a WorkerScript.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_stack.clear();
    }

private:
    struct Data {
        qint64 longestNs = 0;
        int count = 0;
    };

    QHash<int, Data> m_data;
    RangeStack m_stack;
    const qint64 m_thresholdNs;
};

// Reports what ran during the frames that took too long. An average spread over every
// frame hides work that all lands in a few of them, and those few are the ones the user
// sees as a stutter.
class FrameJankRule final : public FindingRule
{
public:
    explicit FrameJankRule(qint64 budgetNs) : m_budgetNs(budgetNs) {}

    quint64 features() const override
    {
        return (1ULL << ProfileAnimations) | (1ULL << ProfileBinding)
               | (1ULL << ProfileJavaScript) | (1ULL << ProfileCreating)
               | (1ULL << ProfileHandlingSignal);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() == Event && type.detailType() == AnimationFrame) {
            loadFrame(event);
            return;
        }

        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript && rangeType != Creating
            && rangeType != HandlingSignal) {
            return;
        }

        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(typeIndex, rangeType, event.timestamp());
            break;
        case RangeEnd: {
            const std::optional<RangeStack::Frame> frame = m_stack.pop(typeIndex);
            // Only the outermost range counts: the frame waited for as long as it ran, and
            // adding what nests inside it would charge the same time twice.
            if (frame && m_stack.isEmpty())
                m_pending[typeIndex] += event.timestamp() - frame->startTime;
            break;
        }
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            // A location has to account for a frame's worth of work before it is worth
            // naming: in a long trace almost everything appears in some slow frame.
            if (it->costNs < m_budgetNs)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "frame-jank";
            finding.severity = Finding::Warning;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->costNs;
            finding.occurrences = it->frames;
            finding.what = Tr::tr("Runs for %1 ms inside %2 of the %3 frames that took longer "
                                  "than %4 ms.")
                               .arg(it->costNs / 1000000).arg(it->frames).arg(m_jankyFrames)
                               .arg(m_budgetNs / 1000000);
            finding.why = Tr::tr("The frame could not be finished while this was running, so it "
                                 "stayed on screen for as long as the work took. Work that lands "
                                 "in a few frames is seen as a jump, not as a slowdown.");
            finding.suggestion = Tr::tr("Move the work out of the frame it happens in: do it "
                                        "before it is needed, spread it over several frames, or "
                                        "let it run asynchronously and show the result when it "
                                        "arrives.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_pending.clear();
        m_stack.clear();
        m_frameThread.clear();
        m_lastFrameEnd = -1;
        m_jankyFrames = 0;
    }

private:
    void loadFrame(const QmlEvent &event)
    {
        if (!m_frameThread.accepts(event))
            return;

        // The profiler timestamps a frame when it ends, so what was collected since the
        // previous frame is what this one waited for. A range that started even earlier is
        // charged to the frame it ended in, which is the frame it finally released.
        if (m_lastFrameEnd >= 0 && event.timestamp() - m_lastFrameEnd > m_budgetNs) {
            ++m_jankyFrames;
            for (auto it = m_pending.constBegin(), end = m_pending.constEnd(); it != end; ++it) {
                Data &data = m_data[it.key()];
                data.costNs += it.value();
                ++data.frames;
            }
        }
        m_lastFrameEnd = event.timestamp();
        m_pending.clear();
    }

    struct Data {
        qint64 costNs = 0;
        int frames = 0;
    };

    QHash<int, Data> m_data;
    QHash<int, qint64> m_pending;
    RangeStack m_stack;
    FrameThread m_frameThread;
    qint64 m_lastFrameEnd = -1;
    int m_jankyFrames = 0;
    const qint64 m_budgetNs;
};

// Reports the code that allocates most on the JavaScript heap. Allocating is cheap; the
// collection it forces is not, and that runs at a moment the allocating code does not pick.
class MemoryChurnRule final : public FindingRule
{
public:
    explicit MemoryChurnRule(qint64 thresholdBytes) : m_thresholdBytes(thresholdBytes) {}

    quint64 features() const override
    {
        return (1ULL << ProfileMemory) | (1ULL << ProfileBinding) | (1ULL << ProfileJavaScript)
               | (1ULL << ProfileCreating) | (1ULL << ProfileHandlingSignal);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() == MemoryAllocation) {
            // Heap pages are the allocator growing itself, not what a piece of code asked
            // for. The items are.
            if (type.detailType() != SmallItem && type.detailType() != LargeItem)
                return;

            const qint64 amount = event.number<qint64>(0);
            // A negative amount is a release, and it is reported wherever the collector
            // happened to run, not where the memory was taken.
            if (amount <= 0 || m_stack.isEmpty())
                return;

            Data &data = m_data[m_stack.top().typeIndex];
            data.bytes += amount;
            ++data.count;
            return;
        }

        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript && rangeType != Creating
            && rangeType != HandlingSignal) {
            return;
        }

        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(event.typeIndex(), rangeType, event.timestamp());
            break;
        case RangeEnd:
            m_stack.pop(event.typeIndex());
            break;
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->bytes < m_thresholdBytes)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "memory-churn";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Takes %1 from the JavaScript heap in %2 allocations.")
                               .arg(Timeline::formatDataSize(it->bytes)).arg(it->count);
            finding.why = Tr::tr("Every object taken from the heap has to be collected again. "
                                 "The collection does not happen here, but in whichever frame the "
                                 "heap happens to fill up, which is why it is hard to place.");
            finding.suggestion = Tr::tr("Look for objects, arrays or strings built anew on each "
                                        "call and keep them instead, and move bulk data handling "
                                        "to C++ where it does not touch this heap.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_stack.clear();
    }

private:
    struct Data {
        qint64 bytes = 0;
        int count = 0;
    };

    QHash<int, Data> m_data;
    RangeStack m_stack;
    const qint64 m_thresholdBytes;
};

// Reports a component built over and over. A view builds one delegate per item, so what a
// single one costs is paid again for everything the user scrolls past.
class RepeatedCreationRule final : public FindingRule
{
public:
    explicit RepeatedCreationRule(int minCount) : m_minCount(minCount) {}

    quint64 features() const override { return 1ULL << ProfileCreating; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.rangeType() == Creating)
            m_totals.loadEvent(event, Creating);
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_totals.begin(), end = m_totals.end(); it != end; ++it) {
            if (it->count < m_minCount)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "repeated-creation";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Built %1 times, %2 ms in total.")
                               .arg(it->count).arg(it->totalNs / 1000000);
            finding.why = Tr::tr("Whatever one instance costs to build is paid once per "
                                 "instance, and a view builds one per item it shows.");
            finding.suggestion = Tr::tr("Let the view reuse its delegates by setting reuseItems, "
                                        "and make the delegate itself cheaper: fewer items, fewer "
                                        "bindings, and parts that are rarely seen behind a "
                                        "Loader.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_totals.clear(); }

private:
    RangeTotals m_totals;
    const int m_minCount;
};

// Reports a binding evaluated far more often than anything can be looked at. One
// evaluation may be too cheap to show up in any cost; that it happens that often is the
// finding, because something is changing what the binding reads at that rate.
class BindingChurnRule final : public FindingRule
{
public:
    explicit BindingChurnRule(int minCount) : m_minCount(minCount) {}

    quint64 features() const override { return 1ULL << ProfileBinding; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.rangeType() == Binding)
            m_totals.loadEvent(event, Binding);
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_totals.begin(), end = m_totals.end(); it != end; ++it) {
            if (it->count < m_minCount)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "binding-churn";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Re-evaluated %1 times, %2 ms in total.")
                               .arg(it->count).arg(it->totalNs / 1000000);
            finding.why = Tr::tr("A binding runs again whenever anything it reads changes. At "
                                 "this rate it is reading something that changes continuously, "
                                 "and each run can invalidate the bindings that read its result.");
            finding.suggestion = Tr::tr("Find the value that changes that often and stop the "
                                        "binding from reading it directly: derive it once into a "
                                        "property the binding reads, or express the change as an "
                                        "animation or a Behavior instead.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_totals.clear(); }

private:
    RangeTotals m_totals;
    const int m_minCount;
};

// Reports an image loaded more than once. A decoded image stays in the cache only while
// something holds it, so a second load means it was dropped in between and decoded again.
class PixmapReloadRule final : public FindingRule
{
public:
    explicit PixmapReloadRule(int minCount) : m_minCount(minCount) {}

    quint64 features() const override { return 1ULL << ProfilePixmapCache; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() != PixmapCacheEvent)
            return;

        // The stages of one load are separate event types, so they are collected by the
        // source they refer to rather than by type index.
        const QString source = type.location().filename();
        switch (type.detailType()) {
        case PixmapLoadingStarted: {
            Data &data = m_data[source];
            data.typeIndex = event.typeIndex();
            data.lastStart = event.timestamp();
            ++data.count;
            break;
        }
        case PixmapLoadingFinished: {
            auto it = m_data.find(source);
            if (it == m_data.end() || it->lastStart < 0)
                break;
            it->totalNs += event.timestamp() - it->lastStart;
            it->lastStart = -1;
            break;
        }
        case PixmapLoadingError:
            // A source that fails is reported as an error, and each retry of it would
            // otherwise be counted here as a reload as well.
            m_failed.insert(source);
            break;
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->count < m_minCount || m_failed.contains(it.key()))
                continue;

            const int typeIndex = it->typeIndex;
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "pixmap-reload";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Image is loaded %1 times.").arg(it->count);
            finding.why = Tr::tr("The decoded image is kept only while something refers to it. "
                                 "Loading the same source again means it was dropped in between, "
                                 "so reading and decoding it is paid for once more.");
            finding.suggestion = Tr::tr("Keep the item that uses the image alive instead of "
                                        "destroying and recreating it, for example by hiding it "
                                        "rather than unloading its Loader, or raise the image "
                                        "cache so the entry survives until it is needed again.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_failed.clear();
    }

private:
    struct Data {
        qint64 totalNs = 0;
        qint64 lastStart = -1;
        int typeIndex = -1;
        int count = 0;
    };

    QHash<QString, Data> m_data;
    QSet<QString> m_failed;
    const int m_minCount;
};

FindingRules defaultFindingRules()
{
    // Thresholds are user settings: what counts as too slow depends on the target the
    // application is meant to run on, not on the trace.
    const QmlProfilerSettings &settings = globalSettings();

    FindingRules rules;
    rules.push_back(std::make_unique<FirstUseCompileRule>(
        settings.findingsCompileThresholdMs() * 1000000ll));
    rules.push_back(std::make_unique<PixmapLoadErrorRule>());
    rules.push_back(std::make_unique<SyncViewLoadRule>(
        settings.findingsSyncLoadThresholdMs() * 1000000ll));
    rules.push_back(std::make_unique<PeriodicHandlerRule>(
        settings.findingsPeriodicMinCount(),
        settings.findingsPeriodicDeviationPercent() / 100.0));
    rules.push_back(std::make_unique<OversizedPixmapRule>(
        qint64(settings.findingsPixmapMegapixels() * 1000000.0)));
    rules.push_back(std::make_unique<PerFrameCostRule>(
        settings.findingsPerFrameBudgetUs() * 1000ll));
    rules.push_back(std::make_unique<BlockingCallRule>(
        settings.findingsBlockingCallThresholdMs() * 1000000ll));
    rules.push_back(std::make_unique<FrameJankRule>(
        settings.findingsJankFrameThresholdMs() * 1000000ll));
    rules.push_back(std::make_unique<MemoryChurnRule>(
        settings.findingsMemoryChurnKb() * 1024ll));
    rules.push_back(std::make_unique<RepeatedCreationRule>(settings.findingsCreationMinCount()));
    rules.push_back(std::make_unique<BindingChurnRule>(settings.findingsBindingMinCount()));
    rules.push_back(std::make_unique<PixmapReloadRule>(settings.findingsPixmapReloadMinCount()));
    return rules;
}

QmlProfilerFindingsModel::QmlProfilerFindingsModel(QmlProfilerModelManager *modelManager,
                                                   FindingRules rules)
    : m_modelManager(modelManager)
    , m_rules(std::move(rules))
{
    quint64 features = 0;
    for (const auto &rule : m_rules)
        features |= rule->features();

    modelManager->qmlLoaders.append({features,
                                     std::bind(&QmlProfilerFindingsModel::loadEvent, this,
                                               std::placeholders::_1, std::placeholders::_2)});
    modelManager->registerFeatures(features,
                                   std::bind(&QmlProfilerFindingsModel::beginResetModel, this),
                                   std::bind(&QmlProfilerFindingsModel::finalize, this),
                                   std::bind(&QmlProfilerFindingsModel::clear, this));
}

QmlProfilerFindingsModel::~QmlProfilerFindingsModel() = default;

void QmlProfilerFindingsModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    // Rules select the events they care about themselves; the feature mask only decides
    // which events the manager replays to us at all.
    for (const auto &rule : m_rules)
        rule->loadEvent(event, type);
}

void QmlProfilerFindingsModel::finalize()
{
    m_findings.clear();
    for (const auto &rule : m_rules)
        m_findings.append(rule->finalize(m_modelManager));

    std::stable_sort(m_findings.begin(), m_findings.end(),
                     [](const Finding &a, const Finding &b) {
        if (a.severity != b.severity)
            return a.severity > b.severity;
        return a.costNs > b.costNs;
    });

    endResetModel();
}

void QmlProfilerFindingsModel::clear()
{
    beginResetModel();
    for (const auto &rule : m_rules)
        rule->clear();
    m_findings.clear();
    endResetModel();
}

int QmlProfilerFindingsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_findings.count();
}

int QmlProfilerFindingsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : MaxColumn;
}

static QString severityLabel(Finding::Severity severity)
{
    switch (severity) {
    case Finding::Critical:
        return Tr::tr("Critical");
    case Finding::Warning:
        return Tr::tr("Warning");
    case Finding::Info:
        break;
    }
    return Tr::tr("Info");
}

QVariant QmlProfilerFindingsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_findings.count())
        return {};

    const Finding &finding = m_findings.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColumnSeverity:
            return severityLabel(finding.severity);
        case ColumnFinding:
            return finding.what;
        case ColumnLocation:
            // Compiling ranges and pixmap events carry no line: the trace reports 0 for
            // them, and QML lines are 1-based, so anything below 1 is "no line".
            return finding.location.line() > 0
                       ? QString::fromLatin1("%1:%2").arg(finding.location.filename())
                             .arg(finding.location.line())
                       : finding.location.filename();
        case ColumnCost:
            return finding.costNs > 0 ? QString::fromLatin1("%1 ms").arg(finding.costNs / 1000000)
                                      : QString();
        case ColumnOccurrences:
            return finding.occurrences;
        default:
            return {};
        }
    case Qt::ToolTipRole:
        return QString(finding.why + QLatin1String("\n\n") + finding.suggestion);
    case SortRole:
        switch (index.column()) {
        case ColumnSeverity:
            return finding.severity;
        case ColumnCost:
            return finding.costNs;
        case ColumnOccurrences:
            return finding.occurrences;
        default:
            return data(index, Qt::DisplayRole);
        }
    case TypeIdRole:
        return finding.typeIndex;
    case FilenameRole:
        return finding.location.filename();
    case LineRole:
        return finding.location.line();
    case ColumnRole:
        return finding.location.column();
    case WhyRole:
        return finding.why;
    case SuggestionRole:
        return finding.suggestion;
    case RuleIdRole:
        return finding.ruleId;
    default:
        return {};
    }
}

QVariant QmlProfilerFindingsModel::headerData(int section, Qt::Orientation orientation,
                                              int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColumnSeverity:
        return Tr::tr("Severity");
    case ColumnFinding:
        return Tr::tr("Finding");
    case ColumnLocation:
        return Tr::tr("Location");
    case ColumnCost:
        return Tr::tr("Cost");
    case ColumnOccurrences:
        return Tr::tr("Count");
    default:
        return {};
    }
}

bool findingIsInSource(const QModelIndex &index)
{
    return index.data(QmlProfilerFindingsModel::LineRole).toInt() > 0
           && !index.data(QmlProfilerFindingsModel::FilenameRole).toString().isEmpty();
}

} // namespace Profiler::Internal
