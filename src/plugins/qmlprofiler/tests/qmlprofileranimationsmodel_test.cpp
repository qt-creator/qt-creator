// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofileranimationsmodel_test.h"
#include "../qmlprofilertr.h"
#include <tracing/timelineformattime.h>
#include <QTest>

using namespace QmlDebug;
namespace QmlProfiler::Internal {

QmlProfilerAnimationsModelTest::QmlProfilerAnimationsModelTest()
    : model(&manager, &aggregator)
{}

static int frameRate(int i)
{
    return i * 7 - 2;
}

void QmlProfilerAnimationsModelTest::initTestCase()
{
    manager.initialize();

    QmlEvent event;
    event.setTypeIndex(manager.appendEventType(
                           QmlEventType(Event, UndefinedRangeType, AnimationFrame)));

    for (int i = 0; i < 10; ++i) {
        event.setTimestamp(i);
        event.setNumbers<int>({frameRate(i), 9 - i, (i % 2) ? RenderThread : GuiThread});
        manager.appendEvent(QmlEvent(event));
    }
    manager.finalize();
}

void QmlProfilerAnimationsModelTest::testRowMaxValue()
{
    QCOMPARE(model.rowMaxValue(0), 0);
    QCOMPARE(model.rowMaxValue(1), 9);
    QCOMPARE(model.rowMaxValue(2), 8);
}

void QmlProfilerAnimationsModelTest::testRowNumbers()
{
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(model.expandedRow(i), (i % 2) ? 2 : 1);
        QCOMPARE(model.collapsedRow(i), model.expandedRow(i));
    }
}

void QmlProfilerAnimationsModelTest::testTypeId()
{
    for (int i = 0; i < 10; ++i)
        QCOMPARE(model.typeId(i), 0);
}

void QmlProfilerAnimationsModelTest::testColor()
{
    QColor last = QColor::fromHsl(0, 0, 0);
    for (int i = 0; i < 10; ++i) {
        QColor next = model.color(i);
        QVERIFY(next.hue() > last.hue());
        last = next;
    }
}

void QmlProfilerAnimationsModelTest::testRelativeHeight()
{
    float last = 1;
    for (int i = 0; i < 10; ++i) {
        float next = model.relativeHeight(i);
        QVERIFY(next <= last);
        last = next;
    }
}

void QmlProfilerAnimationsModelTest::testLabels()
{
    Timeline::RowLabels labels = model.labels();
    QCOMPARE(labels.length(), 2);

    QCOMPARE(labels[0].description, Tr::tr("GUI Thread"));
    QCOMPARE(labels[0].id, static_cast<int>(GuiThread));

    QCOMPARE(labels[1].description, Tr::tr("Render Thread"));
    QCOMPARE(labels[1].id, static_cast<int>(RenderThread));
}

void QmlProfilerAnimationsModelTest::testDetails()
{
    for (int i = 0; i < 10; ++i) {
        Timeline::ItemDetails details = model.details(i);
        QCOMPARE(details["displayName"], model.displayName());
        QCOMPARE(details[Tr::tr("Duration")], Timeline::formatTime(1));
        QCOMPARE(details[Tr::tr("Framerate")],
                QString::fromLatin1("%1 FPS").arg(frameRate(i)));
        QCOMPARE(details[Tr::tr("Animations")], QString::number(9 - i));
        QCOMPARE(details[Tr::tr("Context")], i % 2 ?
                    Tr::tr("Render Thread") :
                    Tr::tr("GUI Thread"));
    }
}

void QmlProfilerAnimationsModelTest::cleanupTestCase()
{
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.expandedRowCount(), 1);
    QCOMPARE(model.collapsedRowCount(), 1);
}

} // namespace QmlProfiler::Internal
