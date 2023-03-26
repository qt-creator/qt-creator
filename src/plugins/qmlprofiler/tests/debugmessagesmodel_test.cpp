// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugmessagesmodel_test.h"
#include "../qmlprofilertr.h"

#include <tracing/timelineformattime.h>

#include <QtTest>

namespace QmlProfiler {
namespace Internal {

DebugMessagesModelTest::DebugMessagesModelTest(QObject *parent) :
    QObject(parent), model(&manager, &aggregator)
{
}

void DebugMessagesModelTest::initTestCase()
{
    manager.initialize();

    for (int i = 0; i < 10; ++i) {
        QmlEvent event;
        event.setTimestamp(i);
        event.setString(QString::fromLatin1("message %1").arg(i));
        QmlEventType type(DebugMessage, UndefinedRangeType, i % (QtMsgType::QtInfoMsg + 1),
                          QmlEventLocation("somefile.js", i, 10 - i));
        event.setTypeIndex(manager.numEventTypes());
        manager.appendEventType(std::move(type));
        manager.appendEvent(std::move(event));
    }
    manager.finalize();
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
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Debug Message"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Warning Message"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Critical Message"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Fatal Message"),
    QT_TRANSLATE_NOOP("QtC::QmlProfiler", "Info Message"),
};

void DebugMessagesModelTest::testLabels()
{
    QVariantList labels = model.labels();
    for (int i = 0; i <= QtMsgType::QtInfoMsg; ++i) {
        QVariantMap element = labels[i].toMap();
        QCOMPARE(element[QLatin1String("description")].toString(), Tr::tr(messageTypes[i]));
        QCOMPARE(element[QLatin1String("id")].toInt(), i);
    }
}

void DebugMessagesModelTest::testDetails()
{
    for (int i = 0; i < 10; ++i) {
        QVariantMap details = model.details(i);
        QCOMPARE(details.value(QLatin1String("displayName")).toString(),
                 Tr::tr(messageTypes[i % (QtMsgType::QtInfoMsg + 1)]));
        QCOMPARE(details.value(Tr::tr("Timestamp")).toString(),
                 Timeline::formatTime(i));
        QCOMPARE(details.value(Tr::tr("Message")).toString(),
                 QString::fromLatin1("message %1").arg(i));
        QCOMPARE(details.value(Tr::tr("Location")).toString(),
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
