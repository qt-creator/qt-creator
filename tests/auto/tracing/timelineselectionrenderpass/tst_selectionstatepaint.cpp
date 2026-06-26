// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/trackpainter.h>

#include <QTest>

using namespace Timeline;

// TrackPainter renders all tracks in one widget; for these single-track tests
// the item index under a point is the item of track 0.
static int itemIndexAt(const TrackPainter &painter, QPoint pos)
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
            insert(i * 10, 10, i);
    }
};

class tst_SelectionStatePaint : public QObject
{
    Q_OBJECT

private slots:
    void initialState();
    void selectedItemGetter();
    void paintWithSelection();
    void selectionLockedDefaults();
};

void tst_SelectionStatePaint::initialState()
{
    TrackPainter painter;
    QCOMPARE(itemIndexAt(painter, QPoint(-1, -1)), -1);
}

void tst_SelectionStatePaint::selectedItemGetter()
{
    TrackPainter painter;
    painter.setSelectedItem(0, 5);
    painter.setSelectedItem(-1, -1);
}

void tst_SelectionStatePaint::paintWithSelection()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setTracks({&model});
    painter.setRange(0, 100);
    painter.setSelectedItem(0, 0);
    painter.repaint();
}

void tst_SelectionStatePaint::selectionLockedDefaults()
{
    TrackPainter painter;
    painter.setSelectionLocked(true);
    painter.setSelectionLocked(false);
}

QTEST_MAIN(tst_SelectionStatePaint)

#include "tst_selectionstatepaint.moc"
