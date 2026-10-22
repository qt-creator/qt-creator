// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tracklabels.h"

#include "trackpainterbase.h"
#include "tracingtr.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QToolTip>

namespace Timeline {

static constexpr int kAccentWidth = Utils::StyleHelper::SpacingTokens::PaddingHXs;
static constexpr int kTextLeftMargin = kTimeLineLeftMargin + kAccentWidth
                                       + Utils::StyleHelper::SpacingTokens::PrimitiveS;
static constexpr int kNoteButtonWidth = 20;
static constexpr int kIndicatorSize = 16;
static constexpr int kTextRightMargin = kIndicatorSize
                                        + Utils::StyleHelper::SpacingTokens::PaddingHXs
                                        + kIndicatorSize;
static constexpr int kResizeZone = 5;
static constexpr int kMinRowHeight = 30;
static constexpr int kGripDot = Utils::StyleHelper::SpacingTokens::PrimitiveXxs;
static constexpr int kGripColumns = 2;
static constexpr int kGripRows = 3;
static constexpr int kDropMarkerSize = Utils::StyleHelper::SpacingTokens::PrimitiveS;

static int trackTitleHeight(const TrackInfo &track)
{
    return track.rowHeights.isEmpty() ? kMinRowHeight : track.rowHeights[0];
}

static int trackTotalHeight(const TrackInfo &track)
{
    int height = 0;
    for (int h : track.rowHeights)
        height += h;
    return kTrackPaddingAndOutline + height + kTrackPaddingAndOutline;
}

TrackLabels::TrackLabels(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void TrackLabels::setTracks(const QList<TrackInfo> &tracks)
{
    m_tracks = tracks;
    m_noteCurrentIdx.resize(tracks.size());
    m_noteCurrentIdx.fill(-1);
    m_hoverTrack = -1;
    updateTotalHeight();
    update();
}

void TrackLabels::setScrollOffset(int y)
{
    if (m_scrollOffset == y)
        return;
    m_scrollOffset = y;
    update();
}

void TrackLabels::setHiddenTrackCount(int count)
{
    if (m_hiddenTrackCount == count)
        return;
    m_hiddenTrackCount = count;
    update();
}

int TrackLabels::footerHeight() const
{
    return m_hiddenTrackCount > 0 ? kMinRowHeight : 0;
}

// Below the last track, the note that some tracks are hidden. It scrolls with
// the tracks, so it sits where the hidden ones would have continued the list.
QRect TrackLabels::footerRect() const
{
    if (m_hiddenTrackCount <= 0)
        return {};
    return QRect(0, -m_scrollOffset + kTimeLineTopMargin + m_totalHeight, width(),
                 footerHeight());
}

int TrackLabels::titleRowAt(const QPoint &pos) const
{
    int y = -m_scrollOffset + kTimeLineTopMargin;
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (pos.y() >= y && pos.y() < y + trackTitleHeight(m_tracks[i]))
            return i;
        y += trackTotalHeight(m_tracks[i]);
    }
    return -1;
}

void TrackLabels::setHover(int track, bool footer)
{
    if (m_hoverTrack == track && m_hoverFooter == footer)
        return;
    m_hoverTrack = track;
    m_hoverFooter = footer;
    update();
}

QSize TrackLabels::sizeHint() const
{
    return QSize(162, m_totalHeight + footerHeight());
}

void TrackLabels::updateTotalHeight()
{
    m_totalHeight = 0;
    for (const TrackInfo &t : std::as_const(m_tracks))
        m_totalHeight += trackTotalHeight(t);
}

bool TrackLabels::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        auto *help = static_cast<QHelpEvent *>(e);
        const int hoverY = help->pos().y();
        int y = -m_scrollOffset + kTimeLineTopMargin;
        for (const TrackInfo &track : std::as_const(m_tracks)) {
            const int titleHeight = trackTitleHeight(track);
            const int trackHeight = trackTotalHeight(track);

            if (hoverY >= y && hoverY < y + titleHeight) {
                const int expandLeft = width() - kTextRightMargin;
                const int noteLeft = expandLeft - kNoteButtonWidth;
                if (!track.noteEventIds.isEmpty() && help->pos().x() >= noteLeft
                        && help->pos().x() < expandLeft) {
                    QToolTip::showText(help->globalPos(), track.noteTexts.join('\n'), this);
                } else if (help->pos().x() < kTimeLineLeftMargin) {
                    QToolTip::showText(help->globalPos(),
                                       Tr::tr("Drag to move this category. "
                                              "Right-click for more options."),
                                       this);
                } else {
                    const QString tip = track.tooltip.isEmpty() ? track.name : track.tooltip;
                    QToolTip::showText(help->globalPos(), tip, this);
                }
                return true;
            }
            if (track.expanded && track.rowLabels.size() == track.rowHeights.size() - 1) {
                int subRowY = y + titleHeight;
                for (int row = 0; row < track.rowLabels.size(); ++row) {
                    const int rowH = track.rowHeights[row + 1];
                    if (hoverY >= subRowY && hoverY < subRowY + rowH) {
                        QToolTip::showText(help->globalPos(), track.rowLabels.at(row), this);
                        return true;
                    }
                    subRowY += rowH;
                }
            }
            y += trackHeight;
        }
        if (footerRect().contains(help->pos())) {
            QToolTip::showText(help->globalPos(),
                               Tr::tr("Some categories are hidden from the timeline. "
                                      "Click to choose which ones to show."),
                               this);
            return true;
        }
        QToolTip::hideText();
        return true;
    }
    return QWidget::event(e);
}

