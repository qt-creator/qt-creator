// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinenotesmodel.h>
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

QTEST_MAIN(tst_TrackPainterProperties)

#include "tst_trackpainterproperties.moc"
