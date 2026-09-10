// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinecontentwidget.h>
#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/trackpainterbase.h>

#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QSignalSpy>
#include <QTest>

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

class tst_TimelineContentWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void newLocationOfTheSameTypeMovesTheCursor();
    void theSamePlaceAgainLeavesTheCursor();
    void aRequestedSelectionIsNotReportedBack();
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

QTEST_MAIN(tst_TimelineContentWidget)

#include "tst_timelinecontentwidget.moc"
