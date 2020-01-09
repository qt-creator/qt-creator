/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "welcomepagehelper.h"

#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QHeaderView>
#include <QHoverEvent>
#include <QLayout>

namespace Core {

using namespace Utils;

SearchBox::SearchBox(QWidget *parent)
    : WelcomePageFrame(parent)
{
    QPalette pal;
    pal.setColor(QPalette::Base, Utils::creatorTheme()->color(Theme::Welcome_BackgroundColor));

    m_lineEdit = new FancyLineEdit;
    m_lineEdit->setFiltering(true);
    m_lineEdit->setFrame(false);
    QFont f = font();
    f.setPixelSize(14);
    m_lineEdit->setFont(f);
    m_lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_lineEdit->setPalette(pal);

    auto box = new QHBoxLayout(this);
    box->setContentsMargins(10, 3, 3, 3);
    box->addWidget(m_lineEdit);
}

GridView::GridView(QWidget *parent)
    : QTableView(parent)
{
    setVerticalScrollMode(ScrollPerPixel);
    horizontalHeader()->hide();
    horizontalHeader()->setDefaultSectionSize(GridProxyModel::GridItemWidth);
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(GridProxyModel::GridItemHeight);
    setMouseTracking(true); // To enable hover.
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setGridStyle(Qt::NoPen);

    QPalette pal;
    pal.setColor(QPalette::Base, Utils::creatorTheme()->color(Theme::Welcome_BackgroundColor));
    setPalette(pal); // Makes a difference on Mac.
}

void GridView::leaveEvent(QEvent *)
{
    QHoverEvent hev(QEvent::HoverLeave, QPointF(), QPointF());
    viewportEvent(&hev); // Seemingly needed to kill the hover paint.
}

void GridProxyModel::setSourceModel(QAbstractItemModel *newModel)
{
    if (m_sourceModel == newModel)
        return;
    if (m_sourceModel)
        disconnect(m_sourceModel, nullptr, this, nullptr);
    m_sourceModel = newModel;
    if (newModel) {
        connect(newModel, &QAbstractItemModel::layoutAboutToBeChanged, this, [this] {
            emit layoutAboutToBeChanged();
        });
        connect(newModel, &QAbstractItemModel::layoutChanged, this, [this] {
            emit layoutChanged();
        });
        connect(newModel, &QAbstractItemModel::modelAboutToBeReset, this, [this] {
            beginResetModel();
        });
        connect(newModel, &QAbstractItemModel::modelReset, this, [this] { endResetModel(); });
        connect(newModel, &QAbstractItemModel::rowsAboutToBeInserted, this, [this] {
            beginResetModel();
        });
        connect(newModel, &QAbstractItemModel::rowsInserted, this, [this] { endResetModel(); });
        connect(newModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this] {
            beginResetModel();
        });
        connect(newModel, &QAbstractItemModel::rowsRemoved, this, [this] { endResetModel(); });
    }
}

QAbstractItemModel *GridProxyModel::sourceModel() const
{
    return m_sourceModel;
}

QVariant GridProxyModel::data(const QModelIndex &index, int role) const
{
    const OptModelIndex sourceIndex = mapToSource(index);
    if (sourceIndex)
        return sourceModel()->data(*sourceIndex, role);
    return QVariant();
}

Qt::ItemFlags GridProxyModel::flags(const QModelIndex &index) const
{
    const OptModelIndex sourceIndex = mapToSource(index);
    if (sourceIndex)
        return sourceModel()->flags(*sourceIndex);
    return Qt::ItemFlags();
}

bool GridProxyModel::hasChildren(const QModelIndex &parent) const
{
    const OptModelIndex sourceParent = mapToSource(parent);
    if (sourceParent)
        return sourceModel()->hasChildren(*sourceParent);
    return false;
}

void GridProxyModel::setColumnCount(int columnCount)
{
    if (columnCount == m_columnCount)
        return;
    QTC_ASSERT(columnCount >= 1, columnCount = 1);
    m_columnCount = columnCount;
    emit layoutChanged();
}

int GridProxyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    int rows = sourceModel()->rowCount(QModelIndex());
    return (rows + m_columnCount - 1) / m_columnCount;
}

int GridProxyModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_columnCount;
}

QModelIndex GridProxyModel::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row, column, nullptr);
}

QModelIndex GridProxyModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

// The items at the lower right of the grid might not correspond to source items, if
// source's row count is not N*columnCount
OptModelIndex GridProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return QModelIndex();
    int sourceRow = proxyIndex.row() * m_columnCount + proxyIndex.column();
    if (sourceRow < sourceModel()->rowCount())
        return sourceModel()->index(sourceRow, 0);
    return OptModelIndex();
}

QModelIndex GridProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid())
        return QModelIndex();
    QTC_CHECK(sourceIndex.column() == 0);
    int proxyRow = sourceIndex.row() / m_columnCount;
    int proxyColumn = sourceIndex.row() % m_columnCount;
    return index(proxyRow, proxyColumn, QModelIndex());
}

} // namespace Core