static int insertionSlotY(const QList<TrackInfo> &tracks, int slot, int scrollOffset)
{
    int y = -scrollOffset - kTrackPaddingAndOutline + kTimeLineTopMargin;
    for (int i = 0; i < slot && i < tracks.size(); ++i)
        y += trackTotalHeight(tracks[i]);
    return y;
}

constexpr Utils::StyleHelper::TextFormat trackLabelTf {
    .themeColor = Utils::Theme::Token_Text_Default,
    .uiElement = Utils::StyleHelper::UiElementLabelMedium,
};

constexpr Utils::StyleHelper::TextFormat subTrackLabelTf {
    .themeColor = trackLabelTf.themeColor,
    .uiElement = Utils::StyleHelper::UiElementCaption,
};

void TrackLabels::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    const QColor bgColor = Utils::creatorColor(Utils::Theme::Token_Background_Default);
    const QColor trackBgColor = Utils::creatorColor(Utils::Theme::Token_Background_Muted);
    const QColor trackOutlineColor = Utils::creatorColor(Utils::Theme::Token_Stroke_Subtle);
    const QColor dividerColor = Utils::creatorColor(Utils::Theme::Timeline_DividerColor);
    const int rounding = qMax(Utils::StyleHelper::SpacingTokens::RadiusS, kTrackOutline);
    const QPen outlinePen(trackOutlineColor, kTrackOutline);
    const QFont trackLabelFont = trackLabelTf.font();
    const QColor trackLabelColor = trackLabelTf.color();
    const QFont subTrackLabelFont = subTrackLabelTf.font();
    const QColor subTrackLabelColor = subTrackLabelTf.color();
    const QColor gripIdleColor = Utils::creatorColor(Utils::Theme::Token_Text_Subtle);
    const QColor gripActiveColor = Utils::creatorColor(Utils::Theme::Token_Text_Default);
    const QColor dragColor = Utils::creatorColor(Utils::Theme::Token_Accent_Default);
    const QPen dragPen(dragColor, 2 * kTrackOutline);

    p.fillRect(rect(), bgColor);

    int y = -m_scrollOffset + kTimeLineTopMargin;

    const QRect viewRect = event->rect();
    p.fillRect(viewRect, bgColor);
    for (int index = 0; index < m_tracks.size(); ++index) {
        const TrackInfo &track = m_tracks.at(index);
        const int titleHeight = trackTitleHeight(track);
        const int trackHeight = trackTotalHeight(track);

        // Skip tracks entirely above the viewport
        if (y + trackHeight <= 0) {
            y += trackHeight;
            continue;
        }
        // Stop once below the viewport
        if (y >= height())
            break;

        // Track round rect
        const QRect trackStripR(kTimeLineLeftMargin, y - kTrackOutline, width() + rounding,
                                trackHeight - 2 * kTrackPadding);
        Utils::StyleHelper::drawCardBg(&p, trackStripR, trackBgColor, outlinePen, rounding);

        // Left accent strip
        if (track.color.isValid()) {
            QRect accentR = trackStripR;
            accentR.setWidth(kAccentWidth);
            p.save();
            p.setClipRect(accentR);
            Utils::StyleHelper::drawCardBg(&p, trackStripR, track.color, Qt::NoPen, rounding);
            p.restore();
        }

        if (m_dragging && index == m_dragSource)
            Utils::StyleHelper::drawCardBg(&p, trackStripR, Qt::NoBrush, dragPen, rounding);

        // Drag grip, left of the card, centered on the title row
        {
            const int gripW = (2 * kGripColumns - 1) * kGripDot;
            const int gripH = (2 * kGripRows - 1) * kGripDot;
            const int gx = (kTimeLineLeftMargin - gripW) / 2;
            const int gy = y + (titleHeight - gripH) / 2;
            const bool active = index == m_hoverTrack || (m_dragging && index == m_dragSource);
            const QColor gripColor = active ? gripActiveColor : gripIdleColor;
            for (int row = 0; row < kGripRows; ++row) {
                for (int column = 0; column < kGripColumns; ++column) {
                    p.fillRect(gx + 2 * column * kGripDot, gy + 2 * row * kGripDot,
                               kGripDot, kGripDot, gripColor);
                }
            }
        }

        // Title row
        const bool hasNotes = !track.noteEventIds.isEmpty();
        const int textX = kTextLeftMargin;
        const int expandLeft = width() - kTextRightMargin;
        const int noteLeft = hasNotes ? expandLeft - kNoteButtonWidth : expandLeft;
        const int textW = noteLeft - textX;
        const QRectF titleRect(textX, y, textW, titleHeight);
        p.setFont(trackLabelFont);
        p.setPen(trackLabelColor);
        p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                   p.fontMetrics().elidedText(track.name, Qt::ElideRight, textW));

        // Notes indicator button (shown when model has notes)
        if (hasNotes) {
            static const QIcon noteIcon = Utils::Icons::INFO_TOOLBAR.icon();
            const int ix = noteLeft + (kNoteButtonWidth - kIndicatorSize) / 2;
            const int iy = y + (titleHeight - kIndicatorSize) / 2;
            noteIcon.paint(&p, ix, iy, kIndicatorSize, kIndicatorSize);
        }

        // Expand/collapse indicator (right-aligned in title row)
        {
            static const QIcon up = Utils::Icon({{":/utils/images/arrowup.png",
                                                  Utils::Theme::PanelTextColorMid}},
                                                Utils::Icon::Tint).icon();
            static const QIcon down = Utils::Icon({{":/utils/images/arrowdown.png",
                                                    Utils::Theme::PanelTextColorMid}},
                                                  Utils::Icon::Tint).icon();
            const QIcon &icon = track.expanded ? up : down;
            const int ix = expandLeft + (kTextRightMargin - kIndicatorSize) / 2;
            const int iy = y + (titleHeight - kIndicatorSize) / 2;
            icon.paint(&p, ix, iy, kIndicatorSize, kIndicatorSize);
        }

        // Sub-row labels (expanded state)
        if (track.expanded && track.rowLabels.size() == track.rowHeights.size() - 1) {
            p.setFont(subTrackLabelFont);
            p.setPen(subTrackLabelColor);
            int rowY = y + titleHeight;
            for (int i = 0; i < track.rowLabels.size(); ++i) {
                if (rowY > viewRect.bottom())
                    break;
                const int rowH = track.rowHeights[i + 1];
                if (rowY + rowH >= viewRect.top()) {
                    const int rxText = textX + Utils::StyleHelper::SpacingTokens::PaddingHXs;

                    // Row border
                    p.fillRect(rxText, rowY, width() - kAccentWidth, 1, dividerColor);

                    // Row label text
                    const int rxW = width() - rxText - Utils::StyleHelper::SpacingTokens::PaddingHXxs;
                    const QRectF rowRect(rxText, rowY, rxW, rowH);
                    p.drawText(rowRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                               p.fontMetrics().elidedText(track.rowLabels[i], Qt::ElideRight, rxW));
                }
                rowY += rowH;
            }
        }

        y += trackHeight;
    }

    if (m_hiddenTrackCount > 0) {
        const QRect footer = footerRect();
        if (footer.intersects(viewRect)) {
            static const QIcon hiddenIcon = Utils::Icons::EYE_CLOSED_TOOLBAR.icon();
            const int ix = (kTimeLineLeftMargin - kIndicatorSize) / 2;
            const int iy = footer.top() + (footer.height() - kIndicatorSize) / 2;
            hiddenIcon.paint(&p, ix, iy, kIndicatorSize, kIndicatorSize);
            p.setFont(trackLabelFont);
            p.setPen(m_hoverFooter ? trackLabelColor
                                   : Utils::creatorColor(Utils::Theme::Token_Text_Muted));
            const QRect textRect = footer.adjusted(kTextLeftMargin, 0, 0, 0);
            p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                       p.fontMetrics().elidedText(
                           Tr::tr("%1 hidden").arg(m_hiddenTrackCount), Qt::ElideRight,
                           textRect.width()));
        }
    }

    // The drop marker, only where dropping would move the dragged track
    if (m_dragging && m_dragInsertSlot >= 0 && m_dragInsertSlot != m_dragSource
            && m_dragInsertSlot != m_dragSource + 1) {
        const int lineY = insertionSlotY(m_tracks, m_dragInsertSlot, m_scrollOffset);
        const int lineH = 2 * kTrackOutline;
        const int markerX = (kTimeLineLeftMargin - kDropMarkerSize) / 2;
        p.fillRect(markerX, lineY - lineH / 2, width() - markerX, lineH, dragColor);
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(dragColor);
        p.drawEllipse(QRectF(markerX, lineY - kDropMarkerSize / 2.0, kDropMarkerSize,
                             kDropMarkerSize));
        p.restore();
    }
}

