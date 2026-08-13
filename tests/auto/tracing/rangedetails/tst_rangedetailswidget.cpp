// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinecontentwidget.h>
#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/trackpainterbase.h>

#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QAbstractItemModel>
#include <QLabel>
#include <QTest>
#include <QTreeView>

using namespace Timeline;

class DummyTheme : public Utils::Theme
{
public:
    DummyTheme() : Utils::Theme(QLatin1String("dummy")) {}
};

static QList<QPair<QString, QString>> testContent()
{
    return {{QString("Duration"), QString("42")}};
}

static int rowCount(const RangeDetailsWidget &widget)
{
    const QTreeView *view = widget.findChild<QTreeView *>();
    return view && view->model() ? int(view->model()->rowCount()) : -1;
}

static QString title(const RangeDetailsWidget &widget)
{
    const QLabel *label = widget.findChild<QLabel *>();
    return label ? label->text() : QString();
}

// A one-item model that reports a settable title as the item's details and
// records the navigateToDetail() calls a details row triggers.
class DetailsModel : public TimelineModel
{
public:
    static constexpr qint64 itemStart = 0;
    static constexpr qint64 itemDuration = 10;

    DetailsModel(TimelineModelAggregator *aggregator, const QString &title)
        : TimelineModel(aggregator), m_title(title)
    {
        insert(itemStart, itemDuration, 1);
    }

    OrderedItemDetails orderedDetails(int index) const override
    {
        Q_UNUSED(index)
        OrderedItemDetails details;
        details.title = m_title;
        details.content.append({QString("Duration"), QString::number(itemDuration)});
        return details;
    }

    void navigateToDetail(int itemIndex, int detailRow) override
    {
        Q_UNUSED(itemIndex)
        Q_UNUSED(detailRow)
        ++navigations;
    }

    // The details of an unchanged selection changing, as a running trace does.
    // detailsChanged() is emitted on its own so no track rebuild interferes.
    void changeDetails(const QString &title)
    {
        m_title = title;
        emit detailsChanged();
    }

    int navigations = 0;

private:
    QString m_title;
};

// One timeline writing into a shared panel: what TimelineWidget wires up around
// a TimelineContentWidget, plus a single item to select.
class SharedPanelTimeline
{
public:
    static constexpr qint64 traceEnd = 100000;
    static constexpr qint64 rangeLength = 1000;

    SharedPanelTimeline(RangeDetailsWidget *details, const QString &title)
        : m_model(new DetailsModel(&m_aggregator, title))
    {
        m_zoom.setTrace(0, traceEnd);
        m_zoom.setRange(0, rangeLength);
        m_content = new TimelineContentWidget(&m_aggregator, &m_zoom, details);
        m_aggregator.addModel(m_model);
    }

    ~SharedPanelTimeline() { delete m_content; }

    // Fills the panel with the item's details, as clicking the item does.
    void selectItem() { m_content->selectItem(0, 0); }

    // Scrolls the item out of the visible range, so recentering on it moves the
    // range back and is observable.
    void scrollAway() { m_zoom.setRange(traceEnd - rangeLength, traceEnd); }
    bool isScrolledAway() const { return m_zoom.rangeStart() == traceEnd - rangeLength; }

    TimelineContentWidget *content() const { return m_content; }
    DetailsModel *model() const { return m_model; }

private:
    TimelineModelAggregator m_aggregator;
    TimelineZoomControl m_zoom;
    DetailsModel *m_model = nullptr; // Owned by the aggregator.
    TimelineContentWidget *m_content = nullptr;
};

class tst_RangeDetailsWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void clearFromForeignProviderKeepsContent();
    void clearFromOwnProviderDropsContent();
    void clearWithoutProviderDropsContent();
    void resetDropsForeignContent();
    void providerIsClearedWhenProviderDies();
    void recenterReachesOnlyTheShowingTimeline();
    void recenterReachesAVisibleTimeline();
    void doubleClickReachesOnlyTheShowingTimeline();
    void modelUpdateKeepsForeignContent();
};

void tst_RangeDetailsWidget::initTestCase()
{
    Utils::setCreatorTheme(new DummyTheme);
    // No widget is ever shown here, and the software track painter needs no RHI.
    setTrackBackendOverride(TrackBackend::Software);
}

void tst_RangeDetailsWidget::cleanupTestCase()
{
    // setCreatorTheme() deletes the previous theme.
    Utils::setCreatorTheme(nullptr);
}

void tst_RangeDetailsWidget::clearFromForeignProviderKeepsContent()
{
    QObject shower;
    QObject other;
    RangeDetailsWidget widget;

    widget.setData(&shower, QString("Range"), testContent());
    QCOMPARE(widget.provider(), &shower);
    QCOMPARE(rowCount(widget), int(testContent().size()));

    // The other view losing its selection must not take down what this one shows.
    widget.clear(&other);
    QCOMPARE(widget.provider(), &shower);
    QCOMPARE(rowCount(widget), int(testContent().size()));
    QCOMPARE(title(widget), QString("Range"));
}

void tst_RangeDetailsWidget::clearFromOwnProviderDropsContent()
{
    QObject shower;
    RangeDetailsWidget widget;

    widget.setData(&shower, QString("Range"), testContent());
    widget.clear(&shower);
    QVERIFY(widget.provider() == nullptr);
    QCOMPARE(rowCount(widget), 0);
    QVERIFY(title(widget) != QString("Range"));
}

