// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertracefile_test.h"

#include "../memoryusagemodel.h"
#include "../qmlprofilermodelmanager.h"
#include "../qmlprofilerrangemodel.h"
#include "../qmlprofilerstatisticsmodel.h"

#include <tracing/timelinemodelaggregator.h>

#include <utils/async.h>

#include <QFile>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include <atomic>
#include <memory>
#include <optional>

#if defined(Q_OS_MACOS)
#include <malloc/malloc.h>
#elif defined(__GLIBC__)
#include <malloc.h>
#endif
#include <tuple>

using namespace QmlDebug;

namespace Profiler::Internal {

using Ranges = QList<std::pair<qint64, qint64>>;

// Writes a trace of `ranges` JavaScript ranges, long enough to be still loading while a test
// holds the loading thread.
static void writeLongTrace(const QString &path, int ranges)
{
    QmlProfilerModelManager saved;
    saved.initialize();
    const int type = saved.appendEventType(
        QmlEventType(UndefinedMessage, Javascript, -1, QmlEventLocation("main.qml", 1, 1), "f"));
    for (int i = 0; i < ranges; ++i) {
        QmlEvent event(i * 10, type, {});
        event.setRangeStage(RangeStart);
        saved.appendEvent(QmlEvent(event));
        event.setRangeStage(RangeEnd);
        event.setTimestamp(i * 10 + 5);
        saved.appendEvent(std::move(event));
    }
    saved.decreaseTraceStart(0);
    saved.increaseTraceEnd(qint64(ranges) * 10);
    saved.finalize();
    QSignalSpy saveSpy(&saved, &QmlProfilerModelManager::saveFinished);
    saved.save(path);
    QTRY_COMPARE_WITH_TIMEOUT(saveSpy.count(), 1, 60000);
}

namespace {

// A loader that holds the loading thread in its first event until the gate opens, and counts
// the events that arrive once the test has stopped the load.
struct GatedLoader
{
    QSemaphore started;
    QSemaphore gate;
    std::atomic<int> events = 0;
    std::atomic<bool> armed = true;
    std::atomic<bool> stopped = false;
    std::atomic<int> eventsAfterStop = 0;
};

} // namespace

static void addGatedLoader(QmlProfilerModelManager *manager,
                           const std::shared_ptr<GatedLoader> &gated)
{
    manager->qmlLoaders.append({1ULL << ProfileJavaScript,
                                [gated](const QmlEvent &, const QmlEventType &) {
        if (!gated->armed)
            return;
        if (gated->stopped)
            ++gated->eventsAfterStop;
        if (gated->events++ == 0) {
            gated->started.release();
            gated->gate.acquire();
        }
    }});
}

static void fillTrace(QmlProfilerModelManager &manager, const Ranges &paused)
{
    manager.initialize();
    for (const qint64 time : {100, 900}) {
        QmlEvent event;
        event.setTimestamp(time);
        event.setString(QString::fromLatin1("message at %1").arg(time));
        event.setTypeIndex(manager.numEventTypes());
        manager.appendEventType(QmlEventType(DebugMessage, UndefinedRangeType, QtDebugMsg,
                                             QmlEventLocation("main.qml", 1, 1)));
        manager.appendEvent(std::move(event));
    }
    manager.decreaseTraceStart(0);
    manager.increaseTraceEnd(1000);
    for (const auto &[start, end] : paused)
        manager.addPausedRange(start, end);
    manager.finalize();
}

// Saves a trace with `paused` and loads it back into `loaded`, with its paused
// ranges written as `pausedXml` in place of what was saved, if that is set.
static void saveAndLoad(const Ranges &paused, QmlProfilerModelManager &loaded,
                        const QByteArray &pausedXml = {})
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("trace.qtd");

    QmlProfilerModelManager saved;
    fillTrace(saved, paused);
    QSignalSpy saveSpy(&saved, &QmlProfilerModelManager::saveFinished);
    saved.save(path);
    QTRY_COMPARE(saveSpy.count(), 1);

