// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinecontentwidget.h"

#include "rangedetailswidget.h"
#include "selectionrangedetailswidget.h"
#include "selectionrangeoverlay.h"
#include "timelinemodel.h"
#include "timelinemodelaggregator.h"
#include "timelinenotesmodel.h"
#include "timelinescrollsync.h"
#include "timelinezoomcontrol.h"
#include "timeruler.h"
#include "tracklabels.h"
#include "trackpainter.h"
#include "trackpainterraster.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QSplitterHandle>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>

namespace Timeline {

// Off by default (QtWarningMsg); enable the overlay with
// QT_LOGGING_RULES="qtc.tracing.frametime.debug=true".
namespace {
Q_LOGGING_CATEGORY(frameTimeLog, "qtc.tracing.frametime", QtWarningMsg)
}

class DividerHandle : public QSplitterHandle
{
public:
    DividerHandle(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent) {}

protected:
    void paintEvent(QPaintEvent *) override
    {
        const auto *theme = Utils::creatorTheme();
        if (!theme)
            return;
        QPainter p(this);
        p.fillRect(rect(), theme->color(Utils::Theme::Timeline_DividerColor));
    }
};

class DividerSplitter : public QSplitter
{
public:
    explicit DividerSplitter(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QSplitter(orientation, parent) {}

protected:
    QSplitterHandle *createHandle() override
    {
        return new DividerHandle(orientation(), this);
    }
};

TimelineContentWidget::TimelineContentWidget(TimelineModelAggregator *aggregator,
                                             TimelineZoomControl *zoom,
                                             RangeDetailsWidget *details,
                                             QWidget *parent)
    : QWidget(parent)
    , m_aggregator(aggregator)
    , m_zoom(zoom)
    , m_sync(new TimelineScrollSync(zoom, this))
    , m_details(details)
{
    m_ruler = new TimeRuler;

    m_labels = new TrackLabels;

    // The track container is an empty spacer: it is sized to the full content
    // height so the scroll area provides a correctly-ranged vertical scroll bar.
    // The actual rendering happens in m_tracksView, a single viewport-sized
    // widget that paints all tracks with an internal scroll offset.
    m_trackContainer = new QWidget;

    m_scrollArea = new QScrollArea;
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidget(m_trackContainer);

    activateTrackView(resolvedTrackBackend());

    m_overlay = new SelectionRangeOverlay(zoom, m_scrollArea->viewport());
    m_overlay->resize(m_scrollArea->viewport()->size());
    m_overlay->raise();
    m_scrollArea->viewport()->installEventFilter(this);

    // Frame-time overlay (top-right of the track area): shows the worst
    // single-frame time over the last sample window. Off by default; enable with
    // QT_LOGGING_RULES="qtc.tracing.frametime.debug=true".
    if (frameTimeLog().isDebugEnabled()) {
        using namespace Utils::StyleHelper;
        m_frameTimeLabel = new QLabel(m_scrollArea->viewport());
        m_frameTimeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_frameTimeLabel->setAutoFillBackground(true);
        m_frameTimeLabel->setContentsMargins(SpacingTokens::PaddingVXs, SpacingTokens::PaddingVXs,
                                             SpacingTokens::PaddingVXs, SpacingTokens::PaddingVXs);
        m_frameTimeLabel->setFont(uiFont(UiElementCaptionStrong));
        QPalette pal = m_frameTimeLabel->palette();
        pal.setColor(QPalette::WindowText, Utils::creatorColor(Utils::Theme::Token_Text_Accent));
        const QColor bg = Utils::creatorColor(Utils::Theme::Token_Basic_Black);
        pal.setColor(QPalette::Window, bg);
        m_frameTimeLabel->setPalette(pal);
        m_frameTimeLabel->setText("-- ms");
        m_frameTimeLabel->adjustSize();
        m_frameTimeLabel->raise();
        auto frameTimeTimer = new QTimer(this);
        frameTimeTimer->setInterval(500);
        connect(frameTimeTimer, &QTimer::timeout, this, &TimelineContentWidget::updateFrameTime);
        frameTimeTimer->start();
    }

    connect(m_details, &RangeDetailsWidget::recenterOnItem, this, [this] {
        recenterOnItem(m_selectedModelIndex, m_selectedItemIndex);
    });

    connect(m_details, &RangeDetailsWidget::rowDoubleClicked, this, [this](int row) {
        if (m_selectedModelIndex >= 0 && m_selectedModelIndex < m_trackModels.size()
                && m_selectedItemIndex >= 0) {
            m_trackModels[m_selectedModelIndex]->navigateToDetail(
                m_selectedItemIndex, row);
        }
    });

    m_selectionDetails = new SelectionRangeDetailsWidget(this);
    m_selectionDetails->move(200, 50);
    connect(zoom, &TimelineZoomControl::selectionChanged, this,
            [this](qint64 start, qint64 end) {
        if (!m_overlay->isActive()) {
            m_selectionDetails->hide();
            return;
        }
        m_selectionDetails->setData(start, end, m_zoom->rangeDuration(),
                                        m_zoom->selectionDuration() > 0);
    });
    connect(m_selectionDetails, &SelectionRangeDetailsWidget::closeRequested,
            this, [this] { setSelectionRangeMode(false); });
    connect(m_selectionDetails, &SelectionRangeDetailsWidget::recenterRequested,
            this, [this] {
        // Match QML MainView onRecenter: only recenter when exactly one endpoint
        // is outside the current range (XOR), centering on the selection midpoint.
        const bool startOut = m_zoom->selectionStart() < m_zoom->rangeStart();
        const bool endOut = m_zoom->selectionEnd() > m_zoom->rangeEnd();
        if (startOut != endOut) {
            const qint64 center = (m_zoom->selectionStart() + m_zoom->selectionEnd()) / 2;
            const qint64 halfDur = qMax(m_zoom->selectionDuration(),
                                        m_zoom->rangeDuration()) / 2;
            m_zoom->setRange(center - halfDur, center + halfDur);
        }
    });
    connect(m_overlay, &SelectionRangeOverlay::rangeDoubleClicked,
            this, [this] { setSelectionRangeMode(false); });
    connect(m_overlay, &SelectionRangeOverlay::passthruClick, this,
            [this](const QPoint &viewportPos) {
        m_selectionDetails->hide();
        // viewportPos is in viewport coordinates, which match the render widget.
        int track = -1, item = -1;
        m_tracksView->itemAt(viewportPos, &track, &item);
        if (track >= 0)
            selectItem(track, item);
    });

    m_sync->registerLabels(m_labels);
    m_sync->setVerticalScrollBar(m_scrollArea->verticalScrollBar());
    m_sync->registerRuler(m_ruler);

    // Left panel: header placeholder matching ruler height, then labels below
    const int rulerH = m_ruler->sizeHint().height();
    m_leftPanel = new QWidget;
    m_leftPanel->setMinimumWidth(m_labels->sizeHint().width());
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    m_leftLayout->setSpacing(0);
    m_leftHeader = new QWidget;
    m_leftHeader->setFixedHeight(rulerH);
    m_leftLayout->addWidget(m_leftHeader);
    m_leftLayout->addWidget(m_labels);

    // Right panel: ruler at top, scroll area below
    auto rightPanel = new QWidget;
    auto rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(m_ruler);
    rightLayout->addWidget(m_scrollArea, 1);

    auto splitter = new DividerSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->addWidget(m_leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(splitter);

    connect(m_labels, &TrackLabels::expandToggled, this, [this](int trackIndex) {
        if (trackIndex < 0 || trackIndex >= m_trackModels.size())
            return;
        const int modelId = m_trackModels[trackIndex]->modelId();
        for (TimelineModel *model : m_aggregator->models()) {
            if (model->modelId() == modelId) {
                model->setExpanded(!model->expanded());
                break;
            }
        }
    });

    connect(m_labels, &TrackLabels::moveCategories, this, [this](int from, int to) {
        m_aggregator->moveModel(painterToAggregator(from), painterToAggregator(to));
    });

    connect(m_labels, &TrackLabels::rowLabelClicked,
            this, [this](int trackIndex, int rowIndex, bool reverse) {
        if (trackIndex < 0 || trackIndex >= m_trackModels.size())
            return;
        const int modelId = m_trackModels[trackIndex]->modelId();
        for (TimelineModel *model : m_aggregator->models()) {
            if (model->modelId() != modelId)
                continue;
            if (model->hasMixedTypesInExpandedState())
                return;
            const RowLabels labels = model->labels();
            if (rowIndex < 0 || rowIndex >= labels.size())
                return;
            const int selId = labels.at(rowIndex).id;
            const qint64 time = m_zoom->rangeStart();
            const int curItem = (trackIndex == m_selectedModelIndex) ? m_selectedItemIndex : -1;
            const int item = reverse
                ? model->prevItemBySelectionId(selId, time, curItem)
                : model->nextItemBySelectionId(selId, time, curItem);
            if (item >= 0)
                selectItem(trackIndex, item);
            return;
        }
    });

    connect(m_labels, &TrackLabels::rowHeightChangeRequested,
            this, [this](int trackIndex, int rowIndex, int newHeight) {
        if (trackIndex < 0 || trackIndex >= m_trackModels.size())
            return;
        const int modelId = m_trackModels[trackIndex]->modelId();
        for (TimelineModel *model : m_aggregator->models()) {
            if (model->modelId() == modelId) {
                model->setExpandedRowHeight(rowIndex + 1, newHeight);
                m_tracksView->refreshGeometry();
                updateContainerSize();
                return;
            }
        }
    });

    connect(m_labels, &TrackLabels::noteClicked, this, [this](int trackIndex, int eventId) {
        selectItem(trackIndex, eventId);
    });

    connect(aggregator, &TimelineModelAggregator::modelsChanged,
            this, &TimelineContentWidget::rebuildTracks);
    connect(aggregator, &TimelineModelAggregator::notesChanged,
            this, &TimelineContentWidget::updateNotes);
    connect(m_ruler, &TimeRuler::markersChanged, this, [this](const QList<qint64> &markers) {
        m_markers = markers;
        m_tracksView->setMarkers(markers);
    });

    rebuildTracks();
}

void TimelineContentWidget::setLeftHeaderWidget(QWidget *widget)
{
    widget->setParent(m_leftPanel);
    m_leftLayout->replaceWidget(m_leftHeader, widget);
    delete m_leftHeader;
    m_leftHeader = nullptr;
    m_leftPanel->setMinimumWidth(qMax(m_leftPanel->minimumWidth(), widget->sizeHint().width()));
}

void TimelineContentWidget::addLeftPanelWidget(QWidget *widget)
{
    widget->setParent(m_leftPanel);
    m_leftLayout->insertWidget(m_leftLayout->indexOf(m_labels), widget);
}

QSize TimelineContentWidget::sizeHint() const
{
    return QSize(700, 300);
}

void TimelineContentWidget::setSelectionRangeMode(bool active)
{
    if (m_overlay->isActive() == active)
        return;
    m_overlay->reset();
    m_overlay->setActive(active);
    if (!active)
        m_selectionDetails->hide();
    emit selectionRangeModeChanged(active);
}

bool TimelineContentWidget::selectionRangeMode() const
{
    return m_overlay->isActive();
}

bool TimelineContentWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_scrollArea->viewport() && event->type() == QEvent::Resize) {
        auto *re = static_cast<QResizeEvent *>(event);
        m_tracksWidget->resize(re->size());
        m_overlay->resize(re->size());
        updateContainerSize();
        positionFrameTimeLabel();
    }
    return false;
}

void TimelineContentWidget::onFramePainted(std::chrono::nanoseconds renderTime)
{
    // A single widget renders all tracks, so its paint duration is the full-frame
    // render time. (The accumulate + queued flush mirrors the per-track build so
    // both architectures report the same metric.)
    m_frameAccum += renderTime;
    if (!m_frameFlushScheduled) {
        m_frameFlushScheduled = true;
        QMetaObject::invokeMethod(this, [this] {
            m_maxRender = std::max(m_maxRender, m_frameAccum);
            m_frameAccum = {};
            m_frameFlushScheduled = false;
        }, Qt::QueuedConnection);
    }
}

void TimelineContentWidget::positionFrameTimeLabel()
{
    if (m_frameTimeLabel)
        m_frameTimeLabel->move(m_scrollArea->viewport()->width() - m_frameTimeLabel->width() - 4, 4);
}

void TimelineContentWidget::updateFrameTime()
{
    // Worst full-frame render time observed in the last sample window.
    const double ms = std::chrono::duration<double, std::milli>(m_maxRender).count();
    m_frameTimeLabel->setText(QString::number(ms, 'f', 1) + " ms");
    m_maxRender = {};
    m_frameTimeLabel->adjustSize();
    positionFrameTimeLabel();
    m_frameTimeLabel->raise();
}

void TimelineContentWidget::updateContainerSize()
{
    // Keep the spacer container as wide as the viewport (no horizontal scroll)
    // and as tall as the full track content so the vertical scroll bar covers
    // the whole timeline. The render widget itself stays viewport-sized.
    const int viewW = m_scrollArea->viewport()->width();
    const int contentH = m_tracksView->totalHeight();
    m_trackContainer->resize(viewW, contentH);
}

static void fitFloatingWidget(QWidget *w, int parentW, int parentH)
{
    if (!w->isVisible() || w->width() > parentW || w->height() > parentH)
        return;
    w->move(qBound(0, w->x(), parentW - w->width()),
            qBound(0, w->y(), parentH - w->height()));
}

void TimelineContentWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    fitFloatingWidget(m_selectionDetails, width(), height());
}

