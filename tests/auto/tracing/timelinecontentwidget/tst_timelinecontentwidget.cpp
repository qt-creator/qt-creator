// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/rangedetailswidget.h>
#include <tracing/selectionrangeoverlay.h>
#include <tracing/timelinecontentwidget.h>
#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/tracklabels.h>
#include <tracing/trackpainterbase.h>

#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QApplication>
#include <QNativeGestureEvent>
#include <QPointingDevice>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>

using namespace Timeline;

class DummyTheme : public Utils::Theme
{
public:
    DummyTheme() : Utils::Theme(QLatin1String("dummy")) {}
};

// Two items of one type, each naming a line of its own: what a Chrome Trace
// Format trace of a CMake configure holds. Its events are typed by command
// name, so every `if` in it shares a type while each has its own place.
class LocatedModel : public TimelineModel
{
public:
    static constexpr int sharedTypeId = 1;

    LocatedModel(TimelineModelAggregator *aggregator, int firstLine, int secondLine)
        : TimelineModel(aggregator)
        , m_firstLine(firstLine)
        , m_secondLine(secondLine)
    {
        insert(0, 10, sharedTypeId);
        insert(20, 10, sharedTypeId);
    }

    int typeId(int index) const override
    {
        Q_UNUSED(index)
        return sharedTypeId;
    }

    ItemLocation location(int index) const override
    {
        return {QString("CMakeLists.txt"), index == 0 ? m_firstLine : m_secondLine, 0};
    }

private:
    const int m_firstLine;
    const int m_secondLine;
};

// One timeline over such a model: what TimelineWidget wires up around a
// TimelineContentWidget, reduced to what selecting an item needs.
class SelectableTimeline
{
public:
    SelectableTimeline(int firstLine, int secondLine)
        : model(new LocatedModel(&aggregator, firstLine, secondLine))
    {
        zoom.setTrace(0, traceEnd);
        zoom.setRange(0, traceEnd);
        content = new TimelineContentWidget(&aggregator, &zoom, &details);
        aggregator.addModel(model);
    }

    ~SelectableTimeline() { delete content; }

    static constexpr qint64 traceEnd = 1000;

    TimelineModelAggregator aggregator;
    TimelineZoomControl zoom;
    RangeDetailsWidget details;
    LocatedModel *model = nullptr;          // Owned by the aggregator.
    TimelineContentWidget *content = nullptr;
};

// A timeline on screen with more tracks than fit, zoomed in far enough to pan
// both ways: what a trackpad scrolls and pinches.
class ScrollableTimeline
{
public:
    ScrollableTimeline()
    {
        zoom.setTrace(0, traceEnd);
        zoom.setRange(rangeStart, rangeEnd);
        content = new TimelineContentWidget(&aggregator, &zoom, &details);
        for (int i = 0; i < 20; ++i)
            aggregator.addModel(new LocatedModel(&aggregator, 1, 2));
        content->resize(800, 300);
        content->show();
    }

    ~ScrollableTimeline() { delete content; }

    static constexpr qint64 traceEnd = 1000000;
    static constexpr qint64 rangeStart = 250000;
    static constexpr qint64 rangeEnd = 750000;

    QScrollBar *verticalScrollBar() const
    {
        return content->findChild<QScrollArea *>()->verticalScrollBar();
    }
    QWidget *trackArea() const { return content->findChild<QScrollArea *>()->viewport(); }
    TrackLabels *labels() const { return content->findChild<TrackLabels *>(); }

    // The middle of the track area and of the labels, in content coordinates.
    QPoint trackAreaCenter() const
    {
        return trackArea()->mapTo(content, trackArea()->rect().center());
    }
    QPoint labelsCenter() const { return labels()->mapTo(content, labels()->rect().center()); }

    // The time the track area shows at pos, in content coordinates.
    double timeAt(QPoint pos) const
    {
        const double fraction = trackArea()->mapFrom(content, pos).x()
                                / double(trackArea()->width());
        return zoom.rangeStart() + fraction * zoom.rangeDuration();
    }

    // Two fingers moving over pos by pixelDelta, as macOS reports them: in
    // pixels, and in eighths of a degree at a quarter of a degree per pixel.
    void swipe(QPoint pos, QPoint pixelDelta, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        QWidget *target = content->childAt(pos);
        const QPoint local = target->mapFrom(content, pos);
        QWheelEvent event(local, target->mapToGlobal(local), pixelDelta, pixelDelta * 2,
                          Qt::NoButton, modifiers, Qt::NoScrollPhase, false);
        deliver(target, &event);
    }

