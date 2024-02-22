// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "issueheaderview.h"

#include <utils/icon.h>

#include <QMouseEvent>
#include <QPainter>

namespace Axivion::Internal {

constexpr int ICON_SIZE = 16;

static QIcon iconForSorted(SortOrder order)
{
    const Utils::Icon UNSORTED(
                {{":/axivion/images/sortAsc.png", Utils::Theme::IconsDisabledColor},
                 {":/axivion/images/sortDesc.png", Utils::Theme::IconsDisabledColor}});
    const Utils::Icon SORT_ASC(
                {{":/axivion/images/sortAsc.png", Utils::Theme::PaletteText},
                 {":/axivion/images/sortDesc.png", Utils::Theme::IconsDisabledColor}});
    const Utils::Icon SORT_DESC(
                {{":/axivion/images/sortAsc.png", Utils::Theme::IconsDisabledColor},
                 {":/axivion/images/sortDesc.png", Utils::Theme::PaletteText}});
    static const QIcon unsorted = UNSORTED.icon();
    static const QIcon sortedAsc = SORT_ASC.icon();
    static const QIcon sortedDesc = SORT_DESC.icon();

    switch (order) {
    case SortOrder::None:
        return unsorted;
    case SortOrder::Ascending:
        return sortedAsc;
    case SortOrder::Descending:
        return sortedDesc;
    }
    return {};
}

void IssueHeaderView::setSortableColumns(const QList<bool> &sortable)
{
    m_sortableColumns = sortable;
    int oldIndex = m_currentSortIndex;
    m_currentSortIndex = -1;
    m_currentSortOrder = SortOrder::None;
    if (oldIndex != -1)
        headerDataChanged(Qt::Horizontal, oldIndex, oldIndex);
}

int IssueHeaderView::currentSortColumn() const
{
    return m_currentSortOrder == SortOrder::None ? -1 : m_currentSortIndex;
}

void IssueHeaderView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPoint position = event->position().toPoint();
        const int y = position.y();
        if (y > 1 && y < height() - 2) { // TODO improve
            const int pos = position.x();
            const int logical = logicalIndexAt(pos);
            const int end = sectionPosition(logical) + sectionSize(logical);
            const int start = end - ICON_SIZE - 2;
            m_maybeToggleSort = start < pos && end > pos;
        }
    }
    QHeaderView::mousePressEvent(event);
}

void IssueHeaderView::mouseReleaseEvent(QMouseEvent *event)
{
    bool dontSkip = !m_dragging && m_maybeToggleSort;
    m_dragging = false;
    m_maybeToggleSort = false;

    if (dontSkip) {
        const QPoint position = event->position().toPoint();
        const int y = position.y();
        const int logical = logicalIndexAt(position.x());
        if (logical > -1 && logical < m_sortableColumns.size()) {
            if (m_sortableColumns.at(logical)) { // ignore non-sortable
                if (y < height() / 2) // TODO improve
                    onToggleSort(logical, SortOrder::Ascending);
                else
                    onToggleSort(logical, SortOrder::Descending);
            }
        }
    }
    QHeaderView::mouseReleaseEvent(event);
}

void IssueHeaderView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = true;
    QHeaderView::mouseMoveEvent(event);
}

void IssueHeaderView::onToggleSort(int index, SortOrder order)
{
    if (m_currentSortIndex == index)
        m_currentSortOrder = (order == m_currentSortOrder) ? SortOrder::None : order;
    else
        m_currentSortOrder = order;

    int oldIndex = m_currentSortIndex;
    m_currentSortIndex = index;
    if (oldIndex != -1)
        headerDataChanged(Qt::Horizontal, oldIndex, oldIndex);
    headerDataChanged(Qt::Horizontal, index, index);
    emit sortTriggered();
}

QSize IssueHeaderView::sectionSizeFromContents(int logicalIndex) const
{
    const QSize oldSize = QHeaderView::sectionSizeFromContents(logicalIndex);
    const QSize newSize = logicalIndex < m_columnWidths.size()
        ? QSize(qMax(m_columnWidths.at(logicalIndex), oldSize.width()), oldSize.height()) : oldSize;
    // add icon size and margin (2)
    return QSize{newSize.width() + ICON_SIZE + 2, qMax(newSize.height(), ICON_SIZE)};
}

void IssueHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    QHeaderView::paintSection(painter, rect, logicalIndex);
    painter->restore();
    if (logicalIndex < 0 || logicalIndex >= m_sortableColumns.size())
        return;
    if (!m_sortableColumns.at(logicalIndex))
        return;

    const QIcon icon = iconForSorted(logicalIndex == m_currentSortIndex ? m_currentSortOrder : SortOrder::None);
    const int offset = qMax((rect.height() - ICON_SIZE), 0) / 2;
    const QRect iconRect(rect.left() + rect.width() - ICON_SIZE - 2, offset, ICON_SIZE, ICON_SIZE);
    icon.paint(painter, iconRect);
}

} // namespace Axivion::Internal
