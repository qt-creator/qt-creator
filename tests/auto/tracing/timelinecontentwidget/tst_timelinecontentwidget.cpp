// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/pausedrangesoverlay.h>
#include <tracing/selectionrangeoverlay.h>
#include <tracing/timelinecontentwidget.h>
#include <tracing/timelinedetailswidget.h>
#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/tracklabels.h>
#include <tracing/trackpainterbase.h>
#include <tracing/trackpainterraster.h>

#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QApplication>
#include <QMenu>
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
    TimelineDetailsWidget details;
    LocatedModel *model = nullptr;          // Owned by the aggregator.
    TimelineContentWidget *content = nullptr;
};

// Three categories of a QML trace, the middle one between the two the user wants
// to compare.
class CategoryTimeline
{
public:
    CategoryTimeline()
    {
        zoom.setTrace(0, traceEnd);
        zoom.setRange(0, traceEnd);
        content = new TimelineContentWidget(&aggregator, &zoom, &details);
        for (const char *name : {"Memory Usage", "Pixmap Cache", "Creating"}) {
            auto model = new LocatedModel(&aggregator, 1, 2);
            model->setDisplayName(QString::fromLatin1(name));
            aggregator.addModel(model);
            models.append(model);
        }
        content->resize(800, 300);
    }

    ~CategoryTimeline() { delete content; }

    static constexpr qint64 traceEnd = 1000;

    TrackPainterBase *tracks() const
    {
        return content->findChild<TrackPainterRaster *>();
    }
    TrackLabels *labels() const { return content->findChild<TrackLabels *>(); }

    QStringList trackNames() const
    {
        QStringList names;
        for (int i = 0; i < tracks()->trackCount(); ++i)
            names.append(tracks()->trackModel(i)->displayName());
        return names;
    }

    // The action of the menu the content pops up, by its text.
    QAction *menuAction(const QString &text) const
    {
        for (QMenu *menu : content->findChildren<QMenu *>()) {
            for (QAction *action : menu->actions()) {
                if (action->text() == text)
                    return action;
            }
        }
        return nullptr;
    }

    void closeMenus() const
    {
        qDeleteAll(content->findChildren<QMenu *>(Qt::FindDirectChildrenOnly));
    }

    TimelineModelAggregator aggregator;
    TimelineZoomControl zoom;
    TimelineDetailsWidget details;
    QList<TimelineModel *> models; // Owned by the aggregator.
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
    TimelineDetailsWidget details;
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
    void thePausedRangesLieBetweenTracksAndSelection();
    void aHiddenCategoryLeavesNoGap();
    void theLabelsSayHowManyAreHidden();
    void showingOnlyOneCategoryHidesTheOthers();
    void movingAcrossAHiddenCategoryKeepsItsPlace();
    void theTitleRowOffersToBeDragged();
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

// The paused bands cover the tracks, and the selection range covers both, over
// the whole viewport whatever size it takes.
void tst_TimelineContentWidget::thePausedRangesLieBetweenTracksAndSelection()
{
    SelectableTimeline timeline(1, 2);
    auto viewport = timeline.content->findChild<QScrollArea *>()->viewport();
    auto paused = viewport->findChild<PausedRangesOverlay *>(Qt::FindDirectChildrenOnly);
    auto selection = viewport->findChild<SelectionRangeOverlay *>(Qt::FindDirectChildrenOnly);
    QVERIFY(paused);
    QVERIFY(selection);
    QVERIFY(paused->testAttribute(Qt::WA_TransparentForMouseEvents));

    // The software painter, as initTestCase() selects it.
    auto tracks = viewport->findChild<TrackPainterRaster *>(Qt::FindDirectChildrenOnly);
    QVERIFY(tracks);
    const QObjectList children = viewport->children();
    QVERIFY(children.indexOf(tracks) < children.indexOf(paused));
    QVERIFY(children.indexOf(paused) < children.indexOf(selection));

    // Resize events only reach a widget that is shown.
    timeline.content->resize(400, 200);
    timeline.content->show();
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));
    const QSize before = viewport->size();
    timeline.content->resize(640, 300);
    QTRY_VERIFY(viewport->size() != before);
    QCOMPARE(paused->size(), viewport->size());
    QCOMPARE(selection->size(), viewport->size());
}

void tst_TimelineContentWidget::aHiddenCategoryLeavesNoGap()
{
    CategoryTimeline timeline;
    QCOMPARE(timeline.trackNames(),
             QStringList({"Memory Usage", "Pixmap Cache", "Creating"}));
    const int creatingWasAt = timeline.tracks()->trackYOffset(2);
    const int pixmapsAt = timeline.tracks()->trackYOffset(1);
    QSignalSpy hiddenCount(timeline.content, &TimelineContentWidget::hiddenTrackCountChanged);

    timeline.aggregator.setModelHidden(timeline.models.at(1)->modelId(), true);

    QCOMPARE(timeline.trackNames(), QStringList({"Memory Usage", "Creating"}));
    // What was below moves up into the place of what is hidden.
    QCOMPARE(timeline.tracks()->trackYOffset(1), pixmapsAt);
    QVERIFY(timeline.tracks()->trackYOffset(1) < creatingWasAt);
    QCOMPARE(hiddenCount.count(), 1);
    QCOMPARE(hiddenCount.at(0).at(0).toInt(), 1);
    QCOMPARE(timeline.content->hiddenTrackCount(), 1);

    timeline.aggregator.showAllModels();
    QCOMPARE(timeline.trackNames(),
             QStringList({"Memory Usage", "Pixmap Cache", "Creating"}));
    QCOMPARE(timeline.tracks()->trackYOffset(2), creatingWasAt);
    QCOMPARE(hiddenCount.count(), 2);
    QCOMPARE(timeline.content->hiddenTrackCount(), 0);
}

