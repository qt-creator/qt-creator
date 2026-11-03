// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinewidget.h"

#include "timelinecontentwidget.h"
#include "timelinedetailswidget.h"
#include "timelinemodelaggregator.h"
#include "timelinemodel.h"
#include "timelineoverviewwidget.h"
#include "timelinezoomcontrol.h"
#include "trackpainterbase.h"
#include "tracingtr.h"

#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QIcon>
#include <QMenu>
#include <QSize>
#include <QSlider>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

namespace Timeline {

// The slider position maps to the visible fraction of the window by a cubic curve.
static constexpr int zoomSliderMinimum = 1;
static constexpr int zoomSliderMaximum = 10000;
static constexpr double zoomSliderExponent = 3.0;

class TimelineWidget::TimelineWidgetPrivate
{
public:
    TimelineModelAggregator *m_aggregator = nullptr;
    TimelineZoomControl *m_zoomControl = nullptr;
    TimelineContentWidget *m_content = nullptr;
    TimelineOverviewWidget *m_overview = nullptr;
    TimelineDetailsWidget *m_details = nullptr; // Owned by the perspective once docked,
                                             // or by whoever passed it in.

    QSlider *m_zoomSlider = nullptr;

    void updateZoomSlider()
    {
        const qint64 windowDuration = m_zoomControl->windowDuration();
        const qint64 rangeDuration = m_zoomControl->rangeDuration();
        if (windowDuration <= 0 || rangeDuration <= 0)
            return;
        const int value = qRound(std::pow(double(rangeDuration) / double(windowDuration),
                                          1.0 / zoomSliderExponent) * zoomSliderMaximum);
        QSignalBlocker blocker(m_zoomSlider);
        m_zoomSlider->setValue(qBound(zoomSliderMinimum, value, zoomSliderMaximum));
    }

    void applyZoomSlider(int value)
    {
        const qint64 windowDuration = m_zoomControl->windowDuration();
        if (windowDuration <= 0)
            return;
        const double factor = std::pow(double(value) / zoomSliderMaximum, zoomSliderExponent);
        const qint64 newRange = qMax(qint64(1), qRound64(factor * windowDuration));
        const qint64 center = (m_zoomControl->rangeStart() + m_zoomControl->rangeEnd()) / 2;
        const qint64 newStart = qMax(m_zoomControl->windowStart(), center - newRange / 2);
        m_zoomControl->setRange(newStart, newStart + newRange);
    }

