// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertracefile_test.h"

#include "../qmlprofilermodelmanager.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

using Ranges = QList<std::pair<qint64, qint64>>;

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

} // namespace Profiler::Internal