void tst_TimelineContentWidget::theLabelsSayHowManyAreHidden()
{
    CategoryTimeline timeline;
    timeline.content->show();
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));
    TrackLabels *labels = timeline.labels();
    QCOMPARE(labels->footerHeight(), 0);
    const int heightWithAll = labels->sizeHint().height();

    timeline.aggregator.setModelHidden(timeline.models.at(1)->modelId(), true);
    QCOMPARE(labels->hiddenTrackCount(), 1);
    QVERIFY(labels->footerHeight() > 0);
    // The note takes the place of a track, not one in addition to them.
    QVERIFY(labels->sizeHint().height() < heightWithAll);

    // It sits right below the last track, and clicking it offers the choice.
    QSignalSpy clicked(labels, &TrackLabels::hiddenTracksClicked);
    const int footerY = kTimeLineTopMargin + labels->sizeHint().height()
                        - labels->footerHeight() / 2;
    QTest::mouseClick(labels, Qt::LeftButton, {}, QPoint(labels->width() / 2, footerY));
    QCOMPARE(clicked.count(), 1);
    QAction *creating = timeline.menuAction("Creating");
    QAction *pixmaps = timeline.menuAction("Pixmap Cache");
    QVERIFY(creating);
    QVERIFY(pixmaps);
    QVERIFY(creating->isChecked());
    QVERIFY(!pixmaps->isChecked());

    pixmaps->setChecked(true);
    QCOMPARE(labels->hiddenTrackCount(), 0);
    QCOMPARE(labels->footerHeight(), 0);
}

void tst_TimelineContentWidget::showingOnlyOneCategoryHidesTheOthers()
{
    CategoryTimeline timeline;
    TrackLabels *labels = timeline.labels();

    emit labels->contextMenuRequested(2, QPoint());
    QAction *showOnly = timeline.menuAction("Show Only \"Creating\"");
    QVERIFY(showOnly);
    showOnly->trigger();
    QCOMPARE(timeline.trackNames(), QStringList({"Creating"}));
    QCOMPARE(labels->hiddenTrackCount(), 2);

    // And the one to compare it with, from the same menu.
    timeline.closeMenus();
    emit labels->contextMenuRequested(0, QPoint());
    QAction *memory = timeline.menuAction("Memory Usage");
    QVERIFY(memory);
    QVERIFY(!memory->isChecked());
    memory->setChecked(true);
    QCOMPARE(timeline.trackNames(), QStringList({"Memory Usage", "Creating"}));
    QCOMPARE(labels->hiddenTrackCount(), 1);
}

void tst_TimelineContentWidget::movingAcrossAHiddenCategoryKeepsItsPlace()
{
    CategoryTimeline timeline;
    timeline.aggregator.setModelHidden(timeline.models.at(1)->modelId(), true);

    emit timeline.labels()->contextMenuRequested(1, QPoint());
    QAction *moveUp = timeline.menuAction("Move Up");
    QVERIFY(moveUp);
    QVERIFY(moveUp->isEnabled());
    QVERIFY(!timeline.menuAction("Move Down")->isEnabled());
    moveUp->trigger();

    QCOMPARE(timeline.trackNames(), QStringList({"Creating", "Memory Usage"}));
    // The hidden one comes back where it was, after the track it followed.
    timeline.aggregator.showAllModels();
    QCOMPARE(timeline.trackNames(),
             QStringList({"Creating", "Memory Usage", "Pixmap Cache"}));

    // A hidden track above stays above: moving up goes past visible tracks only.
    timeline.closeMenus();
    timeline.aggregator.setModelHidden(timeline.models.at(2)->modelId(), true);
    emit timeline.labels()->contextMenuRequested(1, QPoint());
    timeline.menuAction("Move Up")->trigger();
    QCOMPARE(timeline.trackNames(), QStringList({"Pixmap Cache", "Memory Usage"}));
    timeline.aggregator.showAllModels();
    QCOMPARE(timeline.trackNames(),
             QStringList({"Creating", "Pixmap Cache", "Memory Usage"}));
}

void tst_TimelineContentWidget::theTitleRowOffersToBeDragged()
{
    CategoryTimeline timeline;
    timeline.content->show();
    QVERIFY(QTest::qWaitForWindowExposed(timeline.content));
    TrackLabels *labels = timeline.labels();
    const QPoint overTitle(kTimeLineLeftMargin / 2, kTimeLineTopMargin + 10);

    // Sent rather than QTest::mouseMove(), which moves the real cursor.
    QMouseEvent hover(QEvent::MouseMove, overTitle, labels->mapToGlobal(overTitle),
                      Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(labels, &hover);
    QCOMPARE(labels->cursor().shape(), Qt::OpenHandCursor);

    QSignalSpy menu(labels, &TrackLabels::contextMenuRequested);
    QTest::mousePress(labels, Qt::RightButton, {}, overTitle);
    QCOMPARE(menu.count(), 1);
    QCOMPARE(menu.at(0).at(0).toInt(), 0);
    QTest::mouseRelease(labels, Qt::RightButton, {}, overTitle);
}

QTEST_MAIN(tst_TimelineContentWidget)

#include "tst_timelinecontentwidget.moc"
