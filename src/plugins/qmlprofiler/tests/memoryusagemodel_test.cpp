// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "memoryusagemodel_test.h"
#include "../qmlprofilertr.h"
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

MemoryUsageModelTest::MemoryUsageModelTest(QObject *parent) : QObject(parent),
    model(&manager, &aggregator)
{
}

void MemoryUsageModelTest::initTestCase()
{
    manager.initialize();
    qint64 timestamp = 0;


    heapPageTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(MemoryAllocation, UndefinedRangeType, HeapPage));
    smallItemTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(MemoryAllocation, UndefinedRangeType, SmallItem));
    largeItemTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(MemoryAllocation, UndefinedRangeType, LargeItem));

    auto addMemoryEvents = [&]() {
        QmlEvent event;
        event.setTimestamp(++timestamp);
        event.setTypeIndex(heapPageTypeId);
        event.setNumbers({2048});
        manager.appendEvent(QmlEvent(event));
        manager.appendEvent(QmlEvent(event)); // allocate two of them and make the model summarize

        event.setTimestamp(++timestamp);
        event.setTypeIndex(smallItemTypeId);
        event.setNumbers({32});
        manager.appendEvent(QmlEvent(event));

        event.setTimestamp(++timestamp);
        event.setTypeIndex(largeItemTypeId);
        event.setNumbers({1024});
        manager.appendEvent(QmlEvent(event));

        event.setTimestamp(++timestamp);
        event.setTypeIndex(smallItemTypeId);
        event.setNumbers({-32});
        manager.appendEvent(std::move(event));
    };

    addMemoryEvents();

    rangeTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Javascript, -1,
                                         QmlEventLocation(QString("somefile.js"), 10, 20),
                                         QString("funcfunc")));

    QmlEvent event;
    event.setRangeStage(RangeStart);
    event.setTimestamp(++timestamp);
    event.setTypeIndex(rangeTypeId);
    manager.appendEvent(QmlEvent(event));

    addMemoryEvents();
    addMemoryEvents(); // twice to also trigger summary in first row

    event.setRangeStage(RangeEnd);
    event.setTimestamp(++timestamp);
    manager.appendEvent(QmlEvent(event));

    event.setTimestamp(++timestamp);
    event.setTypeIndex(largeItemTypeId);
    event.setNumbers({-1024});
    manager.appendEvent(std::move(event));

    manager.finalize();
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
    QCOMPARE(allocations[QString("description")].toString(), Tr::tr("Memory Allocation"));
    QCOMPARE(allocations[QString("id")].toInt(), static_cast<int>(HeapPage));

    const QVariantMap usages = labels[1].toMap();
    QCOMPARE(usages[QString("description")].toString(), Tr::tr("Memory Usage"));
    QCOMPARE(usages[QString("id")].toInt(), static_cast<int>(SmallItem));
}

void MemoryUsageModelTest::testDetails()
{
    const QVariantMap allocated = model.details(0);
    QCOMPARE(allocated[QString("displayName")].toString(), Tr::tr("Memory Allocated"));
    QCOMPARE(allocated[Tr::tr("Total")].toString(),
            Tr::tr("%n byte(s)", nullptr, 4096));
    QCOMPARE(allocated[Tr::tr("Allocated")].toString(),
            Tr::tr("%n byte(s)", nullptr, 4096));
    QCOMPARE(allocated[Tr::tr("Allocations")].toString(), QString::number(2));
    QCOMPARE(allocated[Tr::tr("Type")].toString(), Tr::tr("Heap Allocation"));
    QCOMPARE(allocated[Tr::tr("Location")].toString(), Tr::tr("<bytecode>"));

    QVERIFY(!allocated.contains(Tr::tr("Deallocated")));
    QVERIFY(!allocated.contains(Tr::tr("Deallocations")));

    const QVariantMap large = model.details(2);
    QCOMPARE(large[QString("displayName")].toString(), Tr::tr("Memory Allocated"));
    QCOMPARE(large[Tr::tr("Total")].toString(),
            Tr::tr("%n byte(s)", nullptr, 5120));
    QCOMPARE(large[Tr::tr("Allocated")].toString(),
            Tr::tr("%n byte(s)", nullptr, 1024));
    QCOMPARE(large[Tr::tr("Allocations")].toString(), QString::number(1));
    QCOMPARE(large[Tr::tr("Type")].toString(), Tr::tr("Large Item Allocation"));
    QCOMPARE(large[Tr::tr("Location")].toString(), Tr::tr("<bytecode>"));

    QVERIFY(!large.contains(Tr::tr("Deallocated")));
    QVERIFY(!large.contains(Tr::tr("Deallocations")));

    const QVariantMap freed = model.details(9);
    QCOMPARE(freed[QString("displayName")].toString(), Tr::tr("Memory Freed"));
    QCOMPARE(freed[Tr::tr("Total")].toString(), Tr::tr("%n byte(s)", nullptr, 2048));
    QCOMPARE(freed[Tr::tr("Deallocated")].toString(), Tr::tr("%n byte(s)", nullptr, 1024));
    QCOMPARE(freed[Tr::tr("Deallocations")].toString(), QString::number(1));
    QCOMPARE(freed[Tr::tr("Type")].toString(), Tr::tr("Heap Usage"));
    QCOMPARE(freed[Tr::tr("Location")].toString(), Tr::tr("<bytecode>"));

    QVERIFY(!freed.contains(Tr::tr("Allocated")));
    QVERIFY(!freed.contains(Tr::tr("Allocations")));
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

void MemoryUsageModelTest::cleanupTestCase()
{
    manager.clearAll();
    QCOMPARE(model.count(), 0);
}

} // namespace Internal
} // namespace QmlProfiler
