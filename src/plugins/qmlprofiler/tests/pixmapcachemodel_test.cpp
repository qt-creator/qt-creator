/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "pixmapcachemodel_test.h"
#include <timeline/timelineformattime.h>
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

PixmapCacheModelTest::PixmapCacheModelTest(QObject *parent) : QObject(parent),
    manager(nullptr), model(&manager)
{
}

void PixmapCacheModelTest::initTestCase()
{
    manager.startAcquiring();
    manager.traceTime()->decreaseStartTime(1);
    manager.traceTime()->increaseEndTime(300);

    for (int i = 0; i < MaximumPixmapEventType; ++i) {
        eventTypeIndices[i] = manager.numLoadedEventTypes();
        manager.addEventType(QmlEventType(PixmapCacheEvent, MaximumRangeType, i,
                                          QmlEventLocation("dings.png", 0, 0)));
    }

    // random data, should still result in consistent model.
    for (int i = 0; i < 100; ++i) {
        QmlEvent event;
        event.setTypeIndex(eventTypeIndices[(i * 13) % MaximumPixmapEventType]);
        event.setTimestamp(i);
        event.setNumbers({i + 1, i - 1, i * 2});
        manager.addEvent(event);
    }

    for (int i = 0; i < MaximumPixmapEventType; ++i) {
        eventTypeIndices[i + MaximumPixmapEventType] = manager.numLoadedEventTypes();
        manager.addEventType(QmlEventType(PixmapCacheEvent, MaximumRangeType, i,
                                          QmlEventLocation("blah.png", 0, 0)));
    }


    // Explicitly test some "interesting" things
    int nextItem = model.count();
    QmlEvent event;
    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapLoadingStarted]);
    event.setTimestamp(101);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncached);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Loading);

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapCacheCountChanged]);
    event.setNumbers({0, 0, 200}); // cache count increase
    event.setTimestamp(102);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::ToBeCached);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Loading);

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapLoadingError]);
    event.setTimestamp(103);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Corrupt);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Error);

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapCacheCountChanged]);
    event.setNumbers({0, 0, 199}); // cache count decrease
    event.setTimestamp(104);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncacheable);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Error);


    ++nextItem;
    QCOMPARE(model.count(), nextItem);
    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapLoadingStarted]);
    event.setTimestamp(105);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncached);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Loading);

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapLoadingError]);
    event.setTimestamp(106);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncacheable);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Error);

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapCacheCountChanged]);
    // We still cache the previous item, even though it's uncacheable.
    // This way we get a corrupt cache entry ...
    event.setNumbers({0, 0, 200});
    event.setTimestamp(107);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem - 1), PixmapCacheModel::Corrupt);
    QCOMPARE(model.loadState(nextItem - 1), PixmapCacheModel::Error);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncacheable);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Error);

    // ... which is immediately removed by another cache count change
    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapCacheCountChanged]);
    event.setNumbers({0, 0, 199}); // cache count decrease, removes the corrupt entry
    event.setTimestamp(108);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem - 1), PixmapCacheModel::Uncacheable);
    QCOMPARE(model.loadState(nextItem - 1), PixmapCacheModel::Error);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncacheable);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Error);


    ++nextItem;
    QCOMPARE(model.count(), nextItem);
    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapLoadingStarted]);
    event.setTimestamp(109);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncached);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Loading);

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapCacheCountChanged]);
    event.setNumbers({0, 0, 200}); // cache count increase
    event.setTimestamp(110);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::ToBeCached);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Loading);

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapSizeKnown]);
    event.setNumbers({50, 50});
    event.setTimestamp(111);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Cached);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Loading);

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapCacheCountChanged]);
    event.setNumbers({0, 0, 199}); // cache count decrease
    event.setTimestamp(112);
    manager.addEvent(event);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncached);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Loading);

    // one pixmap item, and two cache count changes with known size
    QCOMPARE(model.count(), nextItem + 3);
    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapSizeKnown]);
    event.setNumbers({20, 30});
    event.setTimestamp(113);
    manager.addEvent(event); // Results in Uncached, with valid size
    QCOMPARE(model.count(), nextItem + 3); // no item added here; we just store the size

    event.setTypeIndex(eventTypeIndices[MaximumPixmapEventType + PixmapLoadingError]);
    event.setTimestamp(114);
    // terminates the still loading item, adding another cache count change
    manager.addEvent(event);
    QCOMPARE(model.count(), nextItem + 4);
    QCOMPARE(model.cacheState(nextItem), PixmapCacheModel::Uncacheable);
    QCOMPARE(model.loadState(nextItem), PixmapCacheModel::Error);


    // some more random data
    for (int i = MaximumPixmapEventType; i < 2 * MaximumPixmapEventType; ++i) {
        for (int j = 0; j < i; ++j) {
            QmlEvent event;
            event.setTypeIndex(eventTypeIndices[i]);
            event.setTimestamp(i + j + 200);
            event.setNumbers({i + 1, i - 1, i - j});
            manager.addEvent(event);
        }
    }


    manager.acquiringDone();

    QCOMPARE(manager.state(), QmlProfilerModelManager::Done);
}

