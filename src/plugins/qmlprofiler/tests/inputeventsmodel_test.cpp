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

#include "inputeventsmodel_test.h"
#include "timeline/timelinemodel_p.h"
#include "timeline/timelineformattime.h"

#include <QtTest>

namespace QmlProfiler {
namespace Internal {

static InputEventType inputType(int i)
{
    return static_cast<InputEventType>(i % (MaximumInputEventType + 1));
}

InputEventsModelTest::InputEventsModelTest(QObject *parent) :
    QObject(parent), manager(nullptr), model(&manager)
{
    keyTypeId = manager.numLoadedEventTypes();
    manager.addEventType(QmlEventType(Event, MaximumRangeType, Key));
    mouseTypeId = manager.numLoadedEventTypes();
    manager.addEventType(QmlEventType(Event, MaximumRangeType, Mouse));
}

void InputEventsModelTest::initTestCase()
{
    manager.startAcquiring();
    QmlEvent event;

    for (int i = 0; i < 10; ++i) {
        event.setTimestamp(i);
        InputEventType type = inputType(i);
        event.setTypeIndex(type <= InputKeyUnknown ? keyTypeId : mouseTypeId);
        event.setNumbers({static_cast<qint32>(type),
                          (i * 32) % 256,
                          static_cast<qint32>((i * 0x02000000) & Qt::KeyboardModifierMask)});
        manager.addEvent(event);
    }

    manager.acquiringDone();
    QCOMPARE(manager.state(), QmlProfilerModelManager::Done);
}

void InputEventsModelTest::testAccepted()
{
    QVERIFY(!model.accepted(QmlEventType()));
    QVERIFY(!model.accepted(QmlEventType(Event)));
    QVERIFY(!model.accepted(QmlEventType(Event, MaximumRangeType)));
    QVERIFY(model.accepted(QmlEventType(Event, MaximumRangeType, Key)));
    QVERIFY(model.accepted(QmlEventType(Event, MaximumRangeType, Mouse)));
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
    QVariantList labels = model.labels();
    QVariantMap mouseLabel = labels[0].toMap();
    QCOMPARE(mouseLabel[QString("description")].toString(), model.tr("Mouse Events"));
    QCOMPARE(mouseLabel[QString("id")].toInt(), static_cast<int>(Mouse));

    QVariantMap keyLabel = labels[1].toMap();
    QCOMPARE(keyLabel[QString("description")].toString(), model.tr("Keyboard Events"));
    QCOMPARE(keyLabel[QString("id")].toInt(), static_cast<int>(Key));
}

void InputEventsModelTest::testDetails()
{
    for (int i = 0; i < 10; ++i) {
        const QVariantMap details = model.details(i);
        QCOMPARE(details[model.tr("Timestamp")].toString(), Timeline::formatTime(i));
        QString displayName = details[QString("displayName")].toString();
        QVERIFY(!displayName.isEmpty());
        switch (inputType(i)) {
        case InputKeyPress:
            QCOMPARE(displayName, model.tr("Key Press"));
            if (i == 0) {
                // all the numbers are 0 here, so no other members
                QVERIFY(!details.contains(model.tr("Key")));
                QVERIFY(!details.contains(model.tr("Modifiers")));
            } else {
                QCOMPARE(details[model.tr("Key")].toString(), QString("Key_Space"));
                QCOMPARE(details[model.tr("Modifiers")].toString(),
                        QString("ShiftModifier|MetaModifier"));
            }
            break;
        case InputKeyRelease:
            QCOMPARE(displayName, model.tr("Key Release"));
            QCOMPARE(details[model.tr("Modifiers")].toString(), QString("ShiftModifier"));
            QCOMPARE(details[model.tr("Key")].toString(), QString("Key_Space"));
            break;
        case InputKeyUnknown:
            QCOMPARE(displayName, model.tr("Keyboard Event"));
            QVERIFY(!details.contains(model.tr("Key")));
            QVERIFY(!details.contains(model.tr("Modifiers")));
            break;
        case InputMousePress:
            QCOMPARE(displayName, model.tr("Mouse Press"));

            // 0x60 is not a valid mouse button
            QVERIFY(details.contains(model.tr("Button")));
            QVERIFY(details[model.tr("Button")].toString().isEmpty());

            QCOMPARE(details[model.tr("Result")].toString(),
                    QString("ExtraButton23|MaxMouseButton"));
            break;
        case InputMouseRelease:
            QCOMPARE(displayName, model.tr("Mouse Release"));
            QCOMPARE(details[model.tr("Button")].toString(), QString("ExtraButton5"));
            QVERIFY(details.contains(model.tr("Result")));
            QVERIFY(details[model.tr("Result")].toString().isEmpty());
            break;
        case InputMouseMove:
            QCOMPARE(displayName, model.tr("Mouse Move"));
            QCOMPARE(details[model.tr("X")].toString(), QString("160"));
            QCOMPARE(details[model.tr("Y")].toString(), QString("167772160"));
            break;
        case InputMouseDoubleClick:
            QCOMPARE(displayName, model.tr("Double Click"));
            QVERIFY(details.contains(model.tr("Button")));
            QVERIFY(details[model.tr("Button")].toString().isEmpty());
            QCOMPARE(details[model.tr("Result")].toString(), QString("MaxMouseButton"));
            break;
        case InputMouseWheel:
            QCOMPARE(displayName, model.tr("Mouse Wheel"));
            QCOMPARE(details[model.tr("Angle X")].toString(), QString("224"));
            QCOMPARE(details[model.tr("Angle Y")].toString(), QString("234881024"));
            break;
        case InputMouseUnknown:
            QCOMPARE(displayName, model.tr("Mouse Event"));
            QVERIFY(!details.contains(model.tr("X")));
            QVERIFY(!details.contains(model.tr("Y")));
            QVERIFY(!details.contains(model.tr("Angle X")));
            QVERIFY(!details.contains(model.tr("Angle Y")));
            QVERIFY(!details.contains(model.tr("Button")));
            QVERIFY(!details.contains(model.tr("Result")));
            break;
        default:
            QCOMPARE(displayName, model.tr("Unknown"));
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

} // namespace Internal
} // namespace QmlProfiler