int TimelineContentWidget::painterToAggregator(int painterIdx) const
{
    if (painterIdx >= 0 && painterIdx < m_painterAggregatorMap.size())
        return m_painterAggregatorMap[painterIdx];
    return -1;
}

int TimelineContentWidget::aggregatorToPainter(int aggIdx) const
{
    return m_painterAggregatorMap.indexOf(aggIdx);
}

// Wire a freshly created track view (either backend). Both expose the same
// signals, so the connections are made against the concrete type T - a common
// QObject base is impossible because the two QWidget bases differ. Each view is
// wired once, when it is first created; activateTrackView() only toggles which
// one is visible.
template<class T>
void TimelineContentWidget::wireTrackView(T *view)
{
    // Drive the render widget's scroll offset from the scroll bar.
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            view, [view](int y) { view->setScrollOffset(y); });

    // Per-area interaction signals (one connection for the whole track area).
    connect(view, &T::itemClicked, this, &TimelineContentWidget::selectItem);
    connect(view, &T::itemHovered, this, &TimelineContentWidget::onItemHovered);
    connect(view, &T::horizontalPan, this, [this](int dx) { applyHorizontalPan(dx); });
    connect(view, &T::verticalPan, this, [this](int dy) {
        QScrollBar *vbar = m_scrollArea->verticalScrollBar();
        vbar->setValue(vbar->value() - dy);
    });
    connect(view, &T::zoomRequested,
            this, [this](double cursorX, int dy) { applyZoom(cursorX, dy); });

    if (frameTimeLog().isDebugEnabled())
        connect(view, &T::painted, this, &TimelineContentWidget::onFramePainted);

    // Range + rangeChanged connection (kept for the view's whole lifetime, so a
    // hidden view stays in sync and is current when shown again).
    m_sync->registerContent(view);
}

