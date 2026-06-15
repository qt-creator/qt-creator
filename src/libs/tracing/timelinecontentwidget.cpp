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

#include <utils/theme/theme.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QSplitterHandle>
#include <QVBoxLayout>

#include <cmath>

namespace Timeline {

class StripedBackground : public QWidget
{
public:
    explicit StripedBackground(QWidget *parent = nullptr) : QWidget(parent) {}

protected:
    void paintEvent(QPaintEvent *) override
    {
        const auto *theme = Utils::creatorTheme();
        if (!theme)
            return;
        const QColor bg1 = theme->color(Utils::Theme::Timeline_BackgroundColor1);
        const QColor bg2 = theme->color(Utils::Theme::Timeline_BackgroundColor2);
        const int rowH = TimelineModel::defaultRowHeight();
        QPainter p(this);
        for (int y = 0, row = 0; y < height(); y += rowH, ++row)
            p.fillRect(0, y, width(), rowH, (row % 2 == 0) ? bg1 : bg2);
    }
};

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

    m_trackContainer = new StripedBackground;
    m_trackLayout = new QVBoxLayout(m_trackContainer);
    m_trackLayout->setContentsMargins(0, 0, 0, 0);
    m_trackLayout->setSpacing(0);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidget(m_trackContainer);

    m_overlay = new SelectionRangeOverlay(zoom, m_scrollArea->viewport());
    m_overlay->resize(m_scrollArea->viewport()->size());
    m_overlay->raise();
    m_scrollArea->viewport()->installEventFilter(this);

    connect(m_details, &RangeDetailsWidget::recenterOnItem, this, [this] {
        recenterOnItem(m_selectedModelIndex, m_selectedItemIndex);
    });

