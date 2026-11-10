// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplerviewmanager_test.h"

#include <profiler/cpuflamegraphview.h>
#include <profiler/sampletrace.h>
#include <profiler/samplerviewmanager.h>

#include <tracing/timelinedetailswidget.h>
#include <tracing/timelinemodel.h>

#include <utils/temporarydirectory.h>

#include <QSignalSpy>
#include <QTest>

using namespace std::chrono_literals;

namespace Profiler::Internal {

using Details = QList<QPair<QString, QString>>;

static QString detail(const Details &details, const QString &name)
{
    for (const auto &[key, value] : details) {
        if (key == name)
            return value;
    }
    return {};
}

// start > main > work in three samples, start > main in one. `work` is called
// `third` instead when given.
static SampleTraceData sampleTrace(const QString &third = "work")
{
    SampleTraceData data;
    data.pid = 1;
    data.labels = {{"start", "/src/start.cpp", 3, "app", 0x10},
                   {"main", {}, 0, "libmain", 0x20},
                   third};
    data.samples = {
        {0us, 1, true, {0, 1, 2}},
        {100us, 1, true, {0, 1, 2}},
        {200us, 1, true, {0, 1, 2}},
        {300us, 1, true, {0, 1}},
    };
    return data;
}

// A sampler view manager with its CPU flame graph shown in a window of its own.
class FlameGraphFixture
{
public:
    FlameGraphFixture()
    {
        for (QWidget *view : manager.views(&parent)) {
            if (auto cpuFlameGraph = qobject_cast<CpuFlameGraphView *>(view))
                flameGraph = cpuFlameGraph;
        }
        if (!flameGraph)
            return;
        flameGraph->setParent(nullptr);
        flameGraph->resize(400, 300);
        flameGraph->show();
    }

    ~FlameGraphFixture() { delete flameGraph; }

    bool load(const SampleTraceData &data)
    {
        Utils::TemporaryDirectory dir("sampler-flamegraph");
        if (!writeSampleTrace(data, dir.path()))
            return false;
        QSignalSpy loaded(&manager, &SamplerViewManager::loadFinished);
        manager.load(dir.path());
        return loaded.wait();
    }

    // The canvas fills the middle of the widget, below the toolbar.
    QWidget *canvas() const
    {
        return flameGraph->childAt(flameGraph->width() / 2, flameGraph->height() / 2);
    }

    // The roots are drawn at the bottom and each callee above its caller,
    // as wide as its share of the caller's samples: work takes three of
    // main's four, on the left.
    QPoint startPos() const { return {canvas()->width() / 2, canvas()->height() - 1}; }

    QPoint workPos() const
    {
        // Click the left quarter from the top down until work's details show.
        QWidget *c = canvas();
        QSignalSpy shown(flameGraph, &Timeline::FlameGraphWidget::detailsChanged);
        for (int y = 0; y < c->height(); y += 4) {
            QTest::mouseClick(c, Qt::LeftButton, Qt::NoModifier, QPoint(c->width() / 4, y));
            if (!shown.isEmpty() && shown.last().at(0).toString() == "work")
                return {c->width() / 4, y};
        }
        return {};
    }

    Timeline::TimelineDetailsWidget details;
    SamplerViewManager manager{&details};
    QWidget parent;
    CpuFlameGraphView *flameGraph = nullptr;
};

void SamplerViewManagerTest::testFlameGraphShowsTheCallTree()
{
    FlameGraphFixture fixture;
    CpuFlameGraphView *flameGraph = fixture.flameGraph;
    QVERIFY(flameGraph);
    QCOMPARE(flameGraph->windowTitle(), QString("CPU Flame Graph"));
    QVERIFY(fixture.load(sampleTrace()));
    QVERIFY(QTest::qWaitForWindowExposed(flameGraph));

    QWidget *canvas = fixture.canvas();
    QVERIFY(canvas);
    QSignalSpy shown(flameGraph, &Timeline::FlameGraphWidget::detailsChanged);

    // Clicks up a column, bottom to top, and returns the functions it passed
    // through, each with its sample count.
    const auto column = [&](int x) {
        QStringList functions;
        for (int y = canvas->height() - 1; y >= 0; y -= 4) {
            const qsizetype before = shown.count();
            QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(x, y));
            if (shown.count() == before)
                continue;
            const QString function = shown.last().at(0).toString() + ' '
                                     + detail(shown.last().at(1).value<Details>(), "Samples");
            if (functions.isEmpty() || functions.last() != function)
                functions.append(function);
        }
        return functions;
    };

    QCOMPARE(column(canvas->width() / 4), QStringList({"start 4", "main 4", "work 3"}));
    QCOMPARE(column(canvas->width() * 7 / 8), QStringList({"start 4", "main 4"}));

    QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, fixture.startPos());
    QCOMPARE(shown.last().at(0).toString(), QString("start"));
    QCOMPARE(detail(shown.last().at(1).value<Details>(), "Binary"), QString("app"));
}

