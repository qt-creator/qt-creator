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

class tst_TrackPainterInteraction : public QObject
{
    Q_OBJECT

private slots:
    void initialState();
    void selectionLockedHover();
    void indexAtWithData();
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