// Make `backend` the active (visible) track view, creating it on first use.
// Both backends are created lazily and then kept alive for the lifetime of the
// content widget - only their visibility is toggled, never destroyed. Recreating
// the QCanvasPainter (a QRhiWidget) tears down and rebuilds the window's RHI
// (Metal) backingstore, which is fragile and crashes when switched repeatedly.
void TimelineContentWidget::activateTrackView(TrackBackend backend)
{
    QWidget *viewport = m_scrollArea->viewport();
    QWidget *previous = m_tracksWidget;

    if (backend == TrackBackend::Software) {
        if (!m_rasterView) {
            m_rasterView = new TrackPainterRaster(viewport);
            wireTrackView(m_rasterView);
        }
        m_tracksView = m_rasterView;
        m_tracksWidget = m_rasterView;
    } else {
        if (!m_gpuView) {
            m_gpuView = new TrackPainter(viewport);
            wireTrackView(m_gpuView);
        }
        m_tracksView = m_gpuView;
        m_tracksWidget = m_gpuView;
    }

    m_tracksWidget->resize(viewport->size());
    m_tracksWidget->show();
    if (previous && previous != m_tracksWidget)
        previous->hide();
    // Keep the selection-range overlay and frame-time label above the view.
    if (m_overlay)
        m_overlay->raise();
    if (m_frameTimeLabel)
        m_frameTimeLabel->raise();
}