    int modelIndex(int modelId) const
    {
        const QList<TimelineModel *> models = m_aggregator->models();
        for (int i = 0; i < models.size(); ++i) {
            TimelineModel *model = models.at(i);
            if (model->modelId() == modelId)
                return i;
        }
        return -1;
    }
};

TimelineWidget::TimelineWidget(TimelineModelAggregator *aggregator,
                               TimelineZoomControl *zoomControl,
                               TimelineDetailsWidget *details,
                               QWidget *parent)
    : QWidget(parent), d(new TimelineWidgetPrivate)
{
    d->m_aggregator = aggregator;
    d->m_zoomControl = zoomControl;
    setMinimumHeight(170);

    // ---- Range details (hosted in a dockable view; ownership transfers there) ----
    d->m_details = details ? details : new TimelineDetailsWidget(this);

    // ---- Content and overview ----
    d->m_content = new TimelineContentWidget(aggregator, zoomControl, d->m_details, this);
    d->m_overview = new TimelineOverviewWidget(aggregator, zoomControl, this);

    // ---- Toolbar (placed in the left-panel header slot of the content widget) ----
    auto toolbar = new QToolBar;
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setMovable(false);

    const QIcon prevIcon = Utils::Icons::PREV_TOOLBAR.icon();
    const QIcon nextIcon = Utils::Icons::NEXT_TOOLBAR.icon();
    const QIcon rangeSelIcon = Utils::Icon(
        {{":/tracing/images/rangeselection.png",
          Utils::Theme::IconsBaseColor}}).icon();
    const QIcon rangeSelActiveIcon = Utils::Icon(
        {{":/tracing/images/rangeselected.png",
          Utils::Theme::IconsBaseColor}}).icon();
    const QIcon lockIcon = Utils::Icon(
        {{":/tracing/images/selectionmode.png",
          Utils::Theme::IconsBaseColor}}).icon();

    auto prevAction = toolbar->addAction(prevIcon, tr("Jump to Previous Event"));
    auto nextAction = toolbar->addAction(nextIcon, tr("Jump to Next Event"));
    toolbar->addSeparator();
    auto rangeAction = toolbar->addAction(rangeSelIcon, tr("Select Range"));
    rangeAction->setCheckable(true);
    auto lockAction = toolbar->addAction(lockIcon, tr("View Event Information on Mouseover"));
    lockAction->setCheckable(true);
    toolbar->addSeparator();
    auto tracksButton = new QToolButton;
    tracksButton->setIcon(Utils::Icons::FILTER.icon());
    tracksButton->setPopupMode(QToolButton::InstantPopup);
    tracksButton->setProperty(Utils::StyleHelper::C_NO_ARROW, true);
    auto tracksMenu = new QMenu(tracksButton);
    tracksButton->setMenu(tracksMenu);
    toolbar->addWidget(tracksButton);
    if (canSwitchTrackBackends())
        toolbar->addSeparator();
    //: Short for "software", shown on a narrow toolbar button.
    auto softwareAction = toolbar->addAction(tr("SW"));
    softwareAction->setCheckable(true);
    softwareAction->setVisible(canSwitchTrackBackends());
    softwareAction->setToolTip(
        tr("Renders the timeline with the software (QPainter) backend instead of "
           "the hardware-accelerated (QCanvasPainter) one."));

    d->m_content->setLeftHeaderWidget(toolbar);

    // ---- Zoom slider row (below the track labels in left panel) ----
    auto zoomSliderRow = new Utils::StyledBar;
    zoomSliderRow->setProperty(Utils::StyleHelper::C_TOP_BORDER, true);
    d->m_zoomSlider = new QSlider(Qt::Horizontal);
    d->m_zoomSlider->setRange(zoomSliderMinimum, zoomSliderMaximum);
    d->m_zoomSlider->setValue(zoomSliderMaximum);
    using namespace Layouting;
    using Utils::StyleHelper::SpacingTokens::PaddingHXs;
    Row {
        Utils::QtDesignWidgets::IconDisplay { icon(Utils::Icons::ZOOMIN_TOOLBAR) },
        d->m_zoomSlider,
        Utils::QtDesignWidgets::IconDisplay { icon(Utils::Icons::ZOOMOUT_TOOLBAR) },
        customMargins(PaddingHXs, 0, PaddingHXs, 0),
    }.attachTo(zoomSliderRow);
    d->m_content->setLeftFooterWidget(zoomSliderRow);

    Column {
        Utils::QtDesignWidgets::SeparatedItems {
            Utils::separatorColor(Utils::Theme::FancyToolBarSeparatorColor),
            Utils::separatorInset(0),
            Column {
                d->m_content,
                d->m_overview,
                noMargin,
                spacing(Utils::QtcSeparatedItemsWidget::separatorLineWidth()),
            },
        },
        noMargin,
    }.attachTo(this);

    // ---- Toolbar connections ----
    connect(tracksMenu, &QMenu::aboutToShow, this, [this, tracksMenu] {
        tracksMenu->clear();
        d->m_content->populateTracksMenu(tracksMenu);
    });
    const auto updateTracksButton = [this, tracksButton] {
        const int hidden = d->m_content->hiddenTrackCount();
        tracksButton->setToolTip(hidden > 0
            ? Tr::tr("Show or Hide Categories (%1 hidden)").arg(hidden)
            : Tr::tr("Show or Hide Categories"));
    };
    connect(d->m_content, &TimelineContentWidget::hiddenTrackCountChanged,
            this, updateTracksButton);
    updateTracksButton();

    connect(prevAction, &QAction::triggered, d->m_content, &TimelineContentWidget::jumpToPrev);
    connect(nextAction, &QAction::triggered, d->m_content, &TimelineContentWidget::jumpToNext);

    connect(rangeAction, &QAction::toggled,
            d->m_content, &TimelineContentWidget::setSelectionRangeMode);
    connect(d->m_content, &TimelineContentWidget::selectionRangeModeChanged, this,
            [rangeAction, rangeSelIcon, rangeSelActiveIcon](bool active) {
        QSignalBlocker blocker(rangeAction);
        rangeAction->setChecked(active);
        rangeAction->setIcon(active ? rangeSelActiveIcon : rangeSelIcon);
    });

    // The toolbar lock button uses QML's convention: checked = hover mode active = NOT locked.
    connect(lockAction, &QAction::toggled, this, [this](bool checked) {
        d->m_content->setSelectionLocked(!checked);
    });
    connect(d->m_content, &TimelineContentWidget::selectionLockedChanged, this,
            [lockAction](bool locked) {
        QSignalBlocker blocker(lockAction);
        lockAction->setChecked(!locked);
    });

    // Rendering-backend toggle: checked = software (QPainter).
    softwareAction->setChecked(d->m_content->trackBackend() == TrackBackend::Software);
    connect(softwareAction, &QAction::toggled, d->m_content, [this](bool checked) {
        d->m_content->setTrackBackend(checked ? TrackBackend::Software : TrackBackend::Gpu);
    });
    connect(d->m_content, &TimelineContentWidget::trackBackendChanged, this,
            [softwareAction](TrackBackend backend) {
        QSignalBlocker blocker(softwareAction);
        softwareAction->setChecked(backend == TrackBackend::Software);
    });

    connect(d->m_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        d->applyZoomSlider(value);
    });

