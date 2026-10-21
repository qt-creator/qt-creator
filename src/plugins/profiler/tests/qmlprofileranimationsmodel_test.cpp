// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofileranimationsmodel_test.h"
#include "../profilertr.h"
#include "../qmlprofilersettings.h"
#include <tracing/timelineformatdata.h>
#include <utils/stylehelper.h>

#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>

using namespace QmlDebug;
namespace Profiler::Internal {

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
    QCOMPARE(model.collapsedRowCount(), 2);
    QCOMPARE(model.expandedRowCount(), 3);
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(model.expandedRow(i), (i % 2) ? 2 : 1);
        QCOMPARE(model.collapsedRow(i), model.expandedRow(i) - 1);
    }
}

void QmlProfilerAnimationsModelTest::testTypeId()
{
    for (int i = 0; i < 10; ++i)
        QCOMPARE(model.typeId(i), 0);
}

void QmlProfilerAnimationsModelTest::testColor()
{
    // Hue should rise with frame rate; read it in Oklch, which doesn't wrap here like sRGB's does.
    double last = -1;
    for (int i = 0; i < 10; ++i) {
        const double hue = Utils::StyleHelper::oklch(model.color(i)).hue;
        QVERIFY(hue > last);
        last = hue;
    }
}

static void applyTargetFrameRate(int framesPerSecond)
{
    QmlProfilerSettings &settings = globalSettings();
    settings.targetFrameRate.setVolatileValue(framesPerSecond);
    settings.apply();
}

void QmlProfilerAnimationsModelTest::testColorFollowsTargetFrameRate()
{
    const int originalTarget = globalSettings().targetFrameRate();
    const auto restore = qScopeGuard([originalTarget] { applyTargetFrameRate(originalTarget); });
    applyTargetFrameRate(60);

    // frameRate(5) is 33 FPS, frameRate(9) is 61 FPS.
    const QRgb at33Fps = model.color(5);
    const QRgb at61Fps = model.color(9);
    QVERIFY(at33Fps != at61Fps);

    // Against a 30 FPS target both are on target, and coloured alike. The loaded trace is
    // re-coloured: the timeline repaints on contentChanged().
    QSignalSpy contentChanged(&model, &Timeline::TimelineModel::contentChanged);
    applyTargetFrameRate(30);
    QCOMPARE(contentChanged.count(), 1);
    QCOMPARE(model.color(5), at61Fps);
    QCOMPARE(model.color(9), at61Fps);

    // Against 240 FPS, even 61 FPS is far off.
    applyTargetFrameRate(240);
    QCOMPARE(contentChanged.count(), 2);
    QVERIFY(model.color(9) != at61Fps);
    QVERIFY(Utils::StyleHelper::oklch(model.color(9)).hue
            < Utils::StyleHelper::oklch(at61Fps).hue);
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

} // namespace Profiler::Internal