TrackBackend TimelineContentWidget::trackBackend() const
{
    return m_tracksView ? m_tracksView->backend() : resolvedTrackBackend();
}

void TimelineContentWidget::setTrackBackend(TrackBackend backend)
{
    if (m_tracksView && m_tracksView->backend() == backend)
        return;
    setTrackBackendOverride(backend);
    activateTrackView(backend);
    // The newly shown view may be stale (models/notes/selection changed while it
    // was hidden); repopulate it with the current state.
    m_tracksView->setMarkers(m_markers);
    rebuildTracks();
    emit trackBackendChanged(trackBackend());
}

void TimelineContentWidget::rebuildTracks()
{
    // Save selection to restore after rebuild (expand/collapse keeps the item alive)
    int savedModelId = -1;
    int savedItemIndex = -1;
    if (m_selectedModelIndex >= 0 && m_selectedModelIndex < m_trackModels.size()
            && m_selectedItemIndex >= 0) {
        savedModelId = m_trackModels[m_selectedModelIndex]->modelId();
        savedItemIndex = m_selectedItemIndex;
    }

    // Disconnect all per-model signals from this before rebuilding
    for (TimelineModel *model : m_aggregator->models()) {
        disconnect(model, nullptr, this, nullptr);
    }

    m_trackModels.clear();
    m_painterAggregatorMap.clear();
    m_selectedModelIndex = -1;
    m_selectedItemIndex = -1;
    m_hoveredModelIndex = -1;
    m_hoveredItemIndex = -1;

    QList<TrackInfo> tracks;
    const bool hasTrace = m_zoom->traceDuration() > 0;
    int aggIdx = -1;
    for (TimelineModel *model : m_aggregator->models()) {
        ++aggIdx;


        // Connect rebuild signals for ALL models so hidden/empty models becoming visible
        // trigger a rebuild without needing a modelsChanged emission.
        connect(model, &TimelineModel::hiddenChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::labelsChanged, this, [this] { rebuildTracks(); });

        if (model->hidden())
            continue;
        if (hasTrace && model->isEmpty())
            continue;

        m_trackModels.append(model);
        m_painterAggregatorMap.append(aggIdx);
        const int modelIndex = m_trackModels.size() - 1;

        // Per-model live-update connections (hiddenChanged and labelsChanged are above)
        connect(model, &TimelineModel::expandedChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::displayNameChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::tooltipChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::categoryColorChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::heightChanged, this, [this] {
            m_tracksView->refreshGeometry();
            updateContainerSize();
        });
        connect(model, &TimelineModel::detailsChanged, this,
                [this, modelIndex] {
            if (m_selectedModelIndex == modelIndex && m_selectedItemIndex >= 0)
                showItemDetails(m_selectedModelIndex, m_selectedItemIndex);
        });

        TrackInfo info;
        info.name = model->displayName();
        info.tooltip = model->tooltip();
        info.color = model->categoryColor();
        info.expanded = model->expanded();
        info.empty = model->isEmpty();
        for (int row = 0; row < model->rowCount(); ++row)
            info.rowHeights.append(model->rowHeight(row));
        if (model->expanded()) {
            for (const RowLabel &label : model->labels()) {
                info.rowLabels.append(label.description);
                info.rowLabelIds.append(label.id);
            }
        }
        if (const TimelineNotesModel *notes = m_aggregator->notes()) {
            const QList<int> noteIds = notes->byTimelineModel(model->modelId());
            for (const int noteId : noteIds) {
                info.noteEventIds.append(notes->timelineIndex(noteId));
                info.noteTexts.append(notes->text(noteId));
            }
        }
        tracks.append(info);
    }

    m_tracksView->setNotes(m_aggregator->notes());
    m_tracksView->setSelectionLocked(m_selectionLocked);
    m_tracksView->setTracks(m_trackModels);
    updateContainerSize();
    m_tracksView->setScrollOffset(m_scrollArea->verticalScrollBar()->value());

    m_labels->setTracks(tracks);
    m_labels->setScrollOffset(m_scrollArea->verticalScrollBar()->value());

    // Restore selection if the item's model is still visible and the item still
    // exists; otherwise clear details. The item index can go stale when the model
    // was cleared (e.g. on a new profiling run), which would index out of bounds.
    if (savedModelId >= 0) {
        for (int i = 0; i < m_trackModels.size(); ++i) {
            const TimelineModel *model = m_trackModels[i];
            if (model->modelId() == savedModelId
                    && savedItemIndex >= 0 && savedItemIndex < model->count()) {
                m_selectedModelIndex = i;
                m_selectedItemIndex = savedItemIndex;
                m_tracksView->setSelectedItem(i, savedItemIndex);
                showItemDetails(i, savedItemIndex);
                return;
            }
        }
    }
    m_selectedModelIndex = -1;
    m_selectedItemIndex = -1;
    m_details->clear();
}

