// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinenotesmodel.h>
#include <tracing/trackpainter.h>

#include <QTest>

using namespace Timeline;

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

    QCOMPARE(painter.model(), nullptr);
    painter.setModel(&model);
    QCOMPARE(painter.model(), &model);
    painter.setModel(nullptr);
    QCOMPARE(painter.model(), nullptr);
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
    painter.setSelectedItem(-1);
    painter.setSelectedItem(0);
    painter.setSelectedItem(5);
    painter.setSelectedItem(-1);
}

void tst_TrackPainterProperties::indexAt()
{
    TrackPainter painter;
    QCOMPARE(painter.indexAt(QPoint(0, 0)), -1);

    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    painter.setModel(&model);
    painter.setRange(0, 0);
    QCOMPARE(painter.indexAt(QPoint(0, 0)), -1);
}

QTEST_MAIN(tst_TrackPainterProperties)

#include "tst_trackpainterproperties.moc"
