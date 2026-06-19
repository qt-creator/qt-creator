// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugmessagesmodel_test.h"
#include "../profilertr.h"

#include <tracing/timelineformattime.h>

#include <QTest>

using namespace QmlDebug;
namespace Profiler::Internal {

DebugMessagesModelTest::DebugMessagesModelTest()
    : model(&manager, &aggregator)
{}

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
    Timeline::RowLabels labels = model.labels();
    for (int i = 0; i <= QtMsgType::QtInfoMsg; ++i) {
        QCOMPARE(labels[i].description, Tr::tr(messageTypes[i]));
        QCOMPARE(labels[i].id, i);
    }
}

void DebugMessagesModelTest::testDetails()
{
    for (int i = 0; i < 10; ++i) {
        Timeline::ItemDetails details = model.details(i);
        QCOMPARE(details.value(QLatin1String("displayName")),
                 Tr::tr(messageTypes[i % (QtMsgType::QtInfoMsg + 1)]));
        QCOMPARE(details.value(Tr::tr("Timestamp")), Timeline::formatTime(i));
        QCOMPARE(details.value(Tr::tr("Message")),
                 QString::fromLatin1("message %1").arg(i));
        QCOMPARE(details.value(Tr::tr("Location")),
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
    for (int i = 0; i < 10; ++i) {
        const Timeline::ItemLocation loc = model.location(i);
        QCOMPARE(loc.file, QLatin1String("somefile.js"));
        QCOMPARE(loc.line, i);
        QCOMPARE(loc.column, 10 - i);
    }
}

void DebugMessagesModelTest::cleanupTestCase()
{
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.expandedRowCount(), 1);
    QCOMPARE(model.collapsedRowCount(), 1);
}

} // namespace Profiler::Internal