void PixmapCacheModelTest::testConsistency()
{
    // one unique URL
    QCOMPARE(model.expandedRowCount(), 4);
    QVERIFY(model.collapsedRowCount() >= model.expandedRowCount());

    float prevHeight = 0;
    qint64 currentTime = -1;
    qint64 currentEnd = -1;
    QHash<int, qint64> collapsedEnds;
    for (int i = 0; i < model.count(); ++i) {
        int *typesEnd = eventTypeIndices + 2 * MaximumPixmapEventType;
        QVERIFY(std::find(eventTypeIndices, typesEnd, model.typeId(i)) != typesEnd);

        QVERIFY(model.startTime(i) >= currentTime);
        currentTime = model.startTime(i);
        QVERIFY(currentTime >= manager.traceTime()->startTime());
        currentEnd = model.endTime(i);
        QVERIFY(currentEnd <= manager.traceTime()->endTime());

        const QVariantMap details = model.details(i);

        int collapsedRow = model.collapsedRow(i);
        int expandedRow = model.expandedRow(i);

        switch (collapsedRow) {
        case 0:
            QFAIL("There should be no events in row 0");
            break;
        case 1:
            QVERIFY(model.relativeHeight(i) != prevHeight);
            prevHeight = model.relativeHeight(i);
            QCOMPARE(expandedRow, 1);
            break;
        default:
            QVERIFY(expandedRow > 1);
            QCOMPARE(model.relativeHeight(i), 1.0f);
            QVERIFY(collapsedRow < model.collapsedRowCount());
            if (collapsedEnds.contains(collapsedRow))
                QVERIFY(collapsedEnds[collapsedRow] <= currentTime);
            collapsedEnds[collapsedRow] = model.endTime(i);
        }


        switch (expandedRow) {
        case 0:
            QFAIL("There should be no events in row 0");
            break;
        case 1:
            QCOMPARE(collapsedRow, 1);
            QVERIFY(details[QLatin1String("displayName")].toString() == model.tr("Image Cached"));
            QVERIFY(details.contains(model.tr("Cache Size")));
            break;
        default:
            QVERIFY(collapsedRow > 1);
            QCOMPARE(model.relativeHeight(i), 1.0f);
            QVERIFY(expandedRow < model.expandedRowCount());
            QVERIFY(details[QLatin1String("displayName")].toString() == model.tr("Image Loaded"));
            QCOMPARE(details[model.tr("Duration")].toString(),
                    Timeline::formatTime(model.duration(i)));
            // In expanded view pixmaps of the same URL but different sizes are allowed to overlap.
            // It looks bad, but that should be a rare thing.
            break;
        }

        QString filename = details[model.tr("File")].toString();
        QVERIFY(filename == QString("dings.png") || filename == QString("blah.png"));
        QVERIFY(details.contains(model.tr("Width")));
        QVERIFY(details.contains(model.tr("Height")));
    }
}

void PixmapCacheModelTest::testRowMaxValue()
{
    QCOMPARE(model.rowMaxValue(2), 0);
    QVERIFY(model.rowMaxValue(1) > 0);
}

void PixmapCacheModelTest::testColor()
{
    QRgb row1Color = 0;
    QRgb dingsColor = 0;
    QRgb blahColor = 0;
    for (int i = 0; i < model.count(); ++i) {
        if (model.collapsedRow(i) == 1) {
            if (row1Color == 0)
                row1Color = model.color(i);
            else
                QCOMPARE(model.color(i), row1Color);
        } else {
            QRgb &pixmapColor = (model.fileName(i) == QString("blah.png")) ?
                        blahColor : dingsColor;
            if (pixmapColor == 0)
                pixmapColor = model.color(i);
            else
                QCOMPARE(model.color(i), pixmapColor);
        }
    }
}

void PixmapCacheModelTest::testLabels()
{
    const QVariantList labels = model.labels();
    QCOMPARE(labels.length(), 3);

    const QVariantMap countRow = labels[0].toMap();

    QCOMPARE(countRow[QString("description")].toString(), model.tr("Cache Size"));
    QCOMPARE(countRow[QString("id")].toInt(), 0);

    const QVariantMap dingsRow = labels[1].toMap();
    QCOMPARE(dingsRow[QString("description")].toString(), QString("dings.png"));
    QCOMPARE(dingsRow[QString("id")].toInt(), 1); // urlIndex + 1

    const QVariantMap blahRow = labels[2].toMap();
    QCOMPARE(blahRow[QString("description")].toString(), QString("blah.png"));
    QCOMPARE(blahRow[QString("id")].toInt(), 2); // urlIndex + 1
}

void PixmapCacheModelTest::cleanupTestCase()
{
    manager.clear();
    QCOMPARE(model.count(), 0);
}

} // namespace Internal
} // namespace QmlProfiler