void TrackLabels::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) {
        event->ignore();
        return;
    }

    const int clickX = event->pos().x();
    const int clickY = event->pos().y();

    int y = -m_scrollOffset + kTimeLineTopMargin;
    for (int i = 0; i < m_tracks.size(); ++i) {
        const TrackInfo &track = m_tracks[i];
        const int titleHeight = trackTitleHeight(track);
        const int trackHeight = trackTotalHeight(track);

        if (event->button() == Qt::RightButton && clickY >= y && clickY < y + trackHeight) {
            emit contextMenuRequested(i, event->globalPosition().toPoint());
            event->accept();
            return;
        }

        if (clickY >= y && clickY < y + titleHeight) {
            const int expandLeft = width() - kTextRightMargin;
            const int noteLeft = expandLeft - kNoteButtonWidth;
            if (clickX >= expandLeft) {
                if (track.expanded || !track.empty)
                    emit expandToggled(i);
            } else if (!track.noteEventIds.isEmpty() && clickX >= noteLeft) {
                auto &idx = m_noteCurrentIdx[i];
                if (++idx >= track.noteEventIds.size())
                    idx = 0;
                emit noteClicked(i, track.noteEventIds.at(idx));
            } else {
                m_dragSource = i;
                m_dragPressPos = event->pos();
                m_dragging = false;
                m_dragInsertSlot = -1;
                setCursor(Qt::ClosedHandCursor);
            }
            event->accept();
            return;
        }

        if (track.expanded && track.rowLabels.size() == track.rowHeights.size() - 1) {
            int subRowY = y + titleHeight;
            for (int row = 0; row < track.rowLabels.size(); ++row) {
                const int rowH = track.rowHeights[row + 1];
                if (clickY >= subRowY && clickY < subRowY + rowH) {
                    if (clickY >= subRowY + rowH - kResizeZone) {
                        m_resizeTrack = i;
                        m_resizeRow = row;
                        m_resizeStartY = clickY;
                        m_resizeOrigHeight = rowH;
                    } else {
                        const bool reverse = event->modifiers() & Qt::ShiftModifier;
                        emit rowLabelClicked(i, row, reverse);
                    }
                    event->accept();
                    return;
                }
                subRowY += rowH;
            }
        }

        y += trackHeight;
    }

    const QRect footer = footerRect();
    if (footer.contains(event->pos())) {
        emit hiddenTracksClicked(mapToGlobal(footer.bottomLeft()));
        event->accept();
        return;
    }
    event->ignore();
}

