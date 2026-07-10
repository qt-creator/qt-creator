// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "combinedtraceloader.h"

#include "combinedsampler.h"
#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "samplemerge.h"
#include "sampletrace.h"

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventtype.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFutureInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

using namespace Profiler;
using namespace Profiler::Internal;
using namespace QmlDebug;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {

class CombinedTraceLoaderPrivate
{
public:
    QmlProfilerModelManager modelManager;
    FilePath bundleDir;
    bool loading = false;
};

// The clock-correlation offset (microseconds) CombinedSampler wrote to the
// bundle manifest; 0 if absent or unreadable. Added to each native sample's
// timestamp so it lines up with the QML profiler's timeline.
static qint64 readClockOffsetUs(const FilePath &bundleDir)
{
    const Result<QByteArray> content = (bundleDir / combinedManifestName).fileContents();
    if (!content)
        return 0;
    const QJsonObject object = QJsonDocument::fromJson(*content).object();
    return qint64(object.value("qmlClockOffsetUs"_L1).toDouble(0));
}

// A readable fallback name when a range type carries no source details.
static QString nameForRangeType(RangeType rangeType)
{
    switch (rangeType) {
    case Creating: return Tr::tr("Create");
    case Binding: return Tr::tr("Binding");
    case HandlingSignal: return Tr::tr("Signal");
    case Javascript: return Tr::tr("JavaScript");
    default: return {};
    }
}

CombinedTraceLoader::CombinedTraceLoader(QObject *parent)
    : QObject(parent)
    , d(new CombinedTraceLoaderPrivate)
{
    connect(&d->modelManager, &QmlProfilerModelManager::loadFinished,
            this, &CombinedTraceLoader::onQmlLoaded);
    // The model manager emits error() for non-fatal problems during an otherwise
    // successful load too, so it does not gate the flow: loadFinished always fires
    // (the reader is destroyed on both paths) and drives onQmlLoaded. A genuinely
    // failed QML load just yields no ranges, so the merged trace is the native one.
    connect(&d->modelManager, &QmlProfilerModelManager::error, this, [](const QString &message) {
        qWarning().noquote() << "CombinedTraceLoader: QML trace load reported:" << message;
    });
}

CombinedTraceLoader::~CombinedTraceLoader()
{
    delete d;
}

void CombinedTraceLoader::load(const FilePath &bundleDir)
{
    if (d->loading)
        return;
    d->loading = true;
    d->bundleDir = bundleDir;
    // Loads on a worker thread; onQmlLoaded() runs once loadFinished fires.
    d->modelManager.load((bundleDir / combinedQmlFileName).toFSPathString());
}

void CombinedTraceLoader::onQmlLoaded()
{
    if (!d->loading)
        return; // Guard against a spurious loadFinished with no load in flight.
    d->loading = false;

    // Reconstruct the JS/QML call stack as it varies over time: replay every range
    // event, maintaining a stack of open ranges, and emit one QmlRange per closed
    // range with its parent set to the enclosing open range. Timestamps are made
    // zero-based against the QML trace start so they line up (approximately, until
    // clock correlation lands) with the sampler's own zero-based timestamps.
    const qint64 baseNs = d->modelManager.traceStart();
    const auto toUs = [baseNs](qint64 ns) { return quint64(qMax<qint64>(0, ns - baseNs) / 1000); };

    QList<QmlRange> ranges;
    struct OpenRange { int rangeIndex; int typeIndex; };
    QList<OpenRange> stack;

    const auto loader = [&](const QmlEvent &event, const QmlEventType &type) {
        const RangeType rangeType = type.rangeType();
        // Only the ranges that make up the JS/QML call stack. Compiling nests
        // independently (asynchronous compilation), so it is left out.
        if (rangeType != Creating && rangeType != Binding && rangeType != HandlingSignal
            && rangeType != Javascript) {
            return;
        }

        const Message stage = event.rangeStage();
        if (stage == RangeStart) {
            QmlRange range;
            range.startUs = toUs(event.timestamp());
            range.parent = stack.isEmpty() ? -1 : stack.last().rangeIndex;
            const QmlEventLocation location = type.location();
            range.file = location.filename();
            range.line = location.line() < 0 ? 0 : location.line();
            range.name = type.data().isEmpty() ? type.displayName() : type.data();
            if (range.name.isEmpty())
                range.name = nameForRangeType(rangeType);
            stack.append({int(ranges.size()), event.typeIndex()});
            ranges.append(range);
        } else if (stage == RangeEnd) {
            // Match by type from the top, tolerating unmatched opens (a restricted
            // trace can cut ranges), exactly like the flame graph does.
            int matchIndex = -1;
            for (int i = stack.size() - 1; i >= 0; --i) {
                if (stack.at(i).typeIndex == event.typeIndex()) {
                    matchIndex = i;
                    break;
                }
            }
            if (matchIndex < 0)
                return;
            while (stack.size() - 1 > matchIndex)
                stack.removeLast();
            ranges[stack.last().rangeIndex].endUs = toUs(event.timestamp());
            stack.removeLast();
        }
    };

    QFutureInterface<void> future;
    d->modelManager.replayQmlEvents(loader, [] {}, [] {}, [](const QString &) {}, future);

    // Close ranges still open at the end of the trace at the trace end.
    const quint64 traceEndUs = toUs(d->modelManager.traceEnd());
    for (const OpenRange &open : std::as_const(stack)) {
        if (ranges[open.rangeIndex].endUs == 0)
            ranges[open.rangeIndex].endUs = traceEndUs;
    }

    const Result<SampleTraceData> native = readSampleTrace(d->bundleDir / combinedSamplerSubdir);
    if (!native) {
        emit failed(native.error());
        return;
    }

    MergeOptions options;
    options.sampleTimeOffsetUs = readClockOffsetUs(d->bundleDir);
    const SampleTraceData mergedData = mergeQmlIntoSamples(*native, ranges, options);

    const QString dirName = u"qtprofiler-merged-%1"_s.arg(QDateTime::currentMSecsSinceEpoch());
    const QString dirPath =
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(dirName);
    if (!QDir().mkpath(dirPath)) {
        emit failed(Tr::tr("Cannot create the merged trace directory %1.").arg(dirPath));
        return;
    }
    const FilePath outDir = FilePath::fromString(dirPath);
    if (Result<> result = writeSampleTrace(mergedData, outDir); !result) {
        emit failed(result.error());
        return;
    }

    emit merged(outDir);
}

} // namespace QmlProfiler::Internal
