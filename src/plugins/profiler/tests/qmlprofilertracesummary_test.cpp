// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertracesummary_test.h"

#include <profiler/qmlprofilertracesummary.h>
#include <profiler/quick3dmodel.h>

#include <QJsonArray>
#include <QTest>

using namespace QmlDebug;
using namespace std::chrono;

namespace Profiler::Internal {

static QJsonObject stage(const QJsonArray &stages, const QString &name)
{
    for (const QJsonValue &value : stages) {
        if (value.toObject().value("stage").toString() == name)
            return value.toObject();
    }
    return {};
}

static QStringList eventNames(const QJsonObject &summary)
{
    QStringList names;
    const QJsonArray events = summary.value("events").toArray();
    for (const QJsonValue &event : events)
        names.append(event.toObject().value("details").toString());
    return names;
}

// The trace starts at 1000ns, so that relative and absolute times differ.
//   outer (JavaScript)  1100..1200, calling
//   inner (JavaScript)  1120..1150
//   width (Binding)     1300..1310, 1400..1420, 1500..1530
// Frames at 60, 30 and 15 fps on the GUI thread, plus one on the render thread
// that must not count. One scene graph polish-and-sync, one texture upload in
// which only binding and uploading took time, and two Quick3D render frames,
// one of which took no time.
void QmlProfilerTraceSummaryTest::initTestCase()
{
    manager.initialize();

    const int outer = manager.appendEventType(
        QmlEventType(UndefinedMessage, Javascript, -1, QmlEventLocation("main.qml", 10, 5),
                     "outer"));
    const int inner = manager.appendEventType(
        QmlEventType(UndefinedMessage, Javascript, -1, QmlEventLocation("main.qml", 20, 5),
                     "inner"));
    const int width = manager.appendEventType(
        QmlEventType(UndefinedMessage, Binding, -1, QmlEventLocation("main.qml", 30, 9),
                     "width"));
    const int frame = manager.appendEventType(
        QmlEventType(Event, UndefinedRangeType, AnimationFrame));
    const int polishAndSync = manager.appendEventType(
        QmlEventType(SceneGraphFrame, UndefinedRangeType, SceneGraphPolishAndSync));
    const int texturePrepare = manager.appendEventType(
        QmlEventType(SceneGraphFrame, UndefinedRangeType, SceneGraphTexturePrepare));
    const int renderFrame = manager.appendEventType(
        QmlEventType(Quick3DEvent, UndefinedRangeType, Quick3DModel::RenderFrame));

    const auto range = [this](int type, nanoseconds start, nanoseconds end) {
        QmlEvent event(start, type, {});
        event.setRangeStage(RangeStart);
        manager.appendEvent(QmlEvent(event));
        event.setRangeStage(RangeEnd);
        event.setTimestamp(end);
        manager.appendEvent(std::move(event));
    };
    const auto frameAt = [this, frame](nanoseconds timestamp, int fps, AnimationThread thread) {
        QmlEvent event;
        event.setTypeIndex(frame);
        event.setTimestamp(timestamp);
        event.setNumbers<int>({fps, 1, thread});
        manager.appendEvent(std::move(event));
    };

    QmlEvent outerStart(1100ns, outer, {});
    outerStart.setRangeStage(RangeStart);
    manager.appendEvent(std::move(outerStart));
    range(inner, 1120ns, 1150ns);
    QmlEvent outerEnd(1200ns, outer, {});
    outerEnd.setRangeStage(RangeEnd);
    manager.appendEvent(std::move(outerEnd));

    frameAt(1250ns, 60, GuiThread);
    range(width, 1300ns, 1310ns);
    frameAt(1350ns, 30, GuiThread);
    frameAt(1360ns, 5, RenderThread);
    range(width, 1400ns, 1420ns);
    frameAt(1450ns, 15, GuiThread);
    range(width, 1500ns, 1530ns);

    QmlEvent sync;
    sync.setTypeIndex(polishAndSync);
    sync.setTimestamp(1600ns);
    sync.setNumbers<qint64>({4, 3, 2, 1});
    manager.appendEvent(std::move(sync));

    QmlEvent texture;
    texture.setTypeIndex(texturePrepare);
    texture.setTimestamp(1650ns);
    texture.setNumbers<qint64>({5, 0, 0, 7, 0});
    manager.appendEvent(std::move(texture));

    QmlEvent render;
    render.setTypeIndex(renderFrame);
    render.setTimestamp(1700ns);
    render.setNumbers<qint64>({50, 0});
    manager.appendEvent(QmlEvent(render));
    render.setTimestamp(1800ns);
    render.setNumbers<qint64>({0, 0});
    manager.appendEvent(std::move(render));

    manager.decreaseTraceStart(1000ns);
    manager.increaseTraceEnd(2000ns);
    manager.finalize();
}

void QmlProfilerTraceSummaryTest::testEvents()
{
    const QJsonObject summary = qmlTraceSummary(manager, {});
    QVERIFY2(!summary.contains("error"), qPrintable(summary.value("error").toString()));
    QCOMPARE(summary.value("num_event_types").toInt(), 3);

    const QJsonArray events = summary.value("events").toArray();
    QCOMPARE(events.size(), 3);

    const QJsonObject outer = events.at(0).toObject();
    QCOMPARE(outer.value("details").toString(), "outer");
    QCOMPARE(outer.value("type").toString(), "javascript");
    QCOMPARE(outer.value("calls").toInteger(), 1);
    QCOMPARE(outer.value("total_ns").toInteger(), 100);
    QCOMPARE(outer.value("self_ns").toInteger(), 70);
    QCOMPARE(outer.value("file").toString(), "main.qml");
    QCOMPARE(outer.value("line").toInt(), 10);
    QCOMPARE(outer.value("column").toInt(), 5);
    // Top-level time is outer's 100ns and the bindings' 60ns.
    QCOMPARE(outer.value("total_percent").toDouble(), 62.5);

    const QJsonObject width = events.at(1).toObject();
    QCOMPARE(width.value("details").toString(), "width");
    QCOMPARE(width.value("type").toString(), "binding");
    QCOMPARE(width.value("calls").toInteger(), 3);
    QCOMPARE(width.value("total_ns").toInteger(), 60);
    QCOMPARE(width.value("self_ns").toInteger(), 60);
    QCOMPARE(width.value("average_ns").toInteger(), 20);
    QCOMPARE(width.value("median_ns").toInteger(), 20);
    QCOMPARE(width.value("min_ns").toInteger(), 10);
    QCOMPARE(width.value("max_ns").toInteger(), 30);

    const QJsonObject range = summary.value("range").toObject();
    QCOMPARE(range.value("start_ns").toInteger(), 0);
    QCOMPARE(range.value("end_ns").toInteger(), 1000);
}

void QmlProfilerTraceSummaryTest::testSortAndLimit_data()
{
    QTest::addColumn<int>("sortBy");
    QTest::addColumn<int>("limit");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("total") << int(QmlTraceSummaryOptions::TotalTime) << 20
                           << QStringList{"outer", "width", "inner"};
    QTest::newRow("calls") << int(QmlTraceSummaryOptions::Calls) << 1 << QStringList{"width"};
    QTest::newRow("max") << int(QmlTraceSummaryOptions::MaxTime) << 2
                         << QStringList{"outer", "inner"};
    QTest::newRow("median") << int(QmlTraceSummaryOptions::MedianTime) << 3
                            << QStringList{"outer", "inner", "width"};
    QTest::newRow("none") << int(QmlTraceSummaryOptions::TotalTime) << 0 << QStringList{};
}

void QmlProfilerTraceSummaryTest::testSortAndLimit()
{
    QFETCH(int, sortBy);
    QFETCH(int, limit);
    QFETCH(QStringList, expected);

    QmlTraceSummaryOptions options;
    options.sortBy = QmlTraceSummaryOptions::SortBy(sortBy);
    options.limit = limit;
    QCOMPARE(eventNames(qmlTraceSummary(manager, options)), expected);
}

void QmlProfilerTraceSummaryTest::testRange()
{
    QmlTraceSummaryOptions options;
    options.start = 250ns;
    options.end = 440ns;
    const QJsonObject summary = qmlTraceSummary(manager, options);

    QCOMPARE(eventNames(summary), QStringList{"width"});
    const QJsonObject width = summary.value("events").toArray().at(0).toObject();
    QCOMPARE(width.value("calls").toInteger(), 2);
    QCOMPARE(width.value("total_ns").toInteger(), 30);

    const QJsonObject range = summary.value("range").toObject();
    QCOMPARE(range.value("start_ns").toInteger(), 250);
    QCOMPARE(range.value("end_ns").toInteger(), 440);
    QCOMPARE(range.value("duration_ns").toInteger(), 190);

    QCOMPARE(summary.value("frames").toObject().value("count").toInteger(), 2);
}

void QmlProfilerTraceSummaryTest::testFrames()
{
    QmlTraceSummaryOptions options;
    options.targetFrameRate = 60;
    const QJsonObject frames = qmlTraceSummary(manager, options).value("frames").toObject();

    QCOMPARE(frames.value("count").toInteger(), 3);
    QCOMPARE(frames.value("target_fps").toInt(), 60);
    QCOMPARE(frames.value("median_fps").toInt(), 30);
    QCOMPARE(frames.value("on_target").toInteger(), 1);
    QCOMPARE(frames.value("near_target").toInteger(), 0);
    QCOMPARE(frames.value("failed").toInteger(), 2);
    QCOMPARE(frames.value("stuttering").toInteger(), 1);
    QCOMPARE(frames.value("frame_time_p50_ms").toDouble(), 1000.0 / 30);
    QCOMPARE(frames.value("frame_time_p99_ms").toDouble(), 1000.0 / 15);
    QCOMPARE(frames.value("slowest_frame_ms").toDouble(), 1000.0 / 15);
    QCOMPARE(frames.value("slowest_frame_timestamp_ns").toInteger(), 450);

    options.targetFrameRate = 15;
    const QJsonObject relaxed = qmlTraceSummary(manager, options).value("frames").toObject();
    QCOMPARE(relaxed.value("on_target").toInteger(), 3);
    QCOMPARE(relaxed.value("stuttering").toInteger(), 0);
}

void QmlProfilerTraceSummaryTest::testStages()
{
    const QJsonObject summary = qmlTraceSummary(manager, {});

    const QJsonArray sceneGraph = summary.value("scene_graph").toArray();
    QCOMPARE(sceneGraph.size(), 6);
    QCOMPARE(stage(sceneGraph, "polish").value("total_ns").toInteger(), 4);
    QCOMPARE(stage(sceneGraph, "wait").value("total_ns").toInteger(), 3);
    QCOMPARE(stage(sceneGraph, "gui_thread_sync").value("total_ns").toInteger(), 2);
    QCOMPARE(stage(sceneGraph, "animations").value("total_ns").toInteger(), 1);
    QCOMPARE(stage(sceneGraph, "polish").value("count").toInteger(), 1);
    QCOMPARE(stage(sceneGraph, "texture_bind").value("total_ns").toInteger(), 5);
    QCOMPARE(stage(sceneGraph, "texture_upload").value("total_ns").toInteger(), 7);
    // Stages that took no time did not run.
    QVERIFY(stage(sceneGraph, "texture_convert").isEmpty());
    QVERIFY(stage(sceneGraph, "texture_swizzle").isEmpty());
    QVERIFY(stage(sceneGraph, "texture_mipmap").isEmpty());

    const QJsonArray quick3D = summary.value("quick3d").toArray();
    QCOMPARE(quick3D.size(), 1);
    // Unlike the scene graph timeline, the Quick3D one shows events that took no time.
    QCOMPARE(stage(quick3D, "render_frame").value("count").toInteger(), 2);
    QCOMPARE(stage(quick3D, "render_frame").value("total_ns").toInteger(), 50);
    QCOMPARE(stage(quick3D, "render_frame").value("average_ns").toInteger(), 25);
    QCOMPARE(stage(quick3D, "render_frame").value("max_ns").toInteger(), 50);
}

} // namespace Profiler::Internal
