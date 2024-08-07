// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "issueheaderview.h"

#include <utils/icon.h>

#include <QMouseEvent>
#include <QPainter>

namespace Axivion::Internal {

constexpr int IconSize = 16;
constexpr int InnerMargin = 4;

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

static QIcon iconForFilter(bool isActive)
{
    const Utils::Icon INACTIVE(
                {{":/utils/images/filtericon.png", Utils::Theme::IconsDisabledColor}},
                Utils::Icon::MenuTintedStyle);
    const Utils::Icon ACTIVE(
                {{":/utils/images/filtericon.png", Utils::Theme::PaletteText}},
                Utils::Icon::MenuTintedStyle);
    static const QIcon inactive = INACTIVE.icon();
    static const QIcon active = ACTIVE.icon();
    return isActive ? active : inactive;
}

void IssueHeaderView::setColumnInfoList(const QList<ColumnInfo> &infos)
{
    m_columnInfoList = infos;
    const QList<int> oldIndexes = m_currentSortIndexes;
    m_currentSortIndexes.clear();
    for (int i = 0; i < infos.size(); ++i)
        m_columnInfoList[i].sortOrder.reset();
    for (int oldIndex : oldIndexes)
        headerDataChanged(Qt::Horizontal, oldIndex, oldIndex);
}

QList<QPair<int, Qt::SortOrder>> IssueHeaderView::currentSortColumns() const
{
    QList<QPair<int, Qt::SortOrder>> result;
    for (int i : m_currentSortIndexes)
        result.append({i, m_columnInfoList.at(i).sortOrder.value()});
    return result;
}

void IssueHeaderView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPoint position = event->position().toPoint();
        const int y = position.y();
        if (y > 1 && y < height() - 2) { // TODO improve
            const int pos = position.x();
            const int logical = logicalIndexAt(pos);
            QTC_ASSERT(logical > -1 && logical < m_columnInfoList.size(),
                       QHeaderView::mousePressEvent(event); return);
            m_lastToggleLogicalPos = logical;
            const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
            const int lastIconEnd = sectionViewportPosition(logical) + sectionSize(logical) - margin;
            const int lastIconStart = lastIconEnd - IconSize;
            const int firstIconStart = lastIconStart - InnerMargin - IconSize;

            const ColumnInfo info = m_columnInfoList.at(logical);
            if (info.sortable && info.filterable) {
                if (firstIconStart < pos && firstIconStart + IconSize > pos)
                    m_maybeToggle.emplace(Sort);
                else if (lastIconStart < pos && lastIconEnd > pos)
                    m_maybeToggle.emplace(Filter);
                else
                    m_maybeToggle.reset();
            } else {
                if (lastIconStart < pos && lastIconEnd > pos)
                    m_maybeToggle.emplace(info.sortable ? Sort : Filter);
            }
            m_withShift = event->modifiers() == Qt::ShiftModifier;
        }
    }
    QHeaderView::mousePressEvent(event);
}