    connect(zoomControl, &TimelineZoomControl::rangeChanged, this, [this] {
        d->updateZoomSlider();
    });

    const auto updateToolbarEnabled = [=] {
        const bool hasTrace = zoomControl->traceDuration() > 0;
        prevAction->setEnabled(hasTrace);
        nextAction->setEnabled(hasTrace);
        zoomSliderRow->setEnabled(hasTrace);
        rangeAction->setEnabled(hasTrace);
        lockAction->setEnabled(hasTrace);
    };
    connect(zoomControl, &TimelineZoomControl::traceChanged, this, updateToolbarEnabled);
    updateToolbarEnabled();

    // Start in locked mode (details follow clicks, not hover) -- matches QML default.
    d->m_content->setSelectionLocked(true);

    // Propagate source location and type selection from content widget
    connect(aggregator, &TimelineModelAggregator::updateCursorPosition, this, [this] {
        if (!d->m_content->currentFile().isEmpty())
            emit gotoSourceLocation(d->m_content->currentFile(),
                                    d->m_content->currentLine(),
                                    d->m_content->currentColumn());
        emit typeSelected(d->m_content->currentTypeId());
    });
}

TimelineWidget::~TimelineWidget()
{
    delete d;
}

TimelineDetailsWidget *TimelineWidget::timelineDetailsWidget() const
{
    return d->m_details;
}

bool TimelineWidget::hasValidSelection() const
{
    return d->m_content->selectionRangeReady();
}

qint64 TimelineWidget::selectionStart() const
{
    return d->m_zoomControl->selectionStart();
}

qint64 TimelineWidget::selectionEnd() const
{
    return d->m_zoomControl->selectionEnd();
}

QString TimelineWidget::currentFile() const
{
    return d->m_content->currentFile();
}

int TimelineWidget::currentLine() const
{
    return d->m_content->currentLine();
}

int TimelineWidget::currentColumn() const
{
    return d->m_content->currentColumn();
}

int TimelineWidget::currentTypeId() const
{
    return d->m_content->currentTypeId();
}

void TimelineWidget::clear()
{
    d->m_content->clear();
}

void TimelineWidget::selectByTypeId(int typeId)
{
    d->m_content->selectByTypeId(typeId);
}

void TimelineWidget::selectByIndices(int modelIndex, int eventIndex)
{
    d->m_content->selectByAggregatorIndex(modelIndex, eventIndex);
}

} // namespace Timeline
