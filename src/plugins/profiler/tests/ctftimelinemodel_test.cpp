// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctftimelinemodel_test.h"

#include <profiler/ctftimelinemodel.h>

#include <QTest>

namespace Profiler::Internal {

using json = nlohmann::json;

// A slice of what "cmake --profiling-format=google-trace" writes: the configure
// run around the commands it ran, each command naming the file and line it was
// called from among its arguments. Two calls of the same command share a name,
// and a Windows path brings a colon of its own.
static const char cmakeTrace[] = R"([
{"cat":"project","name":"configure","ph":"B","pid":42,"tid":42,"ts":1000},
{"cat":"script","name":"if","ph":"B","pid":42,"tid":42,"ts":1100,
 "args":{"functionArgs":"CMAKE_HOST_WIN32","location":"C:/proj/CMakeLists.txt:11"}},
{"ph":"E","pid":42,"tid":42,"ts":1200},
{"cat":"script","name":"if","ph":"B","pid":42,"tid":42,"ts":1300,
 "args":{"functionArgs":"DEFINED FOO","location":"C:/proj/cmake/helper.cmake:134"}},
{"ph":"E","pid":42,"tid":42,"ts":1400},
{"cat":"script","name":"message","ph":"B","pid":42,"tid":42,"ts":1500,
 "args":{"functionArgs":"STATUS hello"}},
{"ph":"E","pid":42,"tid":42,"ts":1600},
{"ph":"E","pid":42,"tid":42,"ts":1700}
])";

// The items are in the order their begin events appear above.
enum ItemIndex { ConfigureItem, FirstIfItem, SecondIfItem, MessageItem };

void CtfTimelineModelTest::initTestCase()
{
    for (const json &event : json::parse(cmakeTrace))
        m_manager.addEvent(event);
    m_manager.finalize();
    QVERIFY(m_manager.errorString().isEmpty());

    const QList<CtfTimelineModel *> threads = m_manager.getSortedThreads();
    QCOMPARE(threads.size(), 1);
    m_model = threads.constFirst();
    QCOMPARE(m_model->count(), 4);
}

void CtfTimelineModelTest::testCommandLocation()
{
    const Timeline::ItemLocation location = m_model->location(FirstIfItem);
    // The drive letter's colon is not the one before the line number.
    QCOMPARE(location.file, QString("C:/proj/CMakeLists.txt"));
    QCOMPARE(location.line, 11);
    QCOMPARE(location.column, 0);
}

void CtfTimelineModelTest::testOneTypeInSeveralPlaces()
{
    // Both events are calls of "if", so the type cannot tell them apart.
    QCOMPARE(m_model->typeId(SecondIfItem), m_model->typeId(FirstIfItem));
    QCOMPARE(m_model->location(SecondIfItem).file, QString("C:/proj/cmake/helper.cmake"));
    QCOMPARE(m_model->location(SecondIfItem).line, 134);
}

void CtfTimelineModelTest::testEventsWithoutLocation()
{
    // The configure run has no arguments at all, the message has arguments that
    // name no place. Neither has a source to go to.
    QVERIFY(m_model->location(ConfigureItem).file.isEmpty());
    QVERIFY(m_model->location(MessageItem).file.isEmpty());
}

} // namespace Profiler::Internal