void IssueHeaderView::mouseReleaseEvent(QMouseEvent *event)
{
    bool dontSkip = !m_dragging && m_maybeToggle;
    const int toggleMode = m_maybeToggle ? m_maybeToggle.value() : -1;
    bool withShift = m_withShift && event->modifiers() == Qt::ShiftModifier;
    m_dragging = false;
    m_maybeToggle.reset();
    m_withShift = false;

    if (dontSkip) {
        const QPoint position = event->position().toPoint();
        const int y = position.y();
        const int logical = logicalIndexAt(position.x());
        if (logical == m_lastToggleLogicalPos
                && logical > -1 && logical < m_columnInfoList.size()) {
            if (toggleMode == Sort && m_columnInfoList.at(logical).sortable) { // ignore non-sortable
                if (y < height() / 2) // TODO improve
                    onToggleSort(logical, Qt::AscendingOrder, withShift);
                else
                    onToggleSort(logical, Qt::DescendingOrder, withShift);
            } else if (toggleMode == Filter && m_columnInfoList.at(logical).filterable) {
                // TODO we need some popup for text input (entering filter expression)
                // apply them to the columninfo, and use them for the search..
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

void IssueHeaderView::onToggleSort(int index, Qt::SortOrder order, bool multi)
{
    QTC_ASSERT(index > -1 && index < m_columnInfoList.size(), return);
    const QList<int> oldSortIndexes = m_currentSortIndexes;
    std::optional<Qt::SortOrder> oldSortOrder = m_columnInfoList.at(index).sortOrder;
    int pos = m_currentSortIndexes.indexOf(index);

    if (oldSortOrder == order)
        m_columnInfoList[index].sortOrder.reset();
    else
        m_columnInfoList[index].sortOrder = order;
    if (multi) {
        if (pos == -1)
            m_currentSortIndexes.append(index);
        else if (oldSortOrder == order)
            m_currentSortIndexes.remove(pos);
    } else {
        m_currentSortIndexes.clear();
        if (pos == -1 || oldSortOrder != order)
            m_currentSortIndexes.append(index);
        for (int oldIndex : oldSortIndexes) {
            if (oldIndex == index)
                continue;
            m_columnInfoList[oldIndex].sortOrder.reset();
        }
    }

    for (int oldIndex : oldSortIndexes)
        headerDataChanged(Qt::Horizontal, oldIndex, oldIndex);
    headerDataChanged(Qt::Horizontal, index, index);
    emit sortTriggered();
}

QSize IssueHeaderView::sectionSizeFromContents(int logicalIndex) const
{
    const QSize oldSize = QHeaderView::sectionSizeFromContents(logicalIndex);
    QTC_ASSERT(logicalIndex > -1 && logicalIndex < m_columnInfoList.size(), return oldSize);
    const ColumnInfo ci = m_columnInfoList.at(logicalIndex);
    const QSize newSize = QSize(qMax(ci.width, oldSize.width()), oldSize.height());

    if (!ci.filterable && !ci.sortable)
        return newSize;

    const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
    // compute additional space for needed icon(s)
    int additionalWidth = IconSize + margin;
    if (ci.filterable && ci.sortable)
        additionalWidth += IconSize + InnerMargin;

    return QSize{newSize.width() + additionalWidth, qMax(newSize.height(), IconSize)};
}

void IssueHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    QHeaderView::paintSection(painter, rect, logicalIndex);
    painter->restore();
    if (logicalIndex < 0 || logicalIndex >= m_columnInfoList.size())
        return;
    const ColumnInfo info = m_columnInfoList.at(logicalIndex);
    if (!info.sortable && !info.filterable)
        return;

    const int offset = qMax((rect.height() - IconSize), 0) / 2;
    const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
    const int lastIconLeft = rect.left() + rect.width() - IconSize - margin;
    const int firstIconLeft = lastIconLeft - InnerMargin - IconSize;

    const bool bothIcons = info.sortable && info.filterable;
    const QRect clearRect(bothIcons ? firstIconLeft : lastIconLeft, 0,
                          bothIcons ? IconSize + InnerMargin + IconSize + margin
                                    : IconSize + margin, rect.height());
    painter->save();
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    opt.rect = clearRect;
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    painter->restore();
    QRect iconRect(lastIconLeft, offset, IconSize, IconSize);
    if (info.filterable) {
        const QIcon icon = iconForFilter(info.filter.has_value());
        icon.paint(painter, iconRect);
        iconRect.setRect(firstIconLeft, offset, IconSize, IconSize);
    }
    if (info.sortable) {
        const QIcon icon = iconForSorted(m_currentSortIndexes.contains(logicalIndex) ? info.sortOrder
                                                                                     : std::nullopt);
        icon.paint(painter, iconRect);
    }
}

} // namespace Axivion::Internal