    if (!pausedXml.isEmpty()) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray content = file.readAll();
        file.close();
        const qsizetype begin = content.indexOf("<pausedRanges>");
        const QByteArray endTag = "</pausedRanges>";
        const qsizetype end = content.indexOf(endTag);
        QVERIFY(begin >= 0 && end > begin);
        content.replace(begin, end + endTag.size() - begin, pausedXml);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(content);
    }

    loaded.addPausedRange(5, 6); // Left from before; loading has to replace it.
    QSignalSpy loadSpy(&loaded, &QmlProfilerModelManager::loadFinished);
    QSignalSpy errorSpy(&loaded, &QmlProfilerModelManager::error);
    loaded.load(path);
    QTRY_COMPARE(loadSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(loaded.numEvents(), 2);
}

void QmlProfilerTraceFileTest::testPausedRangesRoundTrip()
{
    const Ranges paused{{200, 400}, {600, 700}};
    QmlProfilerModelManager loaded;
    saveAndLoad(paused, loaded);
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(loaded.pausedRanges(), paused);
}

void QmlProfilerTraceFileTest::testNoPausedRanges()
{
    QmlProfilerModelManager loaded;
    saveAndLoad({}, loaded);
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(loaded.pausedRanges(), Ranges());
}

void QmlProfilerTraceFileTest::testMalformedPausedRangesAreSkipped()
{
    QmlProfilerModelManager loaded;
    saveAndLoad({{1, 2}}, loaded,
                "<pausedRanges>"
                "<paused start=\"200\" end=\"400\"/>"
                "<paused start=\"500\"/>"               // No end.
                "<paused end=\"550\"/>"                 // No start.
                "<paused start=\"x\" end=\"650\"/>"   // Not a number.
                "<paused start=\"800\" end=\"700\"/>" // Ends before it starts.
                "<paused start=\"750\" end=\"750\"/>" // Empty.
                "<paused start=\"850\" end=\"900\"/>"
                "</pausedRanges>");
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(loaded.pausedRanges(), (Ranges{{200, 400}, {850, 900}}));
}

// Deleting what a load calls into is only safe once cancelBackgroundWork() returned: from then
// on, the loading thread calls nothing any more.
void QmlProfilerTraceFileTest::testCancelStopsTheLoadingThread()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("trace.qzt");
    const int ranges = 100000;
    writeLongTrace(path, ranges);
    if (QTest::currentTestFailed())
        return;

    QmlProfilerModelManager manager;
    auto gated = std::make_shared<GatedLoader>();
    const auto open = qScopeGuard([gated] { gated->gate.release(); });
    addGatedLoader(&manager, gated);
    QSignalSpy loadSpy(&manager, &QmlProfilerModelManager::loadFinished);
    manager.load(path);
    QVERIFY(gated->started.tryAcquire(1, 10000));

    gated->gate.release();
    manager.cancelBackgroundWork();
    gated->stopped = true;
    QTRY_COMPARE(loadSpy.count(), 1);
    QCOMPARE(gated->eventsAfterStop.load(), 0);
    QVERIFY(gated->events.load() < 2 * ranges);
}

// What the process holds allocated on the heap, in bytes, where the platform tells.
static std::optional<qint64> heapInUse()
{
#if defined(Q_OS_MACOS)
    malloc_statistics_t statistics;
    malloc_zone_statistics(nullptr, &statistics);
    return qint64(statistics.size_in_use);
#elif defined(__GLIBC__) && __GLIBC_PREREQ(2, 33)
    const struct mallinfo2 info = mallinfo2();
    return qint64(info.uordblks + info.hblkhd);
#else
    return std::nullopt;
#endif
}