void TimelineContentWidget::updateNotes()
{
    TimelineNotesModel *notes = m_aggregator->notes();
    if (m_notes != notes) {
        if (m_notes)
            disconnect(m_notes, &TimelineNotesModel::changed, this,
                       &TimelineContentWidget::rebuildTracks);
        m_notes = notes;
        if (m_notes)
            connect(m_notes, &TimelineNotesModel::changed, this,
                    &TimelineContentWidget::rebuildTracks);
    }
    m_tracksView->setNotes(notes);
    rebuildTracks();
}

void TimelineContentWidget::onItemHovered(int modelIndex, int itemIndex)
{
    if (m_hoveredModelIndex == modelIndex && m_hoveredItemIndex == itemIndex)
        return;
    m_hoveredModelIndex = modelIndex;
    m_hoveredItemIndex = itemIndex;
    emit itemHovered(modelIndex, itemIndex);

    if (!m_selectionLocked) {
        // In hover mode (unlocked), hovering over an item fully selects it so that
        // external views stay in sync. Moving away does not clear so the last details
        // remain readable.
        if (itemIndex >= 0)
            selectItem(modelIndex, itemIndex);
    }
}

void TimelineContentWidget::selectItem(int modelIndex, int itemIndex)
{
    m_selectedModelIndex = modelIndex;
    m_selectedItemIndex = itemIndex;

    // The single render widget holds one (track, item) selection; setting it
    // replaces any previous selection.
    if (modelIndex >= 0 && modelIndex < m_trackModels.size())
        m_tracksView->setSelectedItem(modelIndex, itemIndex);
    else
        m_tracksView->setSelectedItem(-1, -1);

    if (modelIndex >= 0 && modelIndex < m_trackModels.size() && itemIndex >= 0) {
        const TimelineModel *model = m_trackModels[modelIndex];
        const int newTypeId = model->typeId(itemIndex);
        const ItemLocation loc = model->location(itemIndex);
        m_currentFile = loc.file;
        m_currentLine = loc.line;
        m_currentColumn = loc.column;
        if (newTypeId != m_currentTypeId) {
            m_currentTypeId = newTypeId;
            emit m_aggregator->updateCursorPosition();
        }
    } else {
        m_currentTypeId = -1;
        m_currentFile.clear();
        m_currentLine = -1;
        m_currentColumn = 0;
    }

    recenterOnItem(modelIndex, itemIndex);
    showItemDetails(modelIndex, itemIndex);
}

