// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "issueheaderview.h"

#include <utils/icon.h>

#include <QMouseEvent>
#include <QPainter>

namespace Axivion::Internal {

constexpr int ICON_SIZE = 16;

static QIcon iconForSorted(std::optional<Qt::SortOrder> order)
{
    const Utils::Icon UNSORTED(
                {{":/axivion/images/sortAsc.png", Utils::Theme::IconsDisabledColor},
                 {":/axivion/images/sortDesc.png", Utils::Theme::IconsDisabledColor}},
                Utils::Icon::MenuTintedStyle);
    const Utils::Icon SORT_ASC(
                {{":/axivion/images/sortAsc.png", Utils::Theme::PaletteText},
                 {":/axivion/images/sortDesc.png", Utils::Theme::IconsDisabledColor}},
                Utils::Icon::MenuTintedStyle);
    const Utils::Icon SORT_DESC(
                {{":/axivion/images/sortAsc.png", Utils::Theme::IconsDisabledColor},
                 {":/axivion/images/sortDesc.png", Utils::Theme::PaletteText}},
                Utils::Icon::MenuTintedStyle);
    static const QIcon unsorted = UNSORTED.icon();
    static const QIcon sortedAsc = SORT_ASC.icon();
    static const QIcon sortedDesc = SORT_DESC.icon();

    if (!order)
        return unsorted;
    return order.value() == Qt::AscendingOrder ? sortedAsc : sortedDesc;
}

void IssueHeaderView::setColumnInfoList(const QList<ColumnInfo> &infos)
{
    m_columnInfoList = infos;
    int oldIndex = m_currentSortIndex;
    m_currentSortIndex = -1;
    m_currentSortOrder.reset();
    if (oldIndex != -1)
        headerDataChanged(Qt::Horizontal, oldIndex, oldIndex);
}

int IssueHeaderView::currentSortColumn() const
{
    return m_currentSortOrder ? m_currentSortIndex : -1;
}

void IssueHeaderView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPoint position = event->position().toPoint();
        const int y = position.y();
        if (y > 1 && y < height() - 2) { // TODO improve
            const int pos = position.x();
            const int logical = logicalIndexAt(pos);
            m_lastToggleLogicalPos = logical;
            const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
            const int end = sectionViewportPosition(logical) + sectionSize(logical) - margin;
            const int start = end - ICON_SIZE;
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
        if (logical == m_lastToggleLogicalPos
                && logical > -1 && logical < m_columnInfoList.size()) {
            if (m_columnInfoList.at(logical).sortable) { // ignore non-sortable
                if (y < height() / 2) // TODO improve
                    onToggleSort(logical, Qt::AscendingOrder);
                else
                    onToggleSort(logical, Qt::DescendingOrder);
            }
        }
    }
    m_lastToggleLogicalPos = -1;
    QHeaderView::mouseReleaseEvent(event);
}

void IssueHeaderView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = true;
    QHeaderView::mouseMoveEvent(event);
}

void IssueHeaderView::onToggleSort(int index, Qt::SortOrder order)
{
    if (m_currentSortIndex == index) {
        if (!m_currentSortOrder || m_currentSortOrder.value() != order)
            m_currentSortOrder = order;
        else
            m_currentSortOrder.reset();
    } else {
        m_currentSortOrder = order;
    }

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
    const QSize newSize = logicalIndex < m_columnInfoList.size()
        ? QSize(qMax(m_columnInfoList.at(logicalIndex).width, oldSize.width()), oldSize.height())
        : oldSize;

    const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
    // add icon size and margin (default resize handle margin)
    return QSize{newSize.width() + ICON_SIZE + margin, qMax(newSize.height(), ICON_SIZE)};
}

void IssueHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    QHeaderView::paintSection(painter, rect, logicalIndex);
    painter->restore();
    if (logicalIndex < 0 || logicalIndex >= m_columnInfoList.size())
        return;
    if (!m_columnInfoList.at(logicalIndex).sortable)
        return;

    const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
    const QIcon icon = iconForSorted(logicalIndex == m_currentSortIndex ? m_currentSortOrder : std::nullopt);
    const int offset = qMax((rect.height() - ICON_SIZE), 0) / 2;
    const int left = rect.left() + rect.width() - ICON_SIZE - margin;
    const QRect iconRect(left, offset, ICON_SIZE, ICON_SIZE);
    const QRect clearRect(left, 0, ICON_SIZE + margin, rect.height());
    painter->save();
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    opt.rect = clearRect;
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    painter->restore();
    icon.paint(painter, iconRect);
}

} // namespace Axivion::Internal
