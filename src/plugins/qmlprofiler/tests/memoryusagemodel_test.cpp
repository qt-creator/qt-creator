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

#include "memoryusagemodel_test.h"
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

MemoryUsageModelTest::MemoryUsageModelTest(QObject *parent) : QObject(parent),
    manager(nullptr), model(&manager)
{
}

void MemoryUsageModelTest::initTestCase()
{
    manager.startAcquiring();
    qint64 timestamp = 0;


    heapPageTypeId = manager.numLoadedEventTypes();
    manager.addEventType(QmlEventType(MemoryAllocation, MaximumRangeType, HeapPage));
    smallItemTypeId = manager.numLoadedEventTypes();
    manager.addEventType(QmlEventType(MemoryAllocation, MaximumRangeType, SmallItem));
    largeItemTypeId = manager.numLoadedEventTypes();
    manager.addEventType(QmlEventType(MemoryAllocation, MaximumRangeType, LargeItem));

    auto addMemoryEvents = [&]() {
        QmlEvent event;
        event.setTimestamp(++timestamp);
        event.setTypeIndex(heapPageTypeId);
        event.setNumbers({2048});
        manager.addEvent(event);
        manager.addEvent(event); // allocate two of them and make the model summarize

        event.setTimestamp(++timestamp);
        event.setTypeIndex(smallItemTypeId);
        event.setNumbers({32});
        manager.addEvent(event);

        event.setTimestamp(++timestamp);
        event.setTypeIndex(largeItemTypeId);
        event.setNumbers({1024});
        manager.addEvent(event);

        event.setTimestamp(++timestamp);
        event.setTypeIndex(smallItemTypeId);
        event.setNumbers({-32});
        manager.addEvent(event);
    };

    addMemoryEvents();

    rangeTypeId = manager.numLoadedEventTypes();
    manager.addEventType(QmlEventType(MaximumMessage, Javascript, -1,
                                      QmlEventLocation(QString("somefile.js"), 10, 20),
                                      QString("funcfunc")));

    QmlEvent event;
    event.setRangeStage(RangeStart);
    event.setTimestamp(++timestamp);
    event.setTypeIndex(rangeTypeId);
    manager.addEvent(event);

    addMemoryEvents();
    addMemoryEvents(); // twice to also trigger summary in first row

    event.setRangeStage(RangeEnd);
    event.setTimestamp(++timestamp);
    manager.addEvent(event);

    event.setTimestamp(++timestamp);
    event.setTypeIndex(largeItemTypeId);
    event.setNumbers({-1024});
    manager.addEvent(event);

    manager.acquiringDone();
    QCOMPARE(manager.state(), QmlProfilerModelManager::Done);
    QCOMPARE(model.count(), 11);
}

void MemoryUsageModelTest::testRowMaxValue()
{
    QCOMPARE(model.rowMaxValue(1), 3 * (1024 + 4096));
    QCOMPARE(model.rowMaxValue(2), 3 * (1024 + 4096));
}

void MemoryUsageModelTest::testTypeId()
{
    QCOMPARE(model.typeId(0), heapPageTypeId);
    QCOMPARE(model.typeId(1), smallItemTypeId);
    QCOMPARE(model.typeId(2), largeItemTypeId);
    QCOMPARE(model.typeId(3), smallItemTypeId);

    for (int i = 4; i < 9; ++i)
        QCOMPARE(model.typeId(i), rangeTypeId);

    QCOMPARE(model.typeId(9), largeItemTypeId);
    QCOMPARE(model.typeId(10), largeItemTypeId);
}

void MemoryUsageModelTest::testColor()
{
    QRgb heapPageColor = model.color(0);
    QRgb smallItemColor = model.color(1);
    QRgb largeItemColor = model.color(2);
    QVERIFY(smallItemColor != heapPageColor);
    QVERIFY(largeItemColor != heapPageColor);
    QVERIFY(smallItemColor != largeItemColor);
    QCOMPARE(model.color(4), heapPageColor);
    QCOMPARE(model.color(8), largeItemColor);
}

void MemoryUsageModelTest::testLabels()
{
    const QVariantList labels = model.labels();

    const QVariantMap allocations = labels[0].toMap();
    QCOMPARE(allocations[QString("description")].toString(), model.tr("Memory Allocation"));
    QCOMPARE(allocations[QString("id")].toInt(), static_cast<int>(HeapPage));

    const QVariantMap usages = labels[1].toMap();
    QCOMPARE(usages[QString("description")].toString(), model.tr("Memory Usage"));
    QCOMPARE(usages[QString("id")].toInt(), static_cast<int>(SmallItem));
}