// Analyzing a short range of a long trace gives back the storage that held all of it.
void QmlProfilerTraceFileTest::testRestrictionReleasesTheStorage()
{
    if (!heapInUse())
        QSKIP("The heap in use cannot be measured on this platform.");

    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    QmlProfilerRangeModel javaScript(&manager, Javascript, &aggregator);
    MemoryUsageModel memory(&manager, &aggregator);
    manager.initialize();
    const int function = manager.appendEventType(
        QmlEventType(UndefinedMessage, Javascript, -1, QmlEventLocation("main.qml", 1, 1), "f"));
    const int allocation = manager.appendEventType(
        QmlEventType(MemoryAllocation, UndefinedRangeType, SmallItem));
    const int ranges = 500000;
    for (int i = 0; i < ranges; ++i) {
        QmlEvent event(i * 10, function, {});
        event.setRangeStage(RangeStart);
        manager.appendEvent(QmlEvent(event));
        event.setRangeStage(RangeEnd);
        event.setTimestamp(i * 10 + 5);
        manager.appendEvent(std::move(event));
        QmlEvent allocated(i * 10 + 6, allocation, {});
        allocated.setNumbers<qint64>({32});
        manager.appendEvent(std::move(allocated));
    }
    manager.decreaseTraceStart(0);
    manager.increaseTraceEnd(qint64(ranges) * 10);
    manager.finalize();
    QCOMPARE(javaScript.count(), ranges);
    const qint64 whole = *heapInUse();

    QSignalSpy finished(&manager, &QmlProfilerModelManager::restrictionFinished);
    manager.restrictToRange(0, 100);
    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(javaScript.count() < 20);
    const qint64 restricted = *heapInUse();
    // A JavaScript range alone takes 32 bytes in its model, 16 for its end and 8 for its rows.
    QVERIFY2(whole - restricted > qint64(ranges) * 40,
             qPrintable(QString("%1 MB before, %2 MB after").arg(whole / 1e6).arg(restricted / 1e6)));
}

// Records in which thread the preparers ran, and whether before the finalizers.
class PrepareRecorder
{
public:
    explicit PrepareRecorder(QmlProfilerModelManager &manager)
    {
        manager.registerFeatures(0, {}, [this] { finalized = true; }, {}, [this] {
            ++preparations;
            preparedInGuiThread = QThread::currentThread() == qApp->thread();
            preparedBeforeFinalize = !finalized;
        });
    }

    int preparations = 0;
    bool preparedInGuiThread = false;
    bool preparedBeforeFinalize = false;
    bool finalized = false;
};

static void fillRanges(QmlProfilerModelManager &manager)
{
    manager.initialize();
    const int typeIndex = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Javascript, -1,
                                         QmlEventLocation("main.qml", 1, 1), "f"));
    for (const auto &[start, end] : {std::pair{100, 110}, std::pair{200, 230}}) {
        QmlEvent event(start, typeIndex, {});
        event.setRangeStage(RangeStart);
        manager.appendEvent(QmlEvent(event));
        event.setRangeStage(RangeEnd);
        event.setTimestamp(end);
        manager.appendEvent(std::move(event));
    }
    manager.decreaseTraceStart(0);
    manager.increaseTraceEnd(1000);
    manager.finalize();
}

void QmlProfilerTraceFileTest::testLoadPreparesInLoadingThread_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::newRow("qtd") << "trace.qtd";
    QTest::newRow("qzt") << "trace.qzt";
}

