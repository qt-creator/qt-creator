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

// Contiguous sub-pixel events with varying heights, like the perf/cpu-usage
// tracks: relativeHeight() differs per event, so the renderer's per-row run
// coalescing has to pick one height for a run of several events.
class VaryingHeightModel : public TimelineModel
{
public:
    VaryingHeightModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        for (int i = 0; i < 2000; ++i)
            insert(i * 5, 5, i);
        computeNesting();
    }

    float relativeHeight(int index) const override
    {
        // Occasional tall events among mostly short ones.
        return (index % 17 == 0) ? 1.0f : 0.15f;
    }
};

// A single wide event occupying a fraction of its row height, like a bar in the
// memory/CPU usage tracks.
class ShortBarModel : public TimelineModel
{
public:
    ShortBarModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        insert(4000, 2000, 0); // x = 400..600, comfortably wider than a pixel
        computeNesting();
    }

    float relativeHeight(int) const override { return 0.2f; }
};

// Exposes the renderer's own fill geometry so a test can assert that whatever
// is drawn is also hit-testable.
class ProbePainter : public TrackPainter
{
public:
    QList<QRectF> fillRects(int trackIndex)
    {
        NeutralTrackGeometry geom;
        Track &t = const_cast<Track &>(tracks()[trackIndex]);
        ensureAttrCache(t);
        buildNeutralGeometry(t, geom);
        QList<QRectF> out;
        for (const ColorRects &cr : geom.fills)
            out += cr.rects;
        return out;
    }
};

// For every drawn fill rect, sample the pixels a user could click inside it and
// count those where the hit test finds nothing.
static int unhittableDrawnPixels(ProbePainter &painter, QString *report)
{
    const QList<QRectF> rects = painter.fillRects(0);
    int misses = 0;
    int samples = 0;
    for (const QRectF &r : rects) {
        for (int x = int(std::ceil(r.left())); x < r.right(); ++x) {
            for (int y = int(std::ceil(r.top())); y < r.bottom(); ++y) {
                ++samples;
                if (itemIndexAt(painter, QPoint(x, y)) < 0) {
                    ++misses;
                    if (misses <= 8) {
                        *report += QString("(%1,%2) in rect %3..%4 x %5..%6; ")
                                       .arg(x).arg(y)
                                       .arg(r.left()).arg(r.right())
                                       .arg(r.top()).arg(r.bottom());
                    }
                }
            }
        }
    }
    *report += QString("[%1 drawn rects, %2 sampled pixels]").arg(rects.size()).arg(samples);
    return misses;
}

class tst_TrackPainterInteraction : public QObject
{
    Q_OBJECT

private slots:
    void initialState();
    void selectionLockedHover();
    void indexAtWithData();
    void indexAtFarParent();
    void unlockedHover();
    void narrowEventTolerance();
    void narrowEventUnderCursorWins();
    void shortBarHitOverFullRow();
    void drawnPixelsAreHittable_data();
    void drawnPixelsAreHittable();
};

// A single event far narrower than one pixel. It is drawn as one pixel column,
// so requiring a pixel-exact click makes it practically unselectable.
class NarrowModel : public TimelineModel
{
public:
    NarrowModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        insert(5005, 1, 0); // 0.1px wide at the zoom level used below
        computeNesting();
    }
};

void tst_TrackPainterInteraction::narrowEventTolerance()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    NarrowModel model(&aggregator);
    model.loadData();
    painter.setTracks({&model});
    painter.setRange(0, 10000);
    painter.resize(1000, 30);

    // Drawn as the pixel column x=500.5..501.5, i.e. only x=501 is pixel-exact.
    QCOMPARE(itemIndexAt(painter, QPoint(501, 15)), 0);
    // One pixel of slack to either side keeps it reachable by mouse.
    QCOMPARE(itemIndexAt(painter, QPoint(500, 15)), 0);
    QCOMPARE(itemIndexAt(painter, QPoint(502, 15)), 0);
    // Beyond the tolerance nothing is selected, so clicking the background
    // still misses.
    QCOMPARE(itemIndexAt(painter, QPoint(498, 15)), -1);
    QCOMPARE(itemIndexAt(painter, QPoint(504, 15)), -1);
}

