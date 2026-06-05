// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tracklabels.h"

#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QToolTip>

namespace Timeline {

static constexpr int kAccentWidth = 3;
static constexpr int kTextLeftMargin = 5;
static constexpr int kTextRightMargin = 20;
static constexpr int kNoteButtonWidth = 20;
static constexpr int kIndicatorSize = 16;
static constexpr int kResizeZone = 5;
static constexpr int kMinRowHeight = 30;

static QColor themeColor(Utils::Theme::Color role)
{
    if (Utils::creatorTheme())
        return Utils::creatorTheme()->color(role);
    return QColor();
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

QSize TrackLabels::sizeHint() const
{
    return QSize(162, m_totalHeight);
}

void TrackLabels::updateTotalHeight()
{
    m_totalHeight = 0;
    for (const TrackInfo &t : std::as_const(m_tracks)) {
        for (int h : t.rowHeights)
            m_totalHeight += h;
    }
}

bool TrackLabels::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        auto *help = static_cast<QHelpEvent *>(e);
        const int hoverY = help->pos().y();
        int y = -m_scrollOffset;
        for (const TrackInfo &track : std::as_const(m_tracks)) {
            const int titleHeight = track.rowHeights.isEmpty() ? 30 : track.rowHeights[0];
            int trackHeight = 0;
            for (int h : track.rowHeights)
                trackHeight += h;
            if (hoverY >= y && hoverY < y + titleHeight) {
                const int expandLeft = width() - kTextRightMargin;
                const int noteLeft = expandLeft - kNoteButtonWidth;
                if (!track.noteEventIds.isEmpty() && help->pos().x() >= noteLeft
                        && help->pos().x() < expandLeft) {
                    QToolTip::showText(help->globalPos(), track.noteTexts.join('\n'), this);
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
        QToolTip::hideText();
        return true;
    }
    return QWidget::event(e);
}

static int insertionSlotY(const QList<TrackInfo> &tracks, int slot, int scrollOffset)
{
    int y = -scrollOffset;
    for (int i = 0; i < slot && i < tracks.size(); ++i) {
        for (int h : tracks[i].rowHeights)
            y += h;
    }
    return y;
}

void TrackLabels::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    const QColor bgColor = themeColor(Utils::Theme::PanelStatusBarBackgroundColor);
    const QColor dividerColor = themeColor(Utils::Theme::Timeline_DividerColor);
    const QColor textColor = themeColor(Utils::Theme::PanelTextColorLight);

    p.fillRect(rect(), bgColor);

    QFont font = p.font();
    font.setPixelSize(11);
    p.setFont(font);

    int y = -m_scrollOffset;
    bool firstTrack = true;

    for (const TrackInfo &track : std::as_const(m_tracks)) {
        const int titleHeight = track.rowHeights.isEmpty() ? 30 : track.rowHeights[0];
        int trackHeight = 0;
        for (int h : track.rowHeights)
            trackHeight += h;

        // Skip tracks entirely above the viewport
        if (y + trackHeight <= 0) {
            y += trackHeight;
            firstTrack = false;
            continue;
        }
        // Stop once below the viewport
        if (y >= height())
            break;

        // Top divider (not for the very first track)
        if (!firstTrack) {
            p.fillRect(0, y, width(), 1, dividerColor);
        }
        firstTrack = false;

        // Left accent strip
        p.fillRect(0, y, kAccentWidth, trackHeight, track.color);

        // Title row
        const bool hasNotes = !track.noteEventIds.isEmpty();
        const int textX = kAccentWidth + kTextLeftMargin;
        const int expandLeft = width() - kTextRightMargin;
        const int noteLeft = hasNotes ? expandLeft - kNoteButtonWidth : expandLeft;
        const int textW = noteLeft - textX;
        const QRectF titleRect(textX, y, textW, titleHeight);
        p.setPen(textColor);
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
            const QIcon icon = track.expanded ? Utils::Icons::CLOSE_SPLIT_TOP.icon()
                                              : Utils::Icons::SPLIT_HORIZONTAL_TOOLBAR.icon();
            const int ix = expandLeft + (kTextRightMargin - kIndicatorSize) / 2;
            const int iy = y + (titleHeight - kIndicatorSize) / 2;
            icon.paint(&p, ix, iy, kIndicatorSize, kIndicatorSize);
        }

        // Sub-row labels (expanded state)
        if (track.expanded && track.rowLabels.size() == track.rowHeights.size() - 1) {
            int rowY = y + titleHeight;
            for (int i = 0; i < track.rowLabels.size(); ++i) {
                const int rowH = track.rowHeights[i + 1];

                // Row background + border
                p.fillRect(kAccentWidth, rowY, width() - kAccentWidth, rowH, bgColor);
                p.fillRect(kAccentWidth, rowY, width() - kAccentWidth, 1, dividerColor);
                p.fillRect(kAccentWidth, rowY, 1, rowH, dividerColor);
                p.fillRect(width() - 1, rowY, 1, rowH, dividerColor);

                // Row label text
                const int rxText = kAccentWidth + 4;
                const int rxW = width() - rxText - 2;
                const QRectF rowRect(rxText, rowY, rxW, rowH);
                p.setPen(textColor);
                p.drawText(rowRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                           p.fontMetrics().elidedText(track.rowLabels[i], Qt::ElideRight, rxW));

                rowY += rowH;
            }
        }

        y += trackHeight;
    }

    if (m_dragging && m_dragInsertSlot >= 0) {
        const int lineY = insertionSlotY(m_tracks, m_dragInsertSlot, m_scrollOffset);
        p.fillRect(0, lineY - 1, width(), 2,
                   themeColor(Utils::Theme::Timeline_HandleColor));
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

    int y = -m_scrollOffset;
    for (int i = 0; i < m_tracks.size(); ++i) {
        const TrackInfo &track = m_tracks[i];
        const int titleHeight = track.rowHeights.isEmpty() ? 30 : track.rowHeights[0];
        int trackHeight = 0;
        for (int h : track.rowHeights)
            trackHeight += h;

        if (clickY >= y && clickY < y + titleHeight) {
            if (event->button() == Qt::LeftButton) {
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
                    setCursor(Qt::OpenHandCursor);
                }
            }
            event->accept();
            return;
        }

        if (track.expanded && track.rowLabels.size() == track.rowHeights.size() - 1) {
            int subRowY = y + titleHeight;
            for (int row = 0; row < track.rowLabels.size(); ++row) {
                const int rowH = track.rowHeights[row + 1];
                if (clickY >= subRowY && clickY < subRowY + rowH) {
                    if (event->button() == Qt::LeftButton
                            && clickY >= subRowY + rowH - kResizeZone) {
                        m_resizeTrack = i;
                        m_resizeRow = row;
                        m_resizeStartY = clickY;
                        m_resizeOrigHeight = rowH;
                    } else {
                        const bool reverse = event->button() == Qt::RightButton
                                             || (event->modifiers() & Qt::ShiftModifier);
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
    event->ignore();
}

void TrackLabels::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragSource != -1) {
        if (!m_dragging) {
            if ((event->pos() - m_dragPressPos).manhattanLength() > 4)
                m_dragging = true;
        }
        if (m_dragging) {
            setCursor(Qt::ClosedHandCursor);
            const int dragY = event->pos().y() + m_scrollOffset;
            int slot = 0;
            int cumY = 0;
            for (int i = 0; i < m_tracks.size(); ++i) {
                int trackHeight = 0;
                for (int h : std::as_const(m_tracks[i].rowHeights))
                    trackHeight += h;
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
    const int hoverY = event->pos().y();
    int y = -m_scrollOffset;
    for (const TrackInfo &track : std::as_const(m_tracks)) {
        const int titleHeight = track.rowHeights.isEmpty() ? 30 : track.rowHeights[0];
        int trackHeight = 0;
        for (int h : track.rowHeights)
            trackHeight += h;
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
    unsetCursor();
    update();
    event->accept();
}

} // namespace Timeline