    // Two fingers spreading over pos by a factor of 1 + value.
    void pinch(QPoint pos, qreal value)
    {
        QWidget *target = content->childAt(pos);
        const QPoint local = target->mapFrom(content, pos);
        QNativeGestureEvent event(Qt::ZoomNativeGesture, QPointingDevice::primaryPointingDevice(),
                                  2, local, pos, target->mapToGlobal(local), value, {});
        deliver(target, &event);
    }

    TimelineModelAggregator aggregator;
    TimelineZoomControl zoom;
    RangeDetailsWidget details;
    TimelineContentWidget *content = nullptr;

private:
    // To the widget under the pointer, and from there up to whichever takes it,
    // as the platform delivers input. sendEvent() would drop the spontaneous
    // flag, and QApplication only passes a spontaneous wheel event on to the
    // parents.
    static void deliver(QWidget *target, QEvent *event)
    {
        QSpontaneKeyEvent::setSpontaneous(event);
        qApp->notify(target, event);
    }
};

class tst_TimelineContentWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void newLocationOfTheSameTypeMovesTheCursor();
    void theSamePlaceAgainLeavesTheCursor();
    void aRequestedSelectionIsNotReportedBack();
    void aDiagonalSwipeScrollsBothWays();
    void aSwipeScrollsWhileSelectingARange();
    void aSwipeOverTheLabelsScrollsBothWays();
    void aPinchZoomsAroundTheFingers();
    void aPinchOverTheLabelsKeepsTheStart();
    void aPinchInABrowserZooms_data();
    void aPinchInABrowserZooms();
};

void tst_TimelineContentWidget::initTestCase()
{
    Utils::setCreatorTheme(new DummyTheme);
    // No widget is ever shown here, and the software track painter needs no RHI.
    setTrackBackendOverride(TrackBackend::Software);
}

void tst_TimelineContentWidget::cleanupTestCase()
{
    // setCreatorTheme() deletes the previous theme.
    Utils::setCreatorTheme(nullptr);
}

void tst_TimelineContentWidget::newLocationOfTheSameTypeMovesTheCursor()
{
    SelectableTimeline timeline(11, 134);
    QSignalSpy cursorMoved(&timeline.aggregator,
                           &TimelineModelAggregator::updateCursorPosition);

    timeline.content->selectItem(0, 0);
    QCOMPARE(cursorMoved.count(), 1);
    QCOMPARE(timeline.content->currentLine(), 11);

    // The type has not changed, the place has: the source to show is another one.
    timeline.content->selectItem(0, 1);
    QCOMPARE(cursorMoved.count(), 2);
    QCOMPARE(timeline.content->currentLine(), 134);
}

void tst_TimelineContentWidget::theSamePlaceAgainLeavesTheCursor()
{
    SelectableTimeline timeline(11, 11);
    QSignalSpy cursorMoved(&timeline.aggregator,
                           &TimelineModelAggregator::updateCursorPosition);

    timeline.content->selectItem(0, 0);
    QCOMPARE(cursorMoved.count(), 1);

    // Same type, same place: nothing to move the cursor to.
    timeline.content->selectItem(0, 1);
    QCOMPARE(cursorMoved.count(), 1);
}

void tst_TimelineContentWidget::aRequestedSelectionIsNotReportedBack()
{
    SelectableTimeline timeline(11, 134);
    timeline.content->selectItem(0, 0);

    QSignalSpy cursorMoved(&timeline.aggregator,
                           &TimelineModelAggregator::updateCursorPosition);
    // Whoever asks for a selection knows where it is; telling them would send
    // the answer back into the view that requested it.
    timeline.content->selectByAggregatorIndex(0, 1);
    QCOMPARE(cursorMoved.count(), 0);
    QCOMPARE(timeline.content->currentLine(), 134);
}

void tst_TimelineContentWidget::aDiagonalSwipeScrollsBothWays()
{
    ScrollableTimeline timeline;
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));
    QScrollBar *vbar = timeline.verticalScrollBar();
    QVERIFY(vbar->maximum() > 30);

    // Fingers moving up and to the left bring in what lies below and to the
    // right, as in any scroll area: later times, and further tracks.
    timeline.swipe(timeline.trackAreaCenter(), {-40, -30});

    const qint64 panned = qRound64(40.0 * (ScrollableTimeline::rangeEnd
                                           - ScrollableTimeline::rangeStart)
                                   / timeline.trackArea()->width());
    QCOMPARE(timeline.zoom.rangeStart(), ScrollableTimeline::rangeStart + panned);
    QCOMPARE(timeline.zoom.rangeEnd(), ScrollableTimeline::rangeEnd + panned);
    QCOMPARE(vbar->value(), 30);
}