// Two narrow events in adjacent pixel columns. Each must select itself: the
// tolerance may only ever fill in where no event is under the cursor.
class TwoNarrowModel : public TimelineModel
{
public:
    TwoNarrowModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        insert(5000, 1, 0); // pixel column 500
        insert(5010, 1, 1); // pixel column 501
        computeNesting();
    }
};

void tst_TrackPainterInteraction::narrowEventUnderCursorWins()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    TwoNarrowModel model(&aggregator);
    model.loadData();
    painter.setTracks({&model});
    painter.setRange(0, 10000);
    painter.resize(1000, 30);

    QCOMPARE(itemIndexAt(painter, QPoint(500, 15)), 0);
    QCOMPARE(itemIndexAt(painter, QPoint(501, 15)), 1);
}

void tst_TrackPainterInteraction::shortBarHitOverFullRow()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    ShortBarModel model(&aggregator);
    model.loadData();
    painter.setTracks({&model});
    painter.setRange(0, 10000);
    painter.resize(1000, 30);

    const int rowH = model.rowHeight(0);
    QCOMPARE(model.relativeHeight(0), 0.2f);

    // The bar is drawn in the bottom fifth of the row, but the whole row column
    // selects it: aiming at a few pixels of bar height is not workable.
    QCOMPARE(itemIndexAt(painter, QPoint(500, rowH - 1)), 0); // on the bar
    QCOMPARE(itemIndexAt(painter, QPoint(500, 0)), 0);        // above it, same row
    QCOMPARE(itemIndexAt(painter, QPoint(500, rowH / 2)), 0);
    // Outside the event's time span nothing is hit, tolerance aside.
    QCOMPARE(itemIndexAt(painter, QPoint(300, rowH / 2)), -1);
}

void tst_TrackPainterInteraction::drawnPixelsAreHittable_data()
{
    QTest::addColumn<bool>("expanded");
    QTest::newRow("collapsed") << false;
    QTest::newRow("expanded") << true;
}

void tst_TrackPainterInteraction::drawnPixelsAreHittable()
{
    QFETCH(bool, expanded);

    ProbePainter painter;
    TimelineModelAggregator aggregator;
    VaryingHeightModel model(&aggregator);
    model.loadData();
    model.setExpanded(expanded);
    painter.setTracks({&model});
    painter.setRange(0, 10000);
    painter.resize(1000, 60);

    QString report;
    const int misses = unhittableDrawnPixels(painter, &report);
    if (misses != 0)
        qDebug().noquote() << misses << "unhittable drawn pixels:" << report;
    QCOMPARE(misses, 0);
}

void tst_TrackPainterInteraction::initialState()
{
    TrackPainter painter;
    QCOMPARE(painter.findChildren<QObject *>().size(), 0);
    painter.setSelectedItem(-1, -1);
    QCOMPARE(itemIndexAt(painter, QPoint(0, 0)), -1);
    painter.setSelectionLocked(true);
}

void tst_TrackPainterInteraction::selectionLockedHover()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setTracks({&model});
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
    painter.setTracks({&model});
    painter.setRange(0, 100);
    painter.resize(100, 30);

    int idx = itemIndexAt(painter, QPoint(5, 15));
    QVERIFY(idx >= 0);
    QCOMPARE(idx, 0);
}

void tst_TrackPainterInteraction::indexAtFarParent()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    NestedModel model(&aggregator);
    model.loadData();
    painter.setTracks({&model});
    painter.setRange(0, 1000);
    painter.resize(1000, 30);

    // x=610 falls in a gap between the children at 600..605 and 640..645, so
    // only the long parent (index 0) is drawn there. bestIndex() lands near the
    // children, far from index 0 in the index space.
    QCOMPARE(itemIndexAt(painter, QPoint(610, 15)), 0);

    // x=602 is covered by both the child at 600..605 and the parent; the
    // narrower child must win.
    QCOMPARE(itemIndexAt(painter, QPoint(602, 15)), 15);
}

void tst_TrackPainterInteraction::unlockedHover()
{
    TrackPainter painter;
    TimelineModelAggregator aggregator;
    DummyModel model(&aggregator);
    model.loadData();
    painter.setTracks({&model});
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
