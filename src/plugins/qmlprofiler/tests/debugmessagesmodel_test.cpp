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

#include "debugmessagesmodel_test.h"

#include <timeline/timelineformattime.h>

#include <QtTest>

namespace QmlProfiler {
namespace Internal {

DebugMessagesModelTest::DebugMessagesModelTest(QObject *parent) :
    QObject(parent), manager(nullptr), model(&manager)
{
}

void DebugMessagesModelTest::initTestCase()
{
    manager.startAcquiring();
    QmlEvent event;
    event.setTypeIndex(-1);

    for (int i = 0; i < 10; ++i) {
        event.setTimestamp(i);
        event.setString(QString::fromLatin1("message %1").arg(i));
        QmlEventType type(DebugMessage, MaximumRangeType, i % (QtMsgType::QtInfoMsg + 1),
                          QmlEventLocation("somefile.js", i, 10 - i));
        event.setTypeIndex(manager.numLoadedEventTypes());
        manager.addEventType(type);
        manager.addEvent(event);
    }
    manager.acquiringDone();
    QCOMPARE(manager.state(), QmlProfilerModelManager::Done);
}

void DebugMessagesModelTest::testTypeId()
{
    // Each event should have a different type and they should be the only ones
    for (int i = 0; i < 10; ++i)
        QCOMPARE(model.typeId(i), i);
}

void DebugMessagesModelTest::testColor()
{
    // TimelineModel::colorBySelectionId ...
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(model.color(i),
                 QColor::fromHsl((i % (QtMsgType::QtInfoMsg + 1) * 25) % 360, 150, 166).rgb());
    }
}

static const char *messageTypes[] = {
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Debug Message"),
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Warning Message"),
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Critical Message"),
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Fatal Message"),
    QT_TRANSLATE_NOOP("DebugMessagesModel", "Info Message"),
};

void DebugMessagesModelTest::testLabels()
{
    QVariantList labels = model.labels();
    for (int i = 0; i <= QtMsgType::QtInfoMsg; ++i) {
        QVariantMap element = labels[i].toMap();
        QCOMPARE(element[QLatin1String("description")].toString(), model.tr(messageTypes[i]));
        QCOMPARE(element[QLatin1String("id")].toInt(), i);
    }
}

void DebugMessagesModelTest::testDetails()
{
    for (int i = 0; i < 10; ++i) {
        QVariantMap details = model.details(i);
        QCOMPARE(details.value(QLatin1String("displayName")).toString(),
                 model.tr(messageTypes[i % (QtMsgType::QtInfoMsg + 1)]));
        QCOMPARE(details.value(model.tr("Timestamp")).toString(),
                 Timeline::formatTime(i));
        QCOMPARE(details.value(model.tr("Message")).toString(),
                 QString::fromLatin1("message %1").arg(i));
        QCOMPARE(details.value(model.tr("Location")).toString(),
                 QString::fromLatin1("somefile.js:%1").arg(i));
    }
}

void DebugMessagesModelTest::testExpandedRow()
{
    for (int i = 0; i < 10; ++i)
        QCOMPARE(model.expandedRow(i), (i % (QtMsgType::QtInfoMsg + 1) + 1));
}

void DebugMessagesModelTest::testCollapsedRow()
{
    for (int i = 0; i < 10; ++i)
        QCOMPARE(model.collapsedRow(i), 1);
}

void DebugMessagesModelTest::testLocation()
{
    QVariantMap expected;
    expected[QLatin1String("file")] = QLatin1String("somefile.js");

    for (int i = 0; i < 10; ++i) {
        expected[QLatin1String("line")] = i;
        expected[QLatin1String("column")] = 10 - i;
        QCOMPARE(model.location(i), expected);
    }
}

void DebugMessagesModelTest::cleanupTestCase()
{
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.expandedRowCount(), 1);
    QCOMPARE(model.collapsedRowCount(), 1);
}

} // namespace Internal
} // namespace QmlProfiler
