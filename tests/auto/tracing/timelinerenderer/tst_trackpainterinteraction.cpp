// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/trackpainter.h>

#include <QCoreApplication>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>

using namespace Timeline;

class DummyModel : public TimelineModel
{
public:
    DummyModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        for (int i = 0; i < 10; ++i)
            insert(i * 10, 10, i);
    }
};

// One long range covering many short children. The long range sorts to index 0
// while bestIndex() of a click in a gap between children lands far away in the
// index space, exercising the outward search in indexAt().
class NestedModel : public TimelineModel
{
public:
    NestedModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        insert(0, 1000, 0);
        for (int i = 1; i <= 20; ++i)
            insert(i * 40, 5, i);
        computeNesting();
    }
};

class tst_TrackPainterInteraction : public QObject
{
    Q_OBJECT

private slots:
    void initialState();
    void selectionLockedHover();
    void indexAtWithData();
    void indexAtFarParent();
    void unlockedHover();
};

void tst_TrackPainterInteraction::initialState()
{
    TrackPainter painter;
    QCOMPARE(painter.findChildren<QObject *>().size(), 0);
    painter.setSelectedItem(-1);
    QCOMPARE(painter.indexAt(QPoint(0, 0)), -1);
    painter.setSelectionLocked(true);
}

void tst_TrackPainterInteraction::selectionLockedHover()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setModel(&model);
    painter.setRange(0, 100);
    painter.resize(100, 30);
    painter.setSelectionLocked(true);

    QMouseEvent ev(QEvent::MouseMove, QPointF(5, 15), QPointF(5, 15),
                   Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&painter, &ev);
}

void tst_TrackPainterInteraction::indexAtWithData()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setModel(&model);
    painter.setRange(0, 100);
    painter.resize(100, 30);

    int idx = painter.indexAt(QPoint(5, 15));
    QVERIFY(idx >= 0);
    QCOMPARE(idx, 0);
}

void tst_TrackPainterInteraction::indexAtFarParent()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    NestedModel model(&aggregator);
    model.loadData();
    painter.setModel(&model);
    painter.setRange(0, 1000);
    painter.resize(1000, 30);

    // x=610 falls in a gap between the children at 600..605 and 640..645, so
    // only the long parent (index 0) is drawn there. bestIndex() lands near the
    // children, far from index 0 in the index space.
    QCOMPARE(painter.indexAt(QPoint(610, 15)), 0);

    // x=602 is covered by both the child at 600..605 and the parent; the
    // narrower child must win.
    QCOMPARE(painter.indexAt(QPoint(602, 15)), 15);
}

void tst_TrackPainterInteraction::unlockedHover()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setModel(&model);
    painter.setRange(0, 100);
    painter.resize(100, 30);
    painter.setSelectionLocked(false);

    QSignalSpy spy(&painter, &TrackPainter::itemHovered);

    QMouseEvent ev(QEvent::MouseMove, QPointF(5, 15), QPointF(5, 15),
                   Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&painter, &ev);
}

QTEST_MAIN(tst_TrackPainterInteraction)

#include "tst_trackpainterinteraction.moc"