    connect(m_details, &RangeDetailsWidget::rowDoubleClicked, this, [this](int row) {
        if (m_selectedModelIndex >= 0 && m_selectedModelIndex < m_painters.size()
                && m_selectedItemIndex >= 0) {
            m_painters[m_selectedModelIndex]->model()->navigateToDetail(
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
        const QPoint containerPos = m_trackContainer->mapFrom(
            m_scrollArea->viewport(), viewportPos);
        for (int i = 0; i < m_painters.size(); ++i) {
            const QRect r(m_painters[i]->pos(), m_painters[i]->size());
            if (r.contains(containerPos)) {
                selectItem(i, m_painters[i]->indexAt(containerPos - m_painters[i]->pos()));
                break;
            }
        }
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
        if (trackIndex < 0 || trackIndex >= m_painters.size())
            return;
        const int modelId = m_painters[trackIndex]->model()->modelId();
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
        if (trackIndex < 0 || trackIndex >= m_painters.size())
            return;
        const int modelId = m_painters[trackIndex]->model()->modelId();
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
        if (trackIndex < 0 || trackIndex >= m_painters.size())
            return;
        const int modelId = m_painters[trackIndex]->model()->modelId();
        for (TimelineModel *model : m_aggregator->models()) {
            if (model->modelId() == modelId) {
                model->setExpandedRowHeight(rowIndex + 1, newHeight);
                m_painters[trackIndex]->updateGeometry();
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
        for (auto painter : std::as_const(m_painters))
            painter->setMarkers(markers);
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
        m_overlay->resize(re->size());
    }
    return false;
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

void TimelineContentWidget::rebuildTracks()
{
    // Save selection to restore after rebuild (expand/collapse keeps the item alive)
    int savedModelId = -1;
    int savedItemIndex = -1;
    if (m_selectedModelIndex >= 0 && m_selectedModelIndex < m_painters.size()
            && m_selectedItemIndex >= 0) {
        savedModelId = m_painters[m_selectedModelIndex]->model()->modelId();
        savedItemIndex = m_selectedItemIndex;
    }

    // Disconnect all per-model signals from this before rebuilding
    for (TimelineModel *model : m_aggregator->models()) {
        disconnect(model, nullptr, this, nullptr);
    }

    // Delete painters; Qt removes their layout items automatically on destruction
    qDeleteAll(m_painters);
    m_painters.clear();
    m_painterAggregatorMap.clear();
    m_selectedModelIndex = -1;
    m_selectedItemIndex = -1;
    m_hoveredModelIndex = -1;
    m_hoveredItemIndex = -1;
    // Remove any remaining non-widget items (e.g. the trailing stretch)
    while (m_trackLayout->count() > 0)
        delete m_trackLayout->takeAt(0);

    QList<TrackInfo> tracks;
    const bool hasTrace = m_zoom->traceDuration() > 0;
    int totalRowsBefore = 0;
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

        auto painter = new TrackPainter(m_trackContainer);
        painter->setModel(model);
        painter->setNotes(m_aggregator->notes());
        painter->setStartOdd(totalRowsBefore % 2 == 0);
        painter->setSelectionLocked(m_selectionLocked);
        totalRowsBefore += model->rowCount();
        m_trackLayout->addWidget(painter);
        m_sync->registerContent(painter);
        m_painters.append(painter);
        m_painterAggregatorMap.append(aggIdx);
        const int modelIndex = m_painters.size() - 1;
        connect(painter, &TrackPainter::itemClicked, this,
                [this, modelIndex](int itemIndex) { selectItem(modelIndex, itemIndex); });
        connect(painter, &TrackPainter::itemHovered, this,
                [this, modelIndex](int itemIndex) { onItemHovered(modelIndex, itemIndex); });
        connect(painter, &TrackPainter::horizontalPan, this,
                [this](int dx) { applyHorizontalPan(dx); });
        connect(painter, &TrackPainter::verticalPan, this, [this](int dy) {
            QScrollBar *vbar = m_scrollArea->verticalScrollBar();
            vbar->setValue(vbar->value() - dy);
        });
        connect(painter, &TrackPainter::zoomRequested, this,
                [this](double cursorX, int dy) { applyZoom(cursorX, dy); });

        // Per-model live-update connections (hiddenChanged and labelsChanged are above)
        connect(model, &TimelineModel::expandedChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::displayNameChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::tooltipChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::categoryColorChanged, this, [this] { rebuildTracks(); });
        connect(model, &TimelineModel::heightChanged, painter,
                [painter] { painter->updateGeometry(); });
        connect(model, &TimelineModel::detailsChanged, painter,
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
    m_trackLayout->addStretch(1);
    m_labels->setTracks(tracks);
    m_labels->setScrollOffset(m_scrollArea->verticalScrollBar()->value());

    // Restore selection if the item's model is still visible and the item still
    // exists; otherwise clear details. The item index can go stale when the model
    // was cleared (e.g. on a new profiling run), which would index out of bounds.
    if (savedModelId >= 0) {
        for (int i = 0; i < m_painters.size(); ++i) {
            const TimelineModel *model = m_painters[i]->model();
            if (model->modelId() == savedModelId
                    && savedItemIndex >= 0 && savedItemIndex < model->count()) {
                m_selectedModelIndex = i;
                m_selectedItemIndex = savedItemIndex;
                m_painters[i]->setSelectedItem(savedItemIndex);
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
    for (auto painter : std::as_const(m_painters))
        painter->setNotes(notes);
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
    if (m_selectedModelIndex != modelIndex && m_selectedModelIndex >= 0
            && m_selectedModelIndex < m_painters.size()) {
        m_painters[m_selectedModelIndex]->setSelectedItem(-1);
    }

    m_selectedModelIndex = modelIndex;
    m_selectedItemIndex = itemIndex;

    if (modelIndex >= 0 && modelIndex < m_painters.size())
        m_painters[modelIndex]->setSelectedItem(itemIndex);

    if (modelIndex >= 0 && modelIndex < m_painters.size() && itemIndex >= 0) {
        const TimelineModel *model = m_painters[modelIndex]->model();
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
    if (m_painters.isEmpty() || dx == 0)
        return;
    const TrackPainter *painter = m_painters.first();
    const int viewW = painter->width();
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
    if (modelIndex < 0 || modelIndex >= m_painters.size() || itemIndex < 0)
        return;
    const TimelineModel *model = m_painters[modelIndex]->model();
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
    const int rowTop = m_painters[modelIndex]->pos().y() + model->rowOffset(row);
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
    if (m_painters.isEmpty())
        return;
    const int viewW = m_painters.first()->width();
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
    if (pi >= 0 && pi < m_painters.size() && itemIndex >= 0 && m_selectedItemIndex != -1)
        m_currentTypeId = m_painters[pi]->model()->typeId(itemIndex);
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
    for (auto painter : std::as_const(m_painters))
        painter->setSelectionLocked(locked);
    emit selectionLockedChanged(locked);
}

void TimelineContentWidget::showItemDetails(int modelIndex, int itemIndex)
{
    if (modelIndex >= 0 && modelIndex < m_painters.size() && itemIndex >= 0) {
        const TimelineModel *model = m_painters[modelIndex]->model();
        const OrderedItemDetails od = model->orderedDetails(itemIndex);
        const QString title = od.title;
        const QList<QPair<QString, QString>> content = od.content;
        m_details->setData(title, content);
    } else {
        m_details->clear();
    }
}

} // namespace Timeline