void QmlProfilerTraceFileTest::testLoadPreparesInLoadingThread()
{
    QFETCH(QString, fileName);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(fileName);

    QmlProfilerModelManager saved;
    fillRanges(saved);
    QSignalSpy saveSpy(&saved, &QmlProfilerModelManager::saveFinished);
    saved.save(path);
    QTRY_COMPARE(saveSpy.count(), 1);

    QmlProfilerModelManager loaded;
    QmlProfilerStatisticsModel statistics(&loaded);
    PrepareRecorder recorder(loaded);
    QSignalSpy loadSpy(&loaded, &QmlProfilerModelManager::loadFinished);
    loaded.load(path);
    QTRY_COMPARE(loadSpy.count(), 1);

    QCOMPARE(recorder.preparations, 1);
    QVERIFY(!recorder.preparedInGuiThread);
    QVERIFY(recorder.preparedBeforeFinalize);
    QVERIFY(recorder.finalized);

    // The statistics are computed in the loading thread and published in the GUI thread.
    const int typeIndex = 0;
    QCOMPARE(statistics.data(statistics.index(typeIndex, MainCallCount), Qt::DisplayRole)
                 .toInt(), 2);
    QCOMPARE(statistics.data(statistics.index(typeIndex, MainMedianTime), SortRole)
                 .toLongLong(), 20);
}

void QmlProfilerTraceFileTest::testFinalizePreparesUnloadedTrace()
{
    // A live recording ends in finalize() with nothing prepared yet.
    QmlProfilerModelManager manager;
    QmlProfilerStatisticsModel statistics(&manager);
    PrepareRecorder recorder(manager);
    fillRanges(manager);

    QCOMPARE(recorder.preparations, 1);
    QVERIFY(recorder.preparedInGuiThread);
    QVERIFY(recorder.preparedBeforeFinalize);
    QCOMPARE(statistics.data(statistics.index(0, MainCallCount), Qt::DisplayRole).toInt(), 2);
}

// Analyzing a range replays all events, minutes of work for a long trace, so it runs in another
// thread and leaves the GUI responsive.
void QmlProfilerTraceFileTest::testRestrictionReplaysInAnotherThread()
{
    QmlProfilerModelManager manager;
    QmlProfilerStatisticsModel statistics(&manager);
    fillRanges(manager);
    PrepareRecorder recorder(manager);

    QSignalSpy started(&manager, &QmlProfilerModelManager::restrictionStarted);
    QSignalSpy finished(&manager, &QmlProfilerModelManager::restrictionFinished);
    manager.restrictToRange(150, 300);
    QCOMPARE(started.count(), 1);
    QTRY_COMPARE(finished.count(), 1);

    QCOMPARE(recorder.preparations, 1);
    QVERIFY(!recorder.preparedInGuiThread);
    QVERIFY(recorder.preparedBeforeFinalize);
    QVERIFY(recorder.finalized);
    // Only the range from 200 to 230 lies in the restricted range.
    QCOMPARE(statistics.data(statistics.index(0, MainCallCount), Qt::DisplayRole).toInt(), 1);
    QCOMPARE(statistics.data(statistics.index(0, MainMedianTime), SortRole).toLongLong(), 30);
}

// Like a load, analyzing a range replays the events in another thread. That stops calling into
// the models once cancelBackgroundWork() returned, too.
void QmlProfilerTraceFileTest::testCancelStopsTheRestrictionThread()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("trace.qzt");
    const int ranges = 100000;
    writeLongTrace(path, ranges);
    if (QTest::currentTestFailed())
        return;

    QmlProfilerModelManager manager;
    auto gated = std::make_shared<GatedLoader>();
    gated->armed = false;
    const auto open = qScopeGuard([gated] { gated->gate.release(); });
    addGatedLoader(&manager, gated);
    QSignalSpy loadSpy(&manager, &QmlProfilerModelManager::loadFinished);
    manager.load(path);
    QTRY_COMPARE(loadSpy.count(), 1);

    gated->armed = true;
    manager.restrictToRange(0, qint64(ranges) * 10);
    QVERIFY(gated->started.tryAcquire(1, 10000));
    gated->gate.release();
    manager.cancelBackgroundWork();
    gated->stopped = true;
    Utils::asyncThreadPool(QThread::InheritPriority)->waitForDone();
    QCOMPARE(gated->eventsAfterStop.load(), 0);
}

