// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplemerge.h"

#include <QHash>

#include <optional>
#include <vector>

namespace Profiler::Internal {

// The JIT region marker the QML engine emits for compiled JS, matching the
// convention used on the perf side (see perfprofilerconstants.h).
static const QLatin1StringView jitRegionMarker("JITCode:QtQml");

bool isEngineFrame(const SampleTraceData::Label &label)
{
    // A frame is "engine" when it belongs to the V4 machinery: either the symbol
    // is in the QV4 namespace / QtQml classes, or the loaded image is the QML
    // engine binary, or it is the JIT'd-JS region marker. This is the run the
    // splice replaces with the exact JS frames from the QML profiler.
    const QString &name = label.name;
    if (name.startsWith(QLatin1StringView("QV4::"))
        || name.startsWith(QLatin1StringView("QQml"))
        || name.startsWith(QLatin1StringView("QJSEngine"))
        || name == jitRegionMarker) {
        return true;
    }
    return label.module.contains(QLatin1StringView("Qml"), Qt::CaseInsensitive);
}

// A frame only a thread that is actually executing JS can show: the V4
// interpreter, or the code it JITs. Merely being inside the QML library is not
// enough (see isEngineFrame) -- that is equally true of every worker thread the
// library runs, none of which executes JS.
//
// This needs the interpreter's own symbols, so against a QML library stripped of
// them nothing matches and no JS is attributed at all. That is the honest answer
// to "which thread ran the engine": without them the trace does not say.
static bool isJsExecutionFrame(const SampleTraceData::Label &label)
{
    return label.name.startsWith(QLatin1StringView("QV4::")) || label.name == jitRegionMarker;
}

namespace {

// The thread that ran the profiled engine's JS, or nothing when the trace
// caught none.
//
// The QML profiler describes one engine, and an engine runs JS on one thread,
// so its stacks belong to exactly one thread of the native trace. Without this
// every thread whose frames merely live in the QML library gets them too -- a
// QQmlThread event loop parked in poll() matches isEngineFrame() in every
// single sample, and would take the engine's JS stack as its own.
//
// An engine on a worker thread (a WorkerScript) is found the same way. Two
// engines running at once resolve to the busier thread, a distinction the QML
// profiler's own single stream of ranges does not draw either.
std::optional<quint64> jsExecutionThread(const SampleTraceData &native)
{
    // Indexed by label id, because it is asked once per frame of every sample.
    std::vector<bool> executing(native.labels.size(), false);
    bool anyExecuting = false;
    for (int i = 0; i < native.labels.size(); ++i) {
        if (isJsExecutionFrame(native.labels.at(i))) {
            executing[i] = true;
            anyExecuting = true;
        }
    }
    if (!anyExecuting)
        return std::nullopt;

    QHash<quint64, int> perThread;
    for (const SampleTraceData::ThreadSample &sample : native.samples) {
        for (int frame : sample.frames) {
            if (executing[frame]) {
                ++perThread[sample.tid];
                break;
            }
        }
    }

    std::optional<quint64> best;
    int bestCount = 0;
    for (auto it = perThread.cbegin(); it != perThread.cend(); ++it) {
        // Ties go to the lower thread id, so the same trace always merges the
        // same way.
        if (it.value() > bestCount || (it.value() == bestCount && best && it.key() < *best)) {
            best = it.key();
            bestCount = it.value();
        }
    }
    return best;
}

// Interns JS labels so an identical (name, file, line) appears once in the
// merged label table, appended after the native labels.
class LabelInterner
{
public:
    explicit LabelInterner(QList<SampleTraceData::Label> &labels)
        : m_labels(labels)
    {}

