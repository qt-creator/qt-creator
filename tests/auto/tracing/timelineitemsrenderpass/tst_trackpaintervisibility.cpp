// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/trackpainter.h>

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

class tst_TrackPainterVisibility : public QObject
{
    Q_OBJECT

private slots:
    void noModel();
    void emptyRange();
    void indexAtItem();
    void outOfRange();
};

void tst_TrackPainterVisibility::noModel()
{
    TrackPainter painter;
    painter.resize(100, 30);
    QCOMPARE(painter.indexAt(QPoint(50, 15)), -1);
}

void tst_TrackPainterVisibility::emptyRange()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setModel(&model);
    painter.setRange(0, 0);
    painter.resize(100, 30);
    QCOMPARE(painter.indexAt(QPoint(50, 15)), -1);
}

void tst_TrackPainterVisibility::indexAtItem()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setModel(&model);
    painter.setRange(0, 100);
    painter.resize(QSize(100, 30));

    int idx = painter.indexAt(QPoint(50, 15));
    QVERIFY(idx >= 0);
    QVERIFY(idx < model.count());
}

void tst_TrackPainterVisibility::outOfRange()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setModel(&model);
    painter.setRange(200, 300);
    painter.resize(100, 30);
    QCOMPARE(painter.indexAt(QPoint(50, 15)), -1);
}

QTEST_MAIN(tst_TrackPainterVisibility)

#include "tst_trackpaintervisibility.moc"