void TimelineContentWidget::applyHorizontalPan(int dx)
{
    if (m_trackModels.isEmpty() || dx == 0)
        return;
    const int viewW = m_tracksWidget->width();
    if (viewW <= 0)
        return;
    const qint64 rangeDuration = m_zoom->rangeEnd() - m_zoom->rangeStart();
    const qint64 dt = qRound64(double(dx) * double(rangeDuration) / double(viewW));
    const qint64 traceStart = m_zoom->traceStart();
    const qint64 traceEnd = m_zoom->traceEnd();
    qint64 newStart = m_zoom->rangeStart() - dt;
    qint64 newEnd = m_zoom->rangeEnd() - dt;
    if (newStart < traceStart) {
        newEnd += traceStart - newStart;
        newStart = traceStart;
    }
    if (newEnd > traceEnd) {
        newStart -= newEnd - traceEnd;
        newStart = qMax(newStart, traceStart);
        newEnd = traceEnd;
    }
    m_zoom->setRange(newStart, newEnd);
}

void TimelineContentWidget::recenterOnItem(int modelIndex, int itemIndex)
{
    if (modelIndex < 0 || modelIndex >= m_trackModels.size() || itemIndex < 0)
        return;
    const TimelineModel *model = m_trackModels[modelIndex];
    const qint64 startTime = model->startTime(itemIndex);
    const qint64 endTime = model->endTime(itemIndex);

    // Horizontal: bring the item into the visible time range if needed
    if (endTime < m_zoom->rangeStart() || startTime > m_zoom->rangeEnd()) {
        const qint64 dur = m_zoom->rangeEnd() - m_zoom->rangeStart();
        qint64 newStart = (startTime + endTime - dur) / 2;
        newStart = qBound(m_zoom->traceStart(), newStart, m_zoom->traceEnd() - dur);
        m_zoom->setRange(newStart, newStart + dur);
    }

    // Vertical: scroll so the item's row is visible
    const int row = model->row(itemIndex);
    const int rowTop = m_tracksView->trackYOffset(modelIndex) + model->rowOffset(row);
    const int rowH = model->rowHeight(row);
    QScrollBar *vbar = m_scrollArea->verticalScrollBar();
    const int viewH = m_scrollArea->viewport()->height();
    const int curY = vbar->value();
    if (rowTop < curY || rowTop + rowH > curY + viewH)
        vbar->setValue(qBound(vbar->minimum(),
                              rowTop + rowH / 2 - viewH / 2,
                              vbar->maximum()));
}