    int idFor(const QmlRange &range)
    {
        const auto key = qMakePair(range.name, qMakePair(range.file, range.line));
        if (const auto it = m_ids.constFind(key); it != m_ids.constEnd())
            return it.value();
        const int id = int(m_labels.size());
        // module is left empty: an empty module with a .qml/.js file is how a
        // consumer tells a JS frame from a C++ one.
        m_labels.append(SampleTraceData::Label(range.name, range.file, range.line));
        m_ids.insert(key, id);
        return id;
    }

private:
    QList<SampleTraceData::Label> &m_labels;
    QHash<QPair<QString, QPair<QString, int>>, int> m_ids;
};

// The QML/JS call stack active at time `tsUs`, as range indices ordered
// root-first, or empty if no range covers that time.
QList<int> activeRangeChain(const QList<QmlRange> &ranges, quint64 tsUs)
{
    // The innermost covering range is the one that starts latest among those
    // that contain tsUs (ties broken by the earliest end); its parent chain is
    // the full stack. Ranges are perfectly nested, so this is unambiguous.
    int innermost = -1;
    for (int i = 0; i < ranges.size(); ++i) {
        const QmlRange &r = ranges[i];
        if (tsUs < r.startUs || tsUs >= r.endUs)
            continue;
        if (innermost < 0 || r.startUs > ranges[innermost].startUs
            || (r.startUs == ranges[innermost].startUs && r.endUs < ranges[innermost].endUs)) {
            innermost = i;
        }
    }
    if (innermost < 0)
        return {};

    QList<int> leafFirst;
    for (int i = innermost; i >= 0; i = ranges[i].parent) {
        leafFirst.append(i);
        // Guard against a malformed parent link pointing forward or at itself.
        if (ranges[i].parent >= i)
            break;
    }
    QList<int> rootFirst;
    rootFirst.reserve(leafFirst.size());
    for (auto it = leafFirst.crbegin(); it != leafFirst.crend(); ++it)
        rootFirst.append(*it);
    return rootFirst;
}

// [begin, end) of the first maximal run of engine frames in `frames`, or a zero
// length run (begin == end) when there is none.
struct FrameRun { int begin = 0; int end = 0; };

FrameRun firstEngineRun(const QList<int> &frames, const QList<SampleTraceData::Label> &labels)
{
    int begin = -1;
    for (int i = 0; i < frames.size(); ++i) {
        const int labelId = frames[i];
        const bool engine = labelId >= 0 && labelId < labels.size()
                            && isEngineFrame(labels[labelId]);
        if (engine) {
            if (begin < 0)
                begin = i;
        } else if (begin >= 0) {
            return {begin, i};
        }
    }
    if (begin >= 0)
        return {begin, int(frames.size())};
    return {0, 0};
}

} // namespace

SampleTraceData mergeQmlIntoSamples(
    const SampleTraceData &native,
    const QList<QmlRange> &qmlRanges,
    const MergeOptions &options)
{
    SampleTraceData merged = native;
    LabelInterner interner(merged.labels);

    // Without a thread to attribute them to, the JS stacks would go to whichever
    // threads happen to sit in the QML library, which is where they never ran.
    const std::optional<quint64> engineThread = jsExecutionThread(native);
    if (!engineThread)
        return merged;

    for (SampleTraceData::ThreadSample &sample : merged.samples) {
        if (sample.tid != *engineThread)
            continue; // Not the engine's thread, so not running the engine's JS.

        const FrameRun run = firstEngineRun(sample.frames, merged.labels);
        if (run.begin == run.end)
            continue; // No engine frames: pure C++ sample, nothing to attribute.

        const qint64 adjusted = qint64(sample.tsUs) + options.sampleTimeOffsetUs;
        const QList<int> chain = activeRangeChain(qmlRanges, adjusted < 0 ? 0 : quint64(adjusted));
        if (chain.isEmpty())
            continue; // Engine active but no instrumented JS: keep engine frames.

        QList<int> jsFrames;
        jsFrames.reserve(chain.size());
        for (int rangeIndex : chain)
            jsFrames.append(interner.idFor(qmlRanges[rangeIndex]));

        QList<int> rebuilt;
        rebuilt.reserve(sample.frames.size() + jsFrames.size());
        // Native frames above the engine boundary.
        for (int i = 0; i < run.begin; ++i)
            rebuilt.append(sample.frames[i]);
        // The engine machinery, kept only when interleaving.
        if (options.revealEngineFrames) {
            for (int i = run.begin; i < run.end; ++i)
                rebuilt.append(sample.frames[i]);
        }
        // The exact JS frames from the QML profiler.
        rebuilt.append(jsFrames);
        // Native callees reached from JS (below the engine boundary).
        for (int i = run.end; i < sample.frames.size(); ++i)
            rebuilt.append(sample.frames[i]);

        sample.frames = rebuilt;
    }

    return merged;
}

} // namespace Profiler::Internal
