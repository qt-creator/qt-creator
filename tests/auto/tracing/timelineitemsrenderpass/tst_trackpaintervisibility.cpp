// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../trackpainterbackends.h"

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>

#include <QTest>

using namespace Timeline;
using namespace std::chrono;

// The track painter renders all tracks in one widget; for these single-track tests
// the item index under a point is the item of track 0.
static int itemIndexAt(const TrackPainterBase &painter, QPoint pos)
{
    int track = -1, item = -1;
    painter.itemAt(pos, &track, &item);
    return item;
}

class DummyModel : public TimelineModel
{
public:
    DummyModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        for (int i = 0; i < 10; ++i)
            insert(10ns * i, 10ns, i);
    }
};

class tst_TrackPainterVisibility : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase_data();
    void noModel();
    void emptyRange();
    void indexAtItem();
    void outOfRange();
};

void tst_TrackPainterVisibility::initTestCase_data()
{
    addTrackPainterBackends();
}

void tst_TrackPainterVisibility::noModel()
{
    withTrackPainter([]<typename TrackPainter>() {
        TrackPainter painter;
        painter.resize(100, 30);
        QCOMPARE(itemIndexAt(painter, QPoint(50, 15)), -1);
    });
}

void tst_TrackPainterVisibility::emptyRange()
{
    withTrackPainter([]<typename TrackPainter>() {
        TrackPainter painter;
        TimelineModelAggregator aggregator;
        DummyModel model(&aggregator);
        model.loadData();
        painter.setTracks({&model});
        painter.setRange(0ns, 0ns);
        painter.resize(100, 30);
        QCOMPARE(itemIndexAt(painter, QPoint(50, 15)), -1);
    });
}

void tst_TrackPainterVisibility::indexAtItem()
{
    withTrackPainter([]<typename TrackPainter>() {
        TrackPainter painter;
        TimelineModelAggregator aggregator;
        DummyModel model(&aggregator);
        model.loadData();
        painter.setTracks({&model});
        painter.setRange(0ns, 100ns);
        painter.resize(QSize(100, 30));

        int idx = itemIndexAt(painter, QPoint(50, 15));
        QVERIFY(idx >= 0);
        QVERIFY(idx < model.count());
    });
}

void tst_TrackPainterVisibility::outOfRange()
{
    withTrackPainter([]<typename TrackPainter>() {
        TrackPainter painter;
        TimelineModelAggregator aggregator;
        DummyModel model(&aggregator);
        model.loadData();
        painter.setTracks({&model});
        painter.setRange(200ns, 300ns);
        painter.resize(100, 30);
        QCOMPARE(itemIndexAt(painter, QPoint(50, 15)), -1);
    });
}

QTEST_MAIN(tst_TrackPainterVisibility)

#include "tst_trackpaintervisibility.moc"