void TimelineContentWidget::applyZoom(double cursorX, int dy)
{
    if (m_trackModels.isEmpty())
        return;
    const int viewW = m_tracksWidget->width();
    if (viewW <= 0)
        return;

    // dy > 0 = scroll up = zoom in (shrink range); dy < 0 = zoom out
    const double factor = std::pow(1.2, double(-dy) / 15.0);
    const qint64 rangeDuration = m_zoom->rangeEnd() - m_zoom->rangeStart();
    const qint64 newDuration = qBound(m_zoom->minimumRangeLength(),
                                      qRound64(double(rangeDuration) * factor),
                                      m_zoom->traceDuration());

    // Keep the time under the cursor fixed
    const double cursorFraction = qBound(0.0, cursorX / double(viewW), 1.0);
    const qint64 cursorTime = m_zoom->rangeStart()
                              + qRound64(cursorFraction * double(rangeDuration));
    qint64 newStart = cursorTime - qRound64(cursorFraction * double(newDuration));
    qint64 newEnd = newStart + newDuration;

    const qint64 traceStart = m_zoom->traceStart();
    const qint64 traceEnd = m_zoom->traceEnd();
    if (newStart < traceStart) {
        newEnd += traceStart - newStart;
        newStart = traceStart;
    }
    if (newEnd > traceEnd) {
        newStart -= newEnd - traceEnd;
        newStart = qMax(newStart, traceStart);
        newEnd = traceEnd;
    }
    m_zoom->setRange(newStart, newEnd);
}

bool TimelineContentWidget::selectionRangeReady() const
{
    return m_overlay->isReady();
}

