// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inputeventsmodel_test.h"
#include "../profilertr.h"

#include <tracing/timelineformattime.h>

#include <QTest>

using namespace QmlDebug;
namespace Profiler::Internal {

static InputEventType inputType(int i)
{
    return static_cast<InputEventType>(i % (MaximumInputEventType + 1));
}

InputEventsModelTest::InputEventsModelTest()
    : model(&manager, &aggregator)
{
    keyTypeId = manager.appendEventType(QmlEventType(Event, UndefinedRangeType, Key));
    mouseTypeId = manager.appendEventType(QmlEventType(Event, UndefinedRangeType, Mouse));
}

void InputEventsModelTest::initTestCase()
{
    manager.initialize();

    for (int i = 0; i < 10; ++i) {
        QmlEvent event;
        event.setTimestamp(i);
        InputEventType type = inputType(i);
        event.setTypeIndex(type <= InputKeyUnknown ? keyTypeId : mouseTypeId);
        event.setNumbers({static_cast<qint32>(type),
                          (i * 32) % 256,
                          static_cast<qint32>((i * 0x02000000) & Qt::KeyboardModifierMask)});
        manager.appendEvent(std::move(event));
    }

    manager.finalize();
}

void InputEventsModelTest::testTypeId()
{
    for (int i = 0; i < 10; ++i) {
        InputEventType type = inputType(i);
        QCOMPARE(model.typeId(i), type <= InputKeyUnknown ? keyTypeId : mouseTypeId);
    }
}

void InputEventsModelTest::testColor()
{
    QRgb keyColor = 0;
    QRgb mouseColor = 0;
    for (int i = 0; i < 10; ++i) {
        InputEventType type = inputType(i);
        int selectionId = (type <= InputKeyUnknown ? Key : Mouse);
        QCOMPARE(selectionId, model.selectionId(i));

        QRgb &expectedColor = selectionId == Key ? keyColor : mouseColor;
        if (expectedColor != 0)
            QCOMPARE(model.color(i), expectedColor);
        else
            expectedColor = model.color(i);
    }
}

void InputEventsModelTest::testLabels()
{
    Timeline::RowLabels labels = model.labels();
    QCOMPARE(labels[0].description, Tr::tr("Mouse Events"));
    QCOMPARE(labels[0].id, static_cast<int>(Mouse));

    QCOMPARE(labels[1].description, Tr::tr("Keyboard Events"));
    QCOMPARE(labels[1].id, static_cast<int>(Key));
}

void InputEventsModelTest::testDetails()
{
    for (int i = 0; i < 10; ++i) {
        const Timeline::ItemDetails details = model.details(i);
        QCOMPARE(details[Tr::tr("Timestamp")], Timeline::formatTime(i));
        QString displayName = details[QString("displayName")];
        QVERIFY(!displayName.isEmpty());
        switch (inputType(i)) {
        case InputKeyPress:
            QCOMPARE(displayName, Tr::tr("Key Press"));
            if (i == 0) {
                // all the numbers are 0 here, so no other members
                QVERIFY(!details.contains(Tr::tr("Key")));
                QVERIFY(!details.contains(Tr::tr("Modifiers")));
            } else {
                QCOMPARE(details[Tr::tr("Key")], QString("Key_Space"));
                QCOMPARE(details[Tr::tr("Modifiers")], QString("ShiftModifier|MetaModifier"));
            }
            break;
        case InputKeyRelease:
            QCOMPARE(displayName, Tr::tr("Key Release"));
            QCOMPARE(details[Tr::tr("Modifiers")], QString("ShiftModifier"));
            QCOMPARE(details[Tr::tr("Key")], QString("Key_Space"));
            break;
        case InputKeyUnknown:
            QCOMPARE(displayName, Tr::tr("Keyboard Event"));
            QVERIFY(!details.contains(Tr::tr("Key")));
            QVERIFY(!details.contains(Tr::tr("Modifiers")));
            break;
        case InputMousePress:
            QCOMPARE(displayName, Tr::tr("Mouse Press"));

            // 0x60 is not a valid mouse button
            QVERIFY(details.contains(Tr::tr("Button")));
            QVERIFY(details[Tr::tr("Button")].isEmpty());

            QCOMPARE(details[Tr::tr("Result")], QString("ExtraButton23|MaxMouseButton"));
            break;
        case InputMouseRelease:
            QCOMPARE(displayName, Tr::tr("Mouse Release"));
            QCOMPARE(details[Tr::tr("Button")], QString("ExtraButton5"));
            QVERIFY(details.contains(Tr::tr("Result")));
            QVERIFY(details[Tr::tr("Result")].isEmpty());
            break;
        case InputMouseMove:
            QCOMPARE(displayName, Tr::tr("Mouse Move"));
            QCOMPARE(details[Tr::tr("X")], QString("160"));
            QCOMPARE(details[Tr::tr("Y")], QString("167772160"));
            break;
        case InputMouseDoubleClick:
            QCOMPARE(displayName, Tr::tr("Double Click"));
            QVERIFY(details.contains(Tr::tr("Button")));
            QVERIFY(details[Tr::tr("Button")].isEmpty());
            QCOMPARE(details[Tr::tr("Result")], QString("MaxMouseButton"));
            break;
        case InputMouseWheel:
            QCOMPARE(displayName, Tr::tr("Mouse Wheel"));
            QCOMPARE(details[Tr::tr("Angle X")], QString("224"));
            QCOMPARE(details[Tr::tr("Angle Y")], QString("234881024"));
            break;
        case InputMouseUnknown:
            QCOMPARE(displayName, Tr::tr("Mouse Event"));
            QVERIFY(!details.contains(Tr::tr("X")));
            QVERIFY(!details.contains(Tr::tr("Y")));
            QVERIFY(!details.contains(Tr::tr("Angle X")));
            QVERIFY(!details.contains(Tr::tr("Angle Y")));
            QVERIFY(!details.contains(Tr::tr("Button")));
            QVERIFY(!details.contains(Tr::tr("Result")));
            break;
        default:
            QCOMPARE(displayName, Tr::tr("Unknown"));
            break;
        }
    }
}

void InputEventsModelTest::testExpandedRow()
{
    for (int i = 0; i < 10; ++i) {
        InputEventType type = inputType(i);
        QCOMPARE(model.expandedRow(i), (type <= InputKeyUnknown ? 2 : 1));
    }
}

void InputEventsModelTest::testCollapsedRow()
{
    for (int i = 0; i < 10; ++i)
        QCOMPARE(model.collapsedRow(i), 1);
}

void InputEventsModelTest::cleanupTestCase()
{
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.expandedRowCount(), 1);
    QCOMPARE(model.collapsedRowCount(), 1);
}

} // namespace Profiler::Internal
