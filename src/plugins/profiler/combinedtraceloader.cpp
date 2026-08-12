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

#include <utils/async.h>

#include <QDebug>
#include <QFutureInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#include <QtTaskTree/QBarrier>
#include <QtTaskTree/QSingleTaskTreeRunner>

using namespace Profiler;
using namespace Profiler::Internal;
using namespace QmlDebug;
using namespace QtTaskTree;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {

class CombinedTraceLoaderPrivate
{
public:
    QmlProfilerModelManager modelManager;
    FilePath bundleDir;
    bool loading = false;
    QSingleTaskTreeRunner taskTreeRunner;

    // The merge runs on a worker thread and stores its percentage here; the GUI
    // thread polls it and turns changes into progress(). A shared_ptr because the
    // worker outlives a cancelled task tree.
    std::shared_ptr<std::atomic<int>> progressCell;
    QTimer progressPoll;
    int lastProgress = -1;
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

// Records the merged trace in the bundle manifest, so a later load finds it
// without having to probe the directory for a valid sampler trace.
static Result<> noteMergedInManifest(const FilePath &bundleDir)
{
    const FilePath manifest = bundleDir / combinedManifestName;
    const Result<QByteArray> content = manifest.fileContents();
    if (!content)
        return ResultError(content.error());

    QJsonObject object = QJsonDocument::fromJson(*content).object();
    object.insert(combinedMergedKey, combinedMergedSubdir);
    const Result<qint64> written =
        manifest.writeFileContents(QJsonDocument(object).toJson(QJsonDocument::Indented));
    if (!written)
        return ResultError(written.error());
    return ResultOk;
}

// Decodes the bundle's native sampler trace, splices `ranges` into its stacks and
// writes the result into the bundle's combinedMergedSubdir, returning that
// directory. `progress` receives 0..100 across the decode and the write, the two
// steps that scale with trace size.
//
// Runs on a worker thread: a sampler stream is routinely hundreds of megabytes,
// so this takes minutes on a long recording (far longer under a sanitizer, which
// taxes every allocation).
static Result<FilePath> mergeBundle(const FilePath &bundleDir, const QList<QmlRange> &ranges,
                                    const std::function<void(int)> &progress)
{
    // Decoding and writing are comparable in cost, so each drives half the bar.
    const auto readProgress = [&progress](int percent) { progress(percent / 2); };
    const auto writeProgress = [&progress](int percent) { progress(50 + percent / 2); };

    const Result<SampleTraceData> native =
        readSampleTrace(bundleDir / combinedSamplerSubdir, readProgress);
    if (!native)
        return ResultError(native.error());

    MergeOptions options;
    options.sampleTimeOffsetUs = readClockOffsetUs(bundleDir);
    const SampleTraceData mergedData = mergeQmlIntoSamples(*native, ranges, options);

    const FilePath outDir = bundleDir / combinedMergedSubdir;
    if (const Result<> result = outDir.ensureWritableDir(); !result)
        return ResultError(result.error());

    if (const Result<> result = writeSampleTrace(mergedData, outDir, writeProgress); !result)
        return ResultError(result.error());

    // Only now is the merged trace complete; a manifest entry written earlier
    // would advertise a half-written directory if the write failed.
    if (const Result<> result = noteMergedInManifest(bundleDir); !result)
        return ResultError(result.error());

    return outDir;
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

    d->progressPoll.setInterval(100);
    connect(&d->progressPoll, &QTimer::timeout, this, [this] {
        const int percent = d->progressCell->load(std::memory_order_relaxed);
        if (percent != d->lastProgress) {
            d->lastProgress = percent;
            emit progress(percent);
        }
    });
}

CombinedTraceLoader::~CombinedTraceLoader()
{
    delete d;
}

void CombinedTraceLoader::load(const FilePath &bundleDir)
{
    // `loading` covers the QML half; the runner covers the native merge that
    // follows it, so both have to be idle before a new bundle can be started.
    if (d->loading || d->taskTreeRunner.isRunning())
        return;
    d->bundleDir = bundleDir;

    // A bundle merged when it was recorded carries the result, so there is nothing
    // to redo. Emit asynchronously to keep merged()/failed() consistently deferred:
    // callers connect right after calling load() and would miss a direct emit.
    const FilePath mergedDir = bundleDir / combinedMergedSubdir;
    if (isSamplerTrace(mergedDir)) {
        QTimer::singleShot(0, this, [this, mergedDir] { emit merged(mergedDir); });
        return;
    }

    d->loading = true;
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

    // The native side of the merge runs on a worker thread. It is the expensive
    // half by far, and this slot is invoked from the QML trace file's destroyed()
    // signal, so running it inline blocked the GUI thread -- and stalled that
    // teardown -- for the entire merge.
    d->progressCell = std::make_shared<std::atomic<int>>(0);
    d->lastProgress = -1;
    d->progressPoll.start();

    const auto onSetup = [bundleDir = d->bundleDir, ranges,
                          cell = d->progressCell](Async<Result<FilePath>> &async) {
        const std::function<void(int)> report = [cell](int percent) {
            cell->store(percent, std::memory_order_relaxed);
        };
        async.setConcurrentCallData(mergeBundle, bundleDir, ranges, report);
    };
    const auto onDone = [this](const Async<Result<FilePath>> &async) {
        d->progressPoll.stop();
        const Result<FilePath> result = async.isResultAvailable()
                                            ? async.result()
                                            : ResultError(Tr::tr("Cannot merge the combined trace."));
        if (result) {
            emit progress(100);
            emit merged(*result);
        } else {
            emit failed(result.error());
        }
    };
    d->taskTreeRunner.start({AsyncTask<Result<FilePath>>(onSetup, onDone)});
}

ExecutableItem mergeCombinedBundleRecipe(const FilePath &bundleDir,
                                         const std::function<void(int)> &reportProgress)
{
    const auto onSetup = [bundleDir, reportProgress](QBarrier &barrier) {
        QBarrier *b = &barrier;
        // Parented to the barrier, so the loader (and its running merge) is torn
        // down with the task if the tree is cancelled.
        auto *loader = new CombinedTraceLoader(b);
        QObject::connect(loader, &CombinedTraceLoader::progress, b, [reportProgress](int percent) {
            if (reportProgress)
                reportProgress(percent);
        });
        // Both outcomes advance: see the note on the declaration for why a failed
        // merge does not fail the recording.
        QObject::connect(loader, &CombinedTraceLoader::merged, b, [b](const FilePath &) {
            b->advance();
        });
        QObject::connect(loader, &CombinedTraceLoader::failed, b, [b](const QString &error) {
            qWarning().noquote() << "Merging the combined trace failed:" << error;
            b->advance();
        });
        loader->load(bundleDir);
    };
    return QBarrierTask(onSetup);
}

} // namespace QmlProfiler::Internal