void SamplerViewManagerTest::testFlameGraphClickReportsTheFrameLocation()
{
    FlameGraphFixture fixture;
    QVERIFY(fixture.flameGraph);
    QVERIFY(fixture.load(sampleTrace()));
    QVERIFY(QTest::qWaitForWindowExposed(fixture.flameGraph));
    QWidget *canvas = fixture.canvas();
    QVERIFY(canvas);

    QSignalSpy goTo(&fixture.manager, &SamplerViewManager::gotoSourceLocation);

    // A frame with debug info reports its module and offset with the source.
    QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, fixture.startPos());
    QCOMPARE(goTo.count(), 1);
    QCOMPARE(goTo.last().at(0).toString(), QString("/src/start.cpp"));
    QCOMPARE(goTo.last().at(1).toInt(), 3);
    QCOMPARE(goTo.last().at(3).toString(), QString("app"));
    QCOMPARE(goTo.last().at(4).toULongLong(), quint64(0x10));

    // A frame without debug info still reports its module and offset. main
    // sits right above start.
    QSignalSpy shown(fixture.flameGraph, &Timeline::FlameGraphWidget::detailsChanged);
    const int x = fixture.startPos().x();
    for (int y = fixture.startPos().y(); y >= 0; y -= 4) {
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(x, y));
        if (!shown.isEmpty() && shown.last().at(0).toString() == "main")
            break;
    }
    QCOMPARE(shown.last().at(0).toString(), QString("main"));
    QCOMPARE(goTo.last().at(0).toString(), QString());
    QCOMPARE(goTo.last().at(3).toString(), QString("libmain"));
    QCOMPARE(goTo.last().at(4).toULongLong(), quint64(0x20));
}

void SamplerViewManagerTest::testLoadingAnotherTraceClearsTheFlameGraphSelection()
{
    FlameGraphFixture fixture;
    QVERIFY(fixture.flameGraph);
    QVERIFY(fixture.load(sampleTrace()));
    QVERIFY(QTest::qWaitForWindowExposed(fixture.flameGraph));

    // workPos() leaves work selected.
    const QPoint workPos = fixture.workPos();
    QVERIFY(!workPos.isNull());

    // In the next trace the same label id names another function, at the same
    // place. Clicking it must select it anew rather than find it selected
    // already.
    QVERIFY(fixture.load(sampleTrace("other")));
    QSignalSpy selected(fixture.flameGraph, &Timeline::FlameGraphWidget::typeSelected);
    QSignalSpy shown(fixture.flameGraph, &Timeline::FlameGraphWidget::detailsChanged);
    QTest::mouseClick(fixture.canvas(), Qt::LeftButton, Qt::NoModifier, workPos);
    QCOMPARE(shown.last().at(0).toString(), QString("other"));
    QCOMPARE(selected.count(), 1);
}

// A thread that was never seen with a stack has nothing to show in a lane of
// its own.
void SamplerViewManagerTest::testThreadsWithAStackGetALane()
{
    Timeline::TimelineDetailsWidget details;
    SamplerViewManager manager(&details);
    SampleTraceData data;
    data.pid = 1;
    data.labels = {"main", "work"};
    data.threadNames = {{1, "first"}, {2, "idle"}, {3, "third"}, {4, "late"}};
    data.samples = {
        {0us, 1, true, {0, 1}},
        {0us, 2, false, {}},
        {0us, 4, false, {}},
        {0us, 3, true, {0}},
        {100us, 1, true, {0}},
        {100us, 2, false, {}},
        {100us, 4, true, {1}},
        {100us, 3, true, {0}},
    };
    Utils::TemporaryDirectory dir("sampler-lanes");
    QVERIFY(writeSampleTrace(data, dir.path()));
    QSignalSpy loaded(&manager, &SamplerViewManager::loadFinished);
    manager.load(dir.path());
    QVERIFY(loaded.wait());

    QStringList names;
    for (const Timeline::TimelineModel *model : manager.timelineModels())
        names.append(model->displayName());
    QCOMPARE(names, QStringList({"CPU Usage", "Stacks of first (1)", "Stacks of third (3)",
                                 "Stacks of late (4)"}));

    // Each lane has the items of its own thread's samples, and only those.
    QStringList items;
    for (const Timeline::TimelineModel *model : manager.timelineModels().mid(1)) {
        for (int i = 0; i < model->count(); ++i) {
            items.append(QString("%1 %2 %3-%4")
                             .arg(model->displayName(), model->details(i).value("displayName"))
                             .arg(model->startTime(i).count())
                             .arg(model->endTime(i).count()));
        }
    }
    QCOMPARE(items, QStringList({"Stacks of first (1) main 0-200000",
                                 "Stacks of first (1) work 0-100000",
                                 "Stacks of third (3) main 0-200000",
                                 "Stacks of late (4) work 100000-200000"}));

    manager.clear();
    QVERIFY(manager.timelineModels().isEmpty());
}

} // namespace Profiler::Internal
