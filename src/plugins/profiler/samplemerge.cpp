// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplemerge.h"

#include <QHash>

namespace QmlProfiler::Internal {

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

namespace {

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

    for (SampleTraceData::ThreadSample &sample : merged.samples) {
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

} // namespace QmlProfiler::Internal