void TrackLabels::contextMenuEvent(QContextMenuEvent *event)
{
    // The track menus open on mouse press. Keep the parents' menu from opening on top.
    event->accept();
}

void TrackLabels::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragSource != -1) {
        if (!m_dragging) {
            if ((event->pos() - m_dragPressPos).manhattanLength() >
                QApplication::startDragDistance())
                m_dragging = true;
        }
        if (m_dragging) {
            setCursor(Qt::ClosedHandCursor);
            const int dragY = event->pos().y() + m_scrollOffset - kTimeLineTopMargin;
            int slot = 0;
            int cumY = 0;
            for (int i = 0; i < m_tracks.size(); ++i) {
                const int trackHeight = trackTotalHeight(m_tracks[i]);
                if (dragY < cumY + trackHeight / 2)
                    break;
                slot = i + 1;
                cumY += trackHeight;
            }
            if (m_dragInsertSlot != slot) {
                m_dragInsertSlot = slot;
                update();
            }
        }
        event->accept();
        return;
    }

    if (m_resizeRow >= 0) {
        const int delta = event->pos().y() - m_resizeStartY;
        const int newH = qMax(kMinRowHeight, m_resizeOrigHeight + delta);
        if (m_tracks[m_resizeTrack].rowHeights[m_resizeRow + 1] != newH) {
            m_tracks[m_resizeTrack].rowHeights[m_resizeRow + 1] = newH;
            update();
            emit rowHeightChangeRequested(m_resizeTrack, m_resizeRow, newH);
        }
        event->accept();
        return;
    }

    // Hover: update cursor based on what's under the mouse
    const int hoverTrack = titleRowAt(event->pos());
    const bool hoverFooter = hoverTrack < 0 && footerRect().contains(event->pos());
    setHover(hoverTrack, hoverFooter);
    if (hoverTrack >= 0) {
        const TrackInfo &track = m_tracks.at(hoverTrack);
        const int expandLeft = width() - kTextRightMargin;
        const int buttonsLeft = track.noteEventIds.isEmpty() ? expandLeft
                                                             : expandLeft - kNoteButtonWidth;
        if (event->pos().x() < buttonsLeft)
            setCursor(Qt::OpenHandCursor);
        else
            unsetCursor();
        event->accept();
        return;
    }
    if (hoverFooter) {
        setCursor(Qt::PointingHandCursor);
        event->accept();
        return;
    }
    const int hoverY = event->pos().y();
    int y = -m_scrollOffset + kTimeLineTopMargin;
    for (const TrackInfo &track : std::as_const(m_tracks)) {
        const int titleHeight = trackTitleHeight(track);
        const int trackHeight = trackTotalHeight(track);
        if (hoverY < y)
            break;
        if (hoverY < y + trackHeight) {
            if (track.expanded && hoverY >= y + titleHeight) {
                int subRowY = y + titleHeight;
                for (int row = 0; row < track.rowLabels.size()
                        && row + 1 < track.rowHeights.size(); ++row) {
                    const int rowH = track.rowHeights[row + 1];
                    if (hoverY >= subRowY + rowH - kResizeZone && hoverY < subRowY + rowH) {
                        setCursor(Qt::SizeVerCursor);
                        event->accept();
                        return;
                    }
                    subRowY += rowH;
                }
            }
            break;
        }
        y += trackHeight;
    }
    unsetCursor();
    event->accept();
}

void TrackLabels::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_resizeRow >= 0 && event->button() == Qt::LeftButton) {
        m_resizeTrack = -1;
        m_resizeRow = -1;
        unsetCursor();
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton || m_dragSource == -1) {
        event->ignore();
        return;
    }
    if (m_dragging && m_dragInsertSlot >= 0) {
        const int targetIndex = (m_dragInsertSlot > m_dragSource)
                                ? m_dragInsertSlot - 1
                                : m_dragInsertSlot;
        if (targetIndex != m_dragSource)
            emit moveCategories(m_dragSource, targetIndex);
    }
    m_dragSource = -1;
    m_dragging = false;
    m_dragInsertSlot = -1;
    const int hoverTrack = titleRowAt(event->pos());
    setHover(hoverTrack, false);
    if (hoverTrack >= 0)
        setCursor(Qt::OpenHandCursor);
    else
        unsetCursor();
    update();
    event->accept();
}

void TrackLabels::leaveEvent(QEvent *event)
{
    setHover(-1, false);
    QWidget::leaveEvent(event);
}

} // namespace Timeline
