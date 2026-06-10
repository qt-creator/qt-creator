// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelineformattime.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/timelinenotesmodel.h>
#include <tracing/timeruler.h>
#include <tracing/tracklabels.h>
#include <tracing/trackpainter.h>
#include <tracing/timelinecontentwidget.h>
#include <tracing/rangedetailswidget.h>

#include "../common/themeselector.h"

using namespace Timeline;

static const qint64 oneMs = 1000 * 1000; // in nanoseconds

class DummyModel : public Timeline::TimelineModel
{
public:
    DummyModel(TimelineModelAggregator *parent)
        : TimelineModel(parent)
    {
        setDisplayName("Dummy Category");
        setCollapsedRowCount(1);
        setExpandedRowCount(3);
        setTooltip("This is a tool tip"); // Tooltip on the title on the left
        setCategoryColor(Qt::yellow);
    }

    QRgb color(int index) const override
    {
        return QColor::fromHsl(index * 20, 96, 128).rgb(); // Red to green
    }

    // Called when a range gets selected, requests index of row which a range belongs to
    int expandedRow(int index) const override
    {
        return selectionId(index) % expandedRowCount(); // Distribute selections among rows
    }

    // Called when the category gets expanded
    RowLabels labels() const override
    {
        return {
            {QLatin1String("Dummy sub category 1"), 0},
            {QLatin1String("Dummy sub category 2"), 1},
        };
    }

    // Called when a range (or track) is selected
    ItemDetails details(int index) const override
    {
        Timeline::ItemDetails result;
        result.insert(QLatin1String("Foo A"), QString::fromLatin1("Bar %1a").arg(index));
        result.insert(QLatin1String("Foo B"), QString::fromLatin1("Bar %1b").arg(index));
        return result;
    }

    float relativeHeight(int index) const override
    {
        return 1.0 - (index / 10.0); // Shrink over time
    }

