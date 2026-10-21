// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertracefile_test.h"

#include "../memoryusagemodel.h"
#include "../qmlprofilermodelmanager.h"
#include "../qmlprofilerrangemodel.h"

#include <tracing/timelinemodelaggregator.h>

#include <QFile>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <atomic>
#include <memory>
#include <optional>

#if defined(Q_OS_MACOS)
#include <malloc/malloc.h>
#elif defined(__GLIBC__)
#include <malloc.h>
#endif

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
    std::atomic<bool> stopped = false;
    std::atomic<int> eventsAfterStop = 0;
};

} // namespace

static void addGatedLoader(QmlProfilerModelManager *manager,
                           const std::shared_ptr<GatedLoader> &gated)
{
    manager->qmlLoaders.append({1ULL << ProfileJavaScript,
                                [gated](const QmlEvent &, const QmlEventType &) {
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

    manager.restrictToRange(0, 100);
    QVERIFY(javaScript.count() < 20);
    const qint64 restricted = *heapInUse();
    // A JavaScript range alone takes 32 bytes in its model, 16 for its end and 8 for its rows.
    QVERIFY2(whole - restricted > qint64(ranges) * 40,
             qPrintable(QString("%1 MB before, %2 MB after").arg(whole / 1e6).arg(restricted / 1e6)));
}


} // namespace Profiler::Internal