void MemoryUsageModelTest::testDetails()
{
    const QVariantMap allocated = model.details(0);
    QCOMPARE(allocated[QString("displayName")].toString(), model.tr("Memory Allocated"));
    QCOMPARE(allocated[model.tr("Total")].toString(), model.tr("%1 bytes").arg(4096));
    QCOMPARE(allocated[model.tr("Allocated")].toString(), model.tr("%1 bytes").arg(4096));
    QCOMPARE(allocated[model.tr("Allocations")].toString(), QString::number(2));
    QCOMPARE(allocated[model.tr("Type")].toString(), model.tr("Heap Allocation"));
    QCOMPARE(allocated[model.tr("Location")].toString(), QmlProfilerModelManager::tr("<bytecode>"));

    QVERIFY(!allocated.contains(model.tr("Deallocated")));
    QVERIFY(!allocated.contains(model.tr("Deallocations")));

    const QVariantMap large = model.details(2);
    QCOMPARE(large[QString("displayName")].toString(), model.tr("Memory Allocated"));
    QCOMPARE(large[model.tr("Total")].toString(), model.tr("%1 bytes").arg(5120));
    QCOMPARE(large[model.tr("Allocated")].toString(), model.tr("%1 bytes").arg(1024));
    QCOMPARE(large[model.tr("Allocations")].toString(), QString::number(1));
    QCOMPARE(large[model.tr("Type")].toString(), model.tr("Large Item Allocation"));
    QCOMPARE(large[model.tr("Location")].toString(), QmlProfilerModelManager::tr("<bytecode>"));

    QVERIFY(!large.contains(model.tr("Deallocated")));
    QVERIFY(!large.contains(model.tr("Deallocations")));

    const QVariantMap freed = model.details(9);
    QCOMPARE(freed[QString("displayName")].toString(), model.tr("Memory Freed"));
    QCOMPARE(freed[model.tr("Total")].toString(), model.tr("%1 bytes").arg(2048));
    QCOMPARE(freed[model.tr("Deallocated")].toString(), model.tr("%1 bytes").arg(1024));
    QCOMPARE(freed[model.tr("Deallocations")].toString(), QString::number(1));
    QCOMPARE(freed[model.tr("Type")].toString(), model.tr("Heap Usage"));
    QCOMPARE(freed[model.tr("Location")].toString(), QmlProfilerModelManager::tr("<bytecode>"));

    QVERIFY(!freed.contains(model.tr("Allocated")));
    QVERIFY(!freed.contains(model.tr("Allocations")));
}

void MemoryUsageModelTest::testExpandedRow()
{
    QCOMPARE(model.expandedRow(0), 1);
    QCOMPARE(model.expandedRow(1), 2); // LargeItem event is expanded to two timeline items
    QCOMPARE(model.expandedRow(2), 1);
    QCOMPARE(model.expandedRow(3), 2); // Allocate and free tracked separately outside ranges
    QCOMPARE(model.expandedRow(4), 1);
    QCOMPARE(model.expandedRow(5), 2);
}

void MemoryUsageModelTest::testCollapsedRow()
{
    QCOMPARE(model.collapsedRow(0), 1);
    QCOMPARE(model.collapsedRow(1), 2); // LargeItem event is expanded to two timeline items
    QCOMPARE(model.collapsedRow(2), 1);
    QCOMPARE(model.collapsedRow(3), 2); // Allocate and free tracked separately outside ranges
    QCOMPARE(model.collapsedRow(4), 1);
    QCOMPARE(model.collapsedRow(5), 2);
}

void MemoryUsageModelTest::testLocation()
{
    const QVariantMap empty = model.location(0);
    QVERIFY(empty[QString("file")].toString().isEmpty());
    QCOMPARE(empty[QString("line")].toInt(), -1);
    QCOMPARE(empty[QString("column")].toInt(), -1);

    const QVariantMap range = model.location(6);
    QCOMPARE(range[QString("file")].toString(), QString("somefile.js"));
    QCOMPARE(range[QString("line")].toInt(), 10);
    QCOMPARE(range[QString("column")].toInt(), 20);
}

void MemoryUsageModelTest::testRelativeHeight()
{
    float heights[] = {0.266667f, 0.06875f, 0.333333f, 0.0666667f, 0.6f, 0.2f,
                       0.666667f, 0.933333f, 1.0f, 0.133333f, 0.933333f};
    for (int i = 0; i < 11; ++i)
        QCOMPARE(model.relativeHeight(i), heights[i]);
}

void MemoryUsageModelTest::testAccepted()
{
    QVERIFY(!model.accepted(QmlEventType()));
    QVERIFY(!model.accepted(QmlEventType(MaximumMessage, MaximumRangeType, HeapPage)));
    QVERIFY(model.accepted(QmlEventType(MemoryAllocation, MaximumRangeType, HeapPage)));
    QVERIFY(model.accepted(QmlEventType(MaximumMessage, Javascript)));
}

void MemoryUsageModelTest::cleanupTestCase()
{
    manager.clear();
    QCOMPARE(model.count(), 0);
}

} // namespace Internal
} // namespace QmlProfiler