// A JavaScript range from 100 to 110 and a binding from 200 to 230, and after them
// extraJavaScript more JavaScript ranges, 10 apart.
static void fillJavaScriptAndBinding(QmlProfilerModelManager &manager, int *javaScriptType,
                                     int *bindingType, int extraJavaScript = 0)
{
    manager.initialize();
    *javaScriptType = manager.appendEventType(
        QmlEventType(UndefinedMessage, Javascript, -1, QmlEventLocation("main.qml", 1, 1), "f"));
    *bindingType = manager.appendEventType(
        QmlEventType(UndefinedMessage, Binding, -1, QmlEventLocation("main.qml", 2, 1), "b"));
    for (const auto &[type, start, end] : {std::tuple{*javaScriptType, 100, 110},
                                           std::tuple{*bindingType, 200, 230}}) {
        QmlEvent event(start, type, {});
        event.setRangeStage(RangeStart);
        manager.appendEvent(QmlEvent(event));
        event.setRangeStage(RangeEnd);
        event.setTimestamp(end);
        manager.appendEvent(std::move(event));
    }
    for (int i = 0; i < extraJavaScript; ++i) {
        QmlEvent event(1000 + i * 10, *javaScriptType, {});
        event.setRangeStage(RangeStart);
        manager.appendEvent(QmlEvent(event));
        event.setRangeStage(RangeEnd);
        event.setTimestamp(1005 + i * 10);
        manager.appendEvent(std::move(event));
    }
    manager.decreaseTraceStart(0);
    manager.increaseTraceEnd(1000 + extraJavaScript * 10);
    manager.finalize();
}

static int calls(const QmlProfilerStatisticsModel &statistics, int typeIndex)
{
    return statistics.data(statistics.index(typeIndex, MainCallCount), Qt::DisplayRole).toInt();
}

// Hiding or showing a category replays all events, which takes minutes for a long trace, so it
// happens in another thread.
void QmlProfilerTraceFileTest::testFeatureRestrictionReplaysInAnotherThread()
{
    QmlProfilerModelManager manager;
    QmlProfilerStatisticsModel statistics(&manager);
    int javaScriptType = -1;
    int bindingType = -1;
    fillJavaScriptAndBinding(manager, &javaScriptType, &bindingType);
    QCOMPARE(calls(statistics, javaScriptType), 1);
    QCOMPARE(calls(statistics, bindingType), 1);

    QSignalSpy started(&manager, &QmlProfilerModelManager::restrictionStarted);
    QSignalSpy finished(&manager, &QmlProfilerModelManager::restrictionFinished);
    statistics.restrictToFeatures(1ULL << ProfileJavaScript);
    QCOMPARE(started.count(), 1);
    QCOMPARE(finished.count(), 0);
    QTRY_COMPARE(finished.count(), 1);
    QCOMPARE(calls(statistics, javaScriptType), 1);
    QCOMPARE(calls(statistics, bindingType), 0);
}

// A category hidden while a range is analyzed applies to the analyzed range. The range holds
// enough events for the analysis to be still running.
void QmlProfilerTraceFileTest::testFeatureRestrictionDuringRangeRestriction()
{
    const int extraJavaScript = 100000;
    QmlProfilerModelManager manager;
    QmlProfilerStatisticsModel statistics(&manager);
    int javaScriptType = -1;
    int bindingType = -1;
    fillJavaScriptAndBinding(manager, &javaScriptType, &bindingType, extraJavaScript);

    QSignalSpy finished(&manager, &QmlProfilerModelManager::restrictionFinished);
    manager.restrictToRange(150, 1000 + extraJavaScript * 10);
    statistics.restrictToFeatures((1ULL << ProfileBinding) | (1ULL << ProfileJavaScript));
    statistics.restrictToFeatures(1ULL << ProfileJavaScript);
    QTRY_COMPARE(finished.count(), 1);
    QCOMPARE(calls(statistics, javaScriptType), extraJavaScript);
    QCOMPARE(calls(statistics, bindingType), 0);
}

} // namespace Profiler::Internal
