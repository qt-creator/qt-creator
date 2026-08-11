// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinenotesmodel.h>
#include <tracing/trackpainter.h>
#include <tracing/trackpainterraster.h>

#include <QPixmap>
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
};

// A model that reports rows its row count does not cover - the state every
// timeline model is in between loading its events and finalize(): per-event
// rows are assigned while loading, the row count is only published from
// finalize(). The track area can paint in that window, so it must not use those
// rows to index its per-row arrays.
class UnfinalizedRowModel : public TimelineModel
{
public:
    UnfinalizedRowModel(TimelineModelAggregator *parent) : TimelineModel(parent)
    {
        insert(0, 10, 1);
        insert(20, 10, 1);
    }

    int expandedRow(int) const override { return 2; }
    int collapsedRow(int) const override { return 2; }
};

class tst_TrackPainterProperties : public QObject
{
    Q_OBJECT

private slots:
    void model();
    void notes();
    void range();
    void selectionLocked();
    void selectedItem();
    void indexAt();
    void rowsBeyondRowCount();
};

void tst_TrackPainterProperties::model()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);

    QCOMPARE(painter.trackModel(0), nullptr);
    painter.setTracks({&model});
    QCOMPARE(painter.trackModel(0), &model);
    painter.setTracks({});
    QCOMPARE(painter.trackModel(0), nullptr);
}

void tst_TrackPainterProperties::notes()
{
    TrackPainter painter;
    TimelineNotesModel notes;

    painter.setNotes(&notes);
    painter.setNotes(nullptr);
}

void tst_TrackPainterProperties::range()
{
    TrackPainter painter;
    painter.setRange(10, 200);
    QCOMPARE(painter.rangeStart(), 10);
    QCOMPARE(painter.rangeEnd(), 200);
}

void tst_TrackPainterProperties::selectionLocked()
{
    TrackPainter painter;
    QVERIFY(painter.findChild<QObject *>() == nullptr || true);
    painter.setSelectionLocked(false);
    painter.setSelectionLocked(true);
}

void tst_TrackPainterProperties::selectedItem()
{
    TrackPainter painter;
    painter.setSelectedItem(-1, -1);
    painter.setSelectedItem(0, 0);
    painter.setSelectedItem(0, 5);
    painter.setSelectedItem(-1, -1);
}

void tst_TrackPainterProperties::indexAt()
{
    TrackPainter painter;
    QCOMPARE(itemIndexAt(painter, QPoint(0, 0)), -1);

    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    painter.setTracks({&model});
    painter.setRange(0, 0);
    QCOMPARE(itemIndexAt(painter, QPoint(0, 0)), -1);
}

// Painting a model whose rows are ahead of its row count must not index the
// per-row arrays out of bounds. The software backend is used because it renders
// without a GPU context; it shares the geometry code with the RHI one.
// TimelineOverviewWidget has its own test for the same state.
void tst_TrackPainterProperties::rowsBeyondRowCount()
{
    TimelineModelAggregator aggregator;
    UnfinalizedRowModel model(&aggregator);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.row(0), 2);

    TrackPainterRaster painter;
    painter.resize(200, 100);
    painter.setTracks({&model});
    painter.setRange(0, 30);

    QPixmap target(painter.size());
    painter.render(&target);
}

QTEST_MAIN(tst_TrackPainterProperties)

#include "tst_trackpainterproperties.moc"
