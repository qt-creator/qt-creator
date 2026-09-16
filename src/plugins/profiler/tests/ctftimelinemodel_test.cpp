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

// A Qt CTF trace as ctfloader.cpp hands it over: the threads of one process,
// which CTF names after the recorded session rather than by a pid, and each
// named by the packet context it was read from. A process that has no id of its
// own beyond that name states the name as its id.
static const char qtTrace[] = R"([
{"ph":"M","name":"thread_name","pid":"qtracedemo","tid":"0","ts":1000,
 "args":{"name":"Qt mainThread","displayId":"0"}},
{"ph":"M","name":"process_name","pid":"qtracedemo","tid":"0","ts":1000,
 "args":{"name":"qtracedemo","displayId":"qtracedemo"}},
{"ph":"M","name":"thread_name","pid":"qtracedemo","tid":"1","ts":1000,
 "args":{"name":"alpha","displayId":"1"}},
{"ph":"M","name":"process_name","pid":"qtracedemo","tid":"1","ts":1000,
 "args":{"name":"qtracedemo","displayId":"qtracedemo"}},
{"name":"work","ph":"B","pid":"qtracedemo","tid":"0","ts":1000},
{"ph":"E","pid":"qtracedemo","tid":"0","ts":2000},
{"name":"work","ph":"B","pid":"qtracedemo","tid":"1","ts":1500},
{"ph":"E","pid":"qtracedemo","tid":"1","ts":2500}
])";

// The same thread, recorded twice -- two traces loaded at once, whose lanes the
// loader qualifies per trace to keep them apart. The ids the traces themselves
// stated come with them, since the qualified ones are keys and no names: both
// recorded a thread 1, under one session, which is numbered for the reader.
static const char twoQtTraces[] = R"([
{"ph":"M","name":"thread_name","pid":"0/qtracedemo","tid":"0/1","ts":1000,
 "args":{"name":"alpha","displayId":"1"}},
{"ph":"M","name":"process_name","pid":"0/qtracedemo","tid":"0/1","ts":1000,
 "args":{"name":"qtracedemo #1","displayId":"qtracedemo #1"}},
{"ph":"M","name":"thread_name","pid":"1/qtracedemo","tid":"1/1","ts":1000,
 "args":{"name":"alpha","displayId":"1"}},
{"ph":"M","name":"process_name","pid":"1/qtracedemo","tid":"1/1","ts":1000,
 "args":{"name":"qtracedemo #2","displayId":"qtracedemo #2"}},
{"name":"work","ph":"B","pid":"0/qtracedemo","tid":"0/1","ts":1000},
{"ph":"E","pid":"0/qtracedemo","tid":"0/1","ts":2000},
{"name":"work","ph":"B","pid":"1/qtracedemo","tid":"1/1","ts":1500},
{"ph":"E","pid":"1/qtracedemo","tid":"1/1","ts":2500}
])";

// What `read` says about each lane of a trace of `events`, in the order the
// timeline puts the lanes in.
static QStringList lanes(const char *events, QString (Timeline::TimelineModel::*read)() const)
{
    Timeline::TimelineModelAggregator aggregator;
    CtfStatisticsModel statistics(nullptr);
    CtfTraceManager manager(nullptr, &aggregator, &statistics);
    for (const json &event : json::parse(events))
        manager.addEvent(event);
    manager.finalize();

    QStringList strings;
    for (const CtfTimelineModel *model : manager.getSortedThreads())
        strings.append((model->*read)());
    return strings;
}

static QStringList laneNames(const char *events)
{
    return lanes(events, &Timeline::TimelineModel::displayName);
}

static QStringList laneTooltips(const char *events)
{
    return lanes(events, &Timeline::TimelineModel::tooltip);
}

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

void CtfTimelineModelTest::testLanesOfOneProcessAreNamedByTheirThread()
{
    // Every lane of a Qt trace belongs to the one traced process, so naming it
    // in front of each of them says nothing about which lane this is -- and
    // what it would name is the recording session, not a process the reader
    // knows. The thread is what tells them apart.
    QCOMPARE(laneNames(qtTrace), (QStringList{"Qt mainThread (0)", "alpha (1)"}));

    // The tooltip names the process either way, and names it once: a process
    // called after the session it was recorded under has no id to add to that.
    QCOMPARE(laneTooltips(qtTrace),
             (QStringList{"Process: qtracedemo\nThread: Qt mainThread (0)",
                          "Process: qtracedemo\nThread: alpha (1)"}));

    // A trace that has a process to name still leads with it. Here the lane is
    // the process itself (tid == pid), as a cmake configure trace is.
    QCOMPARE(laneNames(cmakeTrace), QStringList{"Process 42"});
}

void CtfTimelineModelTest::testLanesOfSeveralProcessesNameTheirProcess()
{
    // Two recordings of one application, loaded at once: the threads are two
    // "alpha"s, and only the recording they were made in tells them apart. What
    // keeps their lanes apart internally -- a qualifier on every id -- is no
    // part of that: the reader is shown the ids the traces stated.
    QCOMPARE(laneNames(twoQtTraces),
             (QStringList{"qtracedemo #1 / alpha (1)", "qtracedemo #2 / alpha (1)"}));
}

void CtfTimelineModelTest::testEventsWithoutLocation()
{
    // The configure run has no arguments at all, the message has arguments that
    // name no place. Neither has a source to go to.
    QVERIFY(m_model->location(ConfigureItem).file.isEmpty());
    QVERIFY(m_model->location(MessageItem).file.isEmpty());
}

} // namespace Profiler::Internal
