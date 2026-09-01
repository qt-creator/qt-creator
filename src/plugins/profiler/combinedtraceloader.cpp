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

#include <utility>

using namespace QmlDebug;
using namespace QtTaskTree;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

class CombinedTraceLoaderPrivate
{
public:
    QmlProfilerModelManager modelManager;
    FilePath bundleDir;
    bool loading = false;
    bool qmlFailed = false;
    // Bumped by every load() and every cancel(). A result still on its way from
    // an earlier load carries the generation it was started for and drops itself
    // when that no longer matches: neither the merge nor the QML load can be
    // interrupted, so this is what makes abandoning one stick.
    int generation = 0;
    int qmlGeneration = 0; // The generation the in-flight QML load belongs to.
    FilePath pending;      // A bundle to load once that QML load has reported.
    QSingleTaskTreeRunner taskTreeRunner;

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

// The manifest entry appears only once the merge is complete, so unlike probing the
// directory - whose metadata file is written long before the stream - this tells a
// finished merge from an abandoned one.
static bool hasMergedTrace(const FilePath &bundleDir)
{
    const Result<QByteArray> content = (bundleDir / combinedManifestName).fileContents();
    if (!content)
        return false;
    const QJsonObject object = QJsonDocument::fromJson(*content).object();
    return object.contains(combinedMergedKey) && isSamplerTrace(bundleDir / combinedMergedSubdir);
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
// `markComplete` is false when the QML half did not load cleanly: the result is
// good enough to show, but recording it in the manifest would serve a QML-less
// merge for good.
// Runs on a worker thread. Progress goes through the promise rather than a
// shared cell someone reads on a timer: Qt marshals it to the thread the task
// was started on, and drops it if that task is gone.
static void mergeBundle(QPromise<Result<FilePath>> &promise, const FilePath &bundleDir,
                        const QList<QmlRange> &ranges, bool markComplete)
{
    promise.setProgressRange(0, 100);
    const auto progress = [&promise](int percent) { promise.setProgressValue(percent); };
    // Decoding and writing are comparable in cost, so each drives half the bar.
    const auto readProgress = [&progress](int percent) { progress(percent / 2); };
    const auto writeProgress = [&progress](int percent) { progress(50 + percent / 2); };

    const Result<SampleTraceData> native =
        readSampleTrace(bundleDir / combinedSamplerSubdir, readProgress);
    if (!native) {
        promise.addResult(ResultError(native.error()));
        return;
    }

    MergeOptions options;
    options.sampleTimeOffsetUs = readClockOffsetUs(bundleDir);
    const SampleTraceData mergedData = mergeQmlIntoSamples(*native, ranges, options);

    const FilePath outDir = bundleDir / combinedMergedSubdir;
    if (const Result<> result = outDir.ensureWritableDir(); !result) {
        promise.addResult(ResultError(result.error()));
        return;
    }

    if (const Result<> result = writeSampleTrace(mergedData, outDir, writeProgress); !result) {
        promise.addResult(ResultError(result.error()));
        return;
    }

    // Only now is the merged trace complete; a manifest entry written earlier
    // would advertise a half-written directory if the write failed.
    if (markComplete) {
        if (const Result<> result = noteMergedInManifest(bundleDir); !result) {
            promise.addResult(ResultError(result.error()));
            return;
        }
    }

    promise.addResult(outDir);
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
    connect(&d->modelManager, &QmlProfilerModelManager::error,
            this, [this](const QString &message) {
        d->qmlFailed = true;
        qWarning().noquote() << "CombinedTraceLoader: QML trace load reported:" << message;
    });

}

CombinedTraceLoader::~CombinedTraceLoader()
{
    delete d;
}

void CombinedTraceLoader::load(const FilePath &bundleDir)
{
    ++d->generation; // Whatever an earlier load still delivers is now stale.

    // A bundle merged when it was recorded carries the result, so there is nothing
    // to redo. Emit asynchronously to keep merged()/failed() consistently deferred:
    // callers connect right after calling load() and would miss a direct emit.
    const FilePath mergedDir = bundleDir / combinedMergedSubdir;
    if (hasMergedTrace(bundleDir)) {
        d->pending.clear();
        QTimer::singleShot(0, this, [this, mergedDir, generation = d->generation] {
            if (generation == d->generation)
                emit merged(mergedDir);
        });
        return;
    }

    // The model manager holds one trace at a time, so a QML load already running
    // has to report before the next one can start; onQmlLoaded() picks this up.
    if (d->loading) {
        d->pending = bundleDir;
        return;
    }
    startQmlLoad(bundleDir);
}

void CombinedTraceLoader::startQmlLoad(const FilePath &bundleDir)
{
    d->bundleDir = bundleDir;
    d->loading = true;
    d->qmlFailed = false;
    d->qmlGeneration = d->generation;
    // Loads on a worker thread; onQmlLoaded() runs once loadFinished fires.
    d->modelManager.load((bundleDir / combinedQmlFileName).toFSPathString());
}

void CombinedTraceLoader::cancel()
{
    // Neither half can be interrupted -- the merge is a plain concurrent call, and
    // finishing it still leaves a usable cache -- so only the reporting is dropped.
    // Tearing the task tree down here instead would risk doing so from its own done
    // handler.
    ++d->generation;
    d->pending.clear();
}

void CombinedTraceLoader::onQmlLoaded()
{
    if (!d->loading)
        return; // Guard against a spurious loadFinished with no load in flight.
    d->loading = false;

    if (d->qmlGeneration != d->generation) {
        // Superseded while it was loading: these ranges describe a trace nobody
        // is waiting for. Take the request that superseded it instead.
        if (!d->pending.isEmpty())
            startQmlLoad(std::exchange(d->pending, {}));
        return;
    }

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
    const auto onSetup = [this, bundleDir = d->bundleDir, ranges, markComplete = !d->qmlFailed,
                          generation = d->generation](Async<Result<FilePath>> &async) {
        // Qt delivers the merge's progress here, on this thread, and stops
        // doing so once the task is gone -- so nothing has to watch for it.
        connect(&async, &AsyncBase::progressValueChanged, this, [this, generation](int percent) {
            if (generation == d->generation)
                emit progress(percent);
        });
        async.setConcurrentCallData(mergeBundle, bundleDir, ranges, markComplete);
    };
    const auto onDone = [this, generation = d->generation](const Async<Result<FilePath>> &async) {
        if (generation != d->generation)
            return;
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

} // namespace Profiler::Internal