    void populateData()
    {
        // One region per selection
        insert(oneMs *  20, oneMs * 20, 0);
        insert(oneMs * 100, oneMs * 30, 1);

        // Two regions in the same selection
        insert(oneMs *  40, oneMs * 20, 2);
        insert(oneMs * 120, oneMs * 30, 2);

        // Three regions in the same selection
        insert(oneMs *  60, oneMs * 30, 3);
        insert(oneMs * 140, oneMs * 20, 3);
        insert(oneMs * 170, oneMs * 20, 3);

        emit contentChanged();
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ManualTest::ThemeSelector::setTheme(":/themes/flat.creatortheme");

    auto modelAggregator = new TimelineModelAggregator;
    auto model = new DummyModel(modelAggregator);
    model->populateData();
    modelAggregator->setModels({model});

    auto notes = new Timeline::TimelineNotesModel;
    notes->addTimelineModel(model);
    notes->add(model->modelId(), 0, "Note on item 0");
    notes->add(model->modelId(), 2, "Note on item 2");
    modelAggregator->setNotes(notes);

    auto zoomControl = new TimelineZoomControl;
    zoomControl->setTrace(0, oneMs * 1000);
    zoomControl->setRange(0, oneMs * 1000 / 3);

    // QPainter ruler widget
    auto rulerWindow = new QWidget;
    rulerWindow->setWindowTitle("TimeRuler (QPainter)");
    rulerWindow->resize(700, 60);

    auto rulerLayout = new QVBoxLayout(rulerWindow);
    rulerLayout->setContentsMargins(0, 0, 0, 0);
    rulerLayout->setSpacing(2);

    auto ruler = new Timeline::TimeRuler(rulerWindow);
    ruler->setRange(0, oneMs * 1000 / 3);
    rulerLayout->addWidget(ruler);

    auto rangeLabel = new QLabel("range: 0 .. 333333333 ns", rulerWindow);
    rangeLabel->setAlignment(Qt::AlignCenter);
    rulerLayout->addWidget(rangeLabel);

    QObject::connect(zoomControl, &Timeline::TimelineZoomControl::rangeChanged,
                     ruler, [ruler, rangeLabel](qint64 start, qint64 end) {
                         ruler->setRange(start, end);
                         rangeLabel->setText(QString("range: %1 .. %2 ns").arg(start).arg(end));
                     });

    rulerWindow->show();

    // TrackLabels sidebar widget
    auto labelsWindow = new QWidget;
    labelsWindow->setWindowTitle("TrackLabels (QPainter)");
    labelsWindow->resize(200, 300);

    auto labelsLayout = new QVBoxLayout(labelsWindow);
    labelsLayout->setContentsMargins(0, 0, 0, 0);

    auto trackLabels = new Timeline::TrackLabels(labelsWindow);
    labelsLayout->addWidget(trackLabels);
    labelsLayout->addStretch();

    {
        using namespace Timeline;
        const int defaultH = 30;

        TrackInfo collapsed;
        collapsed.name = "Dummy Category (collapsed)";
        collapsed.color = Qt::yellow;
        collapsed.expanded = false;
        collapsed.rowHeights = {defaultH};

        TrackInfo expanded;
        expanded.name = "Dummy Category (expanded)";
        expanded.color = Qt::cyan;
        expanded.expanded = true;
        expanded.rowLabels = {"Dummy sub category 1", "Dummy sub category 2"};
        expanded.rowHeights = {defaultH, defaultH, defaultH};

        TrackInfo another;
        another.name = "Another Track";
        another.color = QColor::fromHsl(200, 128, 128);
        another.expanded = false;
        another.rowHeights = {defaultH};

        trackLabels->setTracks({collapsed, expanded, another});
    }

    labelsWindow->show();

    // TrackPainter — single track event renderer
    auto painterWindow = new QWidget;
    painterWindow->setWindowTitle("TrackPainter (QPainter)");
    painterWindow->resize(700, 120);

    auto painterLayout = new QVBoxLayout(painterWindow);
    painterLayout->setContentsMargins(0, 0, 0, 0);

    auto trackPainter = new Timeline::TrackPainter(painterWindow);
    trackPainter->setModel(model);
    trackPainter->setNotes(notes);
    trackPainter->setRange(0, oneMs * 1000 / 3);
    painterLayout->addWidget(trackPainter);
    painterLayout->addStretch();

    QObject::connect(zoomControl, &Timeline::TimelineZoomControl::rangeChanged,
                     trackPainter, [trackPainter](qint64 start, qint64 end) {
                         trackPainter->setRange(start, end);
                     });

    QObject::connect(trackPainter, &Timeline::TrackPainter::itemHovered,
                     trackPainter, [trackPainter](int index) {
                         trackPainter->setHoveredItem(index);
                     });

    QObject::connect(trackPainter, &Timeline::TrackPainter::itemClicked,
                     trackPainter, [trackPainter](int index) {
                         trackPainter->setSelectedItem(index);
                     });

    painterWindow->show();

    // Composed TimelineContentWidget — full sidebar + ruler + track painters
    auto contentWindow = new QWidget;
    contentWindow->setWindowTitle("TimelineContentWidget (QPainter)");
    contentWindow->resize(900, 330);
    auto contentLayout = new QVBoxLayout(contentWindow);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    auto detailsWidget = new Timeline::RangeDetailsWidget(contentWindow);
    auto contentWidget = new Timeline::TimelineContentWidget(
        modelAggregator, zoomControl, detailsWidget, contentWindow);
    contentLayout->addWidget(contentWidget, 1);
    contentLayout->addWidget(detailsWidget);

    auto rangeBtn = new QPushButton("Selection Range", contentWindow);
    rangeBtn->setCheckable(true);
    contentLayout->addWidget(rangeBtn);
    QObject::connect(rangeBtn, &QPushButton::toggled, contentWidget,
                     &Timeline::TimelineContentWidget::setSelectionRangeMode);

    contentWindow->show();

    return app.exec();
}