void TimelineContentWidget::selectByAggregatorIndex(int aggModelIndex, int itemIndex)
{
    const int pi = aggregatorToPainter(aggModelIndex);
    // Pre-set typeId to suppress updateCursorPosition feedback to the caller.
    if (pi >= 0 && pi < m_trackModels.size() && itemIndex >= 0 && m_selectedItemIndex != -1)
        m_currentTypeId = m_trackModels[pi]->typeId(itemIndex);
    selectItem(pi, itemIndex);
}

void TimelineContentWidget::selectByTypeId(int typeId)
{
    if (typeId == -1 || m_currentTypeId == typeId)
        return;

    const QList<TimelineModel *> models = m_aggregator->models();

    // Check notes first
    if (const TimelineNotesModel *notes = m_aggregator->notes()) {
        const QList<int> noteIds = notes->byTypeId(typeId);
        if (!noteIds.isEmpty()) {
            const int noteIdx = noteIds.first();
            const int modelId = notes->timelineModel(noteIdx);
            const int itemIdx = notes->timelineIndex(noteIdx);
            for (int aggIdx = 0; aggIdx < models.size(); ++aggIdx) {
                TimelineModel *model = models[aggIdx];
                if (model->modelId() == modelId) {
                    const int pi = aggregatorToPainter(aggIdx);
                    if (pi < 0)
                        break;
                    m_currentTypeId = typeId;
                    selectItem(pi, itemIdx);
                    setSelectionLocked(true);
                    return;
                }
            }
        }
    }

    // Otherwise search models for items matching the type ID
    const int selectedAgg = painterToAggregator(m_selectedModelIndex);
    for (int aggIdx = 0; aggIdx < models.size(); ++aggIdx) {
        TimelineModel *model = models[aggIdx];

        if (aggIdx == selectedAgg && m_selectedItemIndex != -1
                && model->typeId(m_selectedItemIndex) == typeId) {
            break;
        }
        if (!model->handlesTypeId(typeId))
            continue;
        const int itemIdx = model->nextItemByTypeId(
            typeId, m_zoom->rangeStart(),
            aggIdx == selectedAgg ? m_selectedItemIndex : -1);
        if (itemIdx != -1) {
            const int pi = aggregatorToPainter(aggIdx);
            if (pi < 0)
                continue;
            m_currentTypeId = typeId;
            selectItem(pi, itemIdx);
            setSelectionLocked(true);
            return;
        }
    }
}

void TimelineContentWidget::jumpToNext()
{
    const ModelItem next = m_aggregator->nextItem(
        painterToAggregator(m_selectedModelIndex), m_selectedItemIndex, m_zoom->rangeStart());
    selectItem(aggregatorToPainter(next.modelIndex), next.itemIndex);
}

void TimelineContentWidget::jumpToPrev()
{
    const ModelItem prev = m_aggregator->prevItem(
        painterToAggregator(m_selectedModelIndex), m_selectedItemIndex, m_zoom->rangeEnd());
    selectItem(aggregatorToPainter(prev.modelIndex), prev.itemIndex);
}

void TimelineContentWidget::clear()
{
    selectItem(-1, -1);
    m_details->clear();
    setSelectionRangeMode(false);
    setSelectionLocked(true);
}

bool TimelineContentWidget::selectionLocked() const
{
    return m_selectionLocked;
}

void TimelineContentWidget::setSelectionLocked(bool locked)
{
    if (m_selectionLocked == locked)
        return;
    m_selectionLocked = locked;
    m_tracksView->setSelectionLocked(locked);
    emit selectionLockedChanged(locked);
}

void TimelineContentWidget::showItemDetails(int modelIndex, int itemIndex)
{
    if (modelIndex >= 0 && modelIndex < m_trackModels.size() && itemIndex >= 0) {
        const TimelineModel *model = m_trackModels[modelIndex];
        const OrderedItemDetails od = model->orderedDetails(itemIndex);
        const QString title = od.title;
        const QList<QPair<QString, QString>> content = od.content;
        m_details->setData(title, content);
    } else {
        m_details->clear();
    }
}

} // namespace Timeline