void tst_TimelineContentWidget::aSwipeScrollsWhileSelectingARange()
{
    ScrollableTimeline timeline;
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));
    timeline.content->setSelectionRangeMode(true);
    // What the pointer is over now is the overlay the range is dragged on.
    QCOMPARE(timeline.content->childAt(timeline.trackAreaCenter()),
             timeline.content->findChild<SelectionRangeOverlay *>());

    timeline.swipe(timeline.trackAreaCenter(), {-40, -30});

    QVERIFY(timeline.zoom.rangeStart() > ScrollableTimeline::rangeStart);
    QCOMPARE(timeline.verticalScrollBar()->value(), 30);
}

void tst_TimelineContentWidget::aSwipeOverTheLabelsScrollsBothWays()
{
    ScrollableTimeline timeline;
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));
    const QPoint overLabels = timeline.labelsCenter();
    QCOMPARE(timeline.content->childAt(overLabels), timeline.labels());

    timeline.swipe(overLabels, {-40, -30});

    QVERIFY(timeline.zoom.rangeStart() > ScrollableTimeline::rangeStart);
    QCOMPARE(timeline.verticalScrollBar()->value(), 30);
}

void tst_TimelineContentWidget::aPinchZoomsAroundTheFingers()
{
    ScrollableTimeline timeline;
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));
    const QPoint fingers = timeline.trackArea()->mapTo(
        timeline.content, QPoint(timeline.trackArea()->width() / 4,
                                 timeline.trackArea()->height() / 2));
    const double timeAtFingers = timeline.timeAt(fingers);

    timeline.pinch(fingers, 0.25);

    // The fingers spread by a quarter: what lay between them now fills a
    // quarter more of the screen, and what was under them stays there.
    QCOMPARE(timeline.zoom.rangeDuration(),
             qRound64((ScrollableTimeline::rangeEnd - ScrollableTimeline::rangeStart) / 1.25));
    const double timeAtFingersNow = timeline.timeAt(fingers);
    QVERIFY2(qAbs(timeAtFingersNow - timeAtFingers) <= 1,
             qPrintable(QString("%1 moved to %2").arg(timeAtFingers).arg(timeAtFingersNow)));
}

void tst_TimelineContentWidget::aPinchOverTheLabelsKeepsTheStart()
{
    ScrollableTimeline timeline;
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));

    // The labels lie left of every point in time, so the one that stays put is
    // the first one shown.
    timeline.pinch(timeline.labelsCenter(), 0.25);

    QCOMPARE(timeline.zoom.rangeStart(), ScrollableTimeline::rangeStart);
    QCOMPARE(timeline.zoom.rangeDuration(),
             qRound64((ScrollableTimeline::rangeEnd - ScrollableTimeline::rangeStart) / 1.25));
}

void tst_TimelineContentWidget::aPinchInABrowserZooms_data()
{
    QTest::addColumn<Qt::KeyboardModifier>("modifier");

    // A browser reports a pinch as a wheel event with Ctrl held. On macOS, Qt
    // for WebAssembly passes Ctrl on as Qt::MetaModifier, as the Cocoa plugin
    // would, while elsewhere it stays Qt::ControlModifier.
    QTest::newRow("macOS") << Qt::MetaModifier;
    QTest::newRow("elsewhere") << Qt::ControlModifier;
}

void tst_TimelineContentWidget::aPinchInABrowserZooms()
{
    QFETCH(Qt::KeyboardModifier, modifier);
    ScrollableTimeline timeline;
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));

    // Fingers pinching together, which the browser reports as scrolling down.
    timeline.swipe(timeline.trackAreaCenter(), {0, -30}, modifier);

    QVERIFY2(timeline.zoom.rangeDuration()
                 > ScrollableTimeline::rangeEnd - ScrollableTimeline::rangeStart,
             qPrintable(QString::number(timeline.zoom.rangeDuration())));
    QCOMPARE(timeline.verticalScrollBar()->value(), 0);
}

QTEST_MAIN(tst_TimelineContentWidget)

#include "tst_timelinecontentwidget.moc"
