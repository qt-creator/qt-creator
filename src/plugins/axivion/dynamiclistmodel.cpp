// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dynamiclistmodel.h"

#include "axivionplugin.h"
#include "axiviontr.h"

#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

namespace Axivion::Internal {

DynamicListModel::DynamicListModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_fetchMoreTimer.setSingleShot(true);
    m_fetchMoreTimer.setInterval(50);
    connect(&m_fetchMoreTimer, &QTimer::timeout, this, &DynamicListModel::fetchNow);
}

DynamicListModel::~DynamicListModel()
{
    clear();
}

QModelIndex DynamicListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return {};
    if (row < m_expectedRowCount.value_or(m_children.size())) {
        auto it = m_children.constFind(row);
        return createIndex(row, column, it != m_children.constEnd() ? it.value() : nullptr);
    }
    return {};
}

QModelIndex DynamicListModel::parent(const QModelIndex &/*child*/) const
{
    return {};
}

int DynamicListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) // for simplicity only single level
        return 0;

    return m_expectedRowCount.value_or(m_children.size());
}

int DynamicListModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_columnCount;
}

QVariant DynamicListModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row > m_expectedRowCount.value_or(m_children.size()))
        return {};

    auto item = m_children.constFind(row);
    if (item != m_children.cend()) {
        if (role == Qt::TextAlignmentRole) {
            if (!m_alignments.isEmpty() && index.column() < m_alignments.size())
                return QVariant::fromValue(m_alignments.at(index.column()));
        }
        return item.value()->data(index.column(), role);
    }

    if ((row < m_lastFetch || row > m_lastFetchEnd) && (row < m_fetchStart || row > m_fetchEnd))
        const_cast<DynamicListModel *>(this)->onNeedFetch(row);
    if (role == Qt::DisplayRole && index.column() == 0)
        return Tr::tr("Fetching..."); // TODO improve/customize?
    if (role == Qt::ForegroundRole && index.column() == 0)
        return Utils::creatorTheme()->color(Utils::Theme::TextColorDisabled);
    return {};
}

bool DynamicListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    auto found = m_children.constFind(index.row());
    if (found == m_children.constEnd())
        return false;
    return found.value()->setData(index.column(), value, role);
}

QVariant DynamicListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < m_header.size())
        return m_header.at(section);
    return {};
}

void DynamicListModel::setItems(const QList<ListItem *> &items)
{
    m_lastFetchEnd = -1; // FIXME wrong.. better a callback? - should happen for failed requests too
    // for simplicity we assume an ordered list and no non-existing items between first and last
    if (items.isEmpty())
        return;
    // to keep it simple here, we expect the expectedRowCount to be set *before* adding items
    QTC_ASSERT(m_expectedRowCount, setExpectedRowCount(items.size()));
    if (int lastRow = items.last()->row; lastRow > *m_expectedRowCount)
        m_expectedRowCount.emplace(lastRow);

    emit layoutAboutToBeChanged();
    auto end = m_children.end();
    for (ListItem *it : items) {
        auto found = m_children.find(it->row); // check for old data to be removed
        ListItem *old = nullptr;
        if (found != end)
            old = found.value();
        m_children.insert(it->row, it);
        delete old;
    }
    emit dataChanged(indexForItem(items.first()), indexForItem(items.last()));
    emit layoutChanged();
}

void DynamicListModel::clear()
{
    beginResetModel();
    qDeleteAll(m_children);
    m_children.clear();
    m_expectedRowCount.reset();
    endResetModel();
}

void DynamicListModel::setExpectedRowCount(int expected)
{
    QTC_ASSERT(expected >= m_children.size(), return);
    if (expected == m_children.size())
        return;
    beginInsertRows({}, m_children.size(), expected);
    m_expectedRowCount.emplace(expected);
    endInsertRows();
}

void DynamicListModel::setHeader(const QStringList &header)
{
    m_header = header;
    m_columnCount = m_header.size();
}

void DynamicListModel::setAlignments(const QList<Qt::Alignment> &alignments)
{
    m_alignments = alignments;
}

QModelIndex DynamicListModel::indexForItem(const ListItem *item) const
{
    QTC_ASSERT(item, return {});
    auto found = m_children.constFind(item->row);
    if (found == m_children.cend())
        return {};
    QTC_ASSERT(found.value() == item, return {});
    return createIndex(item->row, 0, item);
}

void DynamicListModel::onNeedFetch(int row)
{
    m_fetchStart = row;
    m_fetchEnd = row + DefaultSearchLimit;
    if (m_fetchStart < 0)
        return;
    m_fetchMoreTimer.start();
}

void DynamicListModel::fetchNow()
{
    const int old = m_lastFetch;
    m_lastFetch = m_fetchStart; // we need the "original" fetch request to avoid endless loop
    m_lastFetchEnd = m_fetchStart + DefaultSearchLimit;

    if (old != -1) {
        const int diff = old - m_fetchStart;
        if (0 < diff && diff < DefaultSearchLimit) {
            m_fetchStart = qMax(old - DefaultSearchLimit, 0);
        } else if (0 > diff && diff > - DefaultSearchLimit) {
            m_fetchStart = old + DefaultSearchLimit;
            if (m_expectedRowCount && m_fetchStart > *m_expectedRowCount)
                m_fetchStart = *m_expectedRowCount;
        }
    }

    QTC_CHECK(m_expectedRowCount ? m_fetchStart <= *m_expectedRowCount
                                 : m_fetchStart >= m_children.size());
    emit fetchRequested(m_fetchStart, DefaultSearchLimit);
    m_fetchStart = -1;
    m_fetchEnd = -1;
}

} // namespace Axivion::Internal