void tst_RangeDetailsWidget::clearWithoutProviderDropsContent()
{
    RangeDetailsWidget widget;

    // Unclaimed content: no provider owns it, so any view may drop it.
    widget.setData(nullptr, QString("Range"), testContent());
    QCOMPARE(rowCount(widget), int(testContent().size()));

    QObject other;
    widget.clear(&other);
    QVERIFY(widget.provider() == nullptr);
    QCOMPARE(rowCount(widget), 0);
}

void tst_RangeDetailsWidget::resetDropsForeignContent()
{
    QObject shower;
    RangeDetailsWidget widget;

    widget.setData(&shower, QString("Range"), testContent());
    // The trace itself is going away, so the content goes whoever put it there.
    widget.reset();
    QVERIFY(widget.provider() == nullptr);
    QCOMPARE(rowCount(widget), 0);
}

void tst_RangeDetailsWidget::providerIsClearedWhenProviderDies()
{
    RangeDetailsWidget widget;
    {
        QObject shower;
        widget.setData(&shower, QString("Range"), testContent());
        QCOMPARE(widget.provider(), &shower);
    }
    QVERIFY(widget.provider() == nullptr);

    // With the provider gone nobody owns the content, so any view may drop it.
    QObject other;
    widget.clear(&other);
    QCOMPARE(rowCount(widget), 0);
}

void tst_RangeDetailsWidget::recenterReachesOnlyTheShowingTimeline()
{
    RangeDetailsWidget widget;
    SharedPanelTimeline first(&widget, QString("First"));
    SharedPanelTimeline second(&widget, QString("Second"));

    first.selectItem();
    QObject *firstView = first.content();
    QCOMPARE(widget.provider(), firstView);

    // The second timeline takes the panel over.
    second.selectItem();
    QObject *secondView = second.content();
    QCOMPARE(widget.provider(), secondView);

    first.scrollAway();
    second.scrollAway();
    emit widget.recenterOnItem();

    // Neither timeline is on screen, so the panel's content decides: only the
    // timeline whose selection it shows recenters; the other one must not scroll
    // back to a selection nobody is looking at.
    QVERIFY(first.isScrolledAway());
    QVERIFY(!second.isScrolledAway());
}

void tst_RangeDetailsWidget::recenterReachesAVisibleTimeline()
{
    RangeDetailsWidget widget;
    SharedPanelTimeline hidden(&widget, QString("Hidden"));
    SharedPanelTimeline shown(&widget, QString("Shown"));

    hidden.selectItem();
    shown.selectItem();
    shown.content()->show();

    // A flame graph row fills the panel while both timelines keep their selection,
    // as a statistics selection fanned out to several views leaves it.
    QObject flameGraph;
    widget.setData(&flameGraph, QString("Flame graph row"), testContent());

    hidden.scrollAway();
    shown.scrollAway();
    emit widget.recenterOnItem();

    // Recentering does not depend on the displayed rows, so the timeline the user
    // can see scrolls back to its selection whoever filled the panel. The one that
    // is off screen still stays put.
    QVERIFY(hidden.isScrolledAway());
    QVERIFY(!shown.isScrolledAway());
}

void tst_RangeDetailsWidget::doubleClickReachesOnlyTheShowingTimeline()
{
    RangeDetailsWidget widget;
    SharedPanelTimeline first(&widget, QString("First"));
    SharedPanelTimeline second(&widget, QString("Second"));

    first.selectItem();
    second.selectItem();

    emit widget.rowDoubleClicked(0);
    QCOMPARE(first.model()->navigations, 0);
    QCOMPARE(second.model()->navigations, 1);

    // A flame graph's rows belong to no timeline item, so no timeline navigates.
    QObject flameGraph;
    widget.setData(&flameGraph, QString("Flame graph"), testContent());
    emit widget.rowDoubleClicked(0);
    QCOMPARE(first.model()->navigations, 0);
    QCOMPARE(second.model()->navigations, 1);
}

void tst_RangeDetailsWidget::modelUpdateKeepsForeignContent()
{
    RangeDetailsWidget widget;
    SharedPanelTimeline timeline(&widget, QString("Timeline item"));

    timeline.selectItem();
    QCOMPARE(title(widget), QString("Timeline item"));

    // A flame graph row fills the panel while the timeline keeps its selection.
    QObject flameGraph;
    widget.setData(&flameGraph, QString("Flame graph row"), testContent());

    // The selected item's details changing is not a user selection, so it must
    // not pull the panel away from the flame graph.
    timeline.model()->changeDetails(QString("Updated"));
    QCOMPARE(widget.provider(), &flameGraph);
    QCOMPARE(title(widget), QString("Flame graph row"));

    // Once the timeline shows its own selection again, updates do arrive.
    timeline.selectItem();
    QCOMPARE(title(widget), QString("Updated"));
    timeline.model()->changeDetails(QString("Updated again"));
    QCOMPARE(title(widget), QString("Updated again"));
}

QTEST_MAIN(tst_RangeDetailsWidget)

#include "tst_rangedetailswidget.moc"
