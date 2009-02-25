/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "searchresulttreemodel.h"
#include "searchresulttreeitems.h"
#include "searchresulttreeitemroles.h"

#include <QtGui/QFont>
#include <QtGui/QColor>

using namespace Find::Internal;

SearchResultTreeModel::SearchResultTreeModel(QObject *parent)
  : QAbstractItemModel(parent), m_lastAppendedResultFile(0)
{
    m_rootItem = new SearchResultTreeItem();
}

SearchResultTreeModel::~SearchResultTreeModel()
{
    delete m_rootItem;
}

QModelIndex SearchResultTreeModel::index(int row, int column,
                          const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const SearchResultTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<const SearchResultTreeItem*>(parent.internalPointer());

    const SearchResultTreeItem *childItem = parentItem->getChild(row);
    if (childItem)
        return createIndex(row, column, (void *)childItem);
    else
        return QModelIndex();
}

QModelIndex SearchResultTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    const SearchResultTreeItem *childItem = static_cast<const SearchResultTreeItem*>(index.internalPointer());
    const SearchResultTreeItem *parentItem = childItem->getParent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->getRowOfItem(), 0, (void *)parentItem);
}

int SearchResultTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    const SearchResultTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<const SearchResultTreeItem*>(parent.internalPointer());

    return parentItem->getChildrenCount();
}

int SearchResultTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant SearchResultTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const SearchResultTreeItem *item = static_cast<const SearchResultTreeItem*>(index.internalPointer());

    QVariant result;

    if (item->getItemType() == SearchResultTreeItem::resultRow)
    {
        const SearchResultTextRow *row = static_cast<const SearchResultTextRow *>(item);
        result = data(row, role);
    }
    else if (item->getItemType() == SearchResultTreeItem::resultFile)
    {
        const SearchResultFile *file = static_cast<const SearchResultFile *>(item);
        result = data(file, role);
    }

    return result;
}

QVariant SearchResultTreeModel::data(const SearchResultTextRow *row, int role) const
{
    QVariant result;

    switch (role)
    {
    case Qt::ToolTipRole:
        result = row->rowText().trimmed();
        break;
    case Qt::FontRole:
        result = QFont("courier");
        break;
    case ItemDataRoles::ResultLineRole:
    case Qt::DisplayRole:
        result = row->rowText();
        break;
    case ItemDataRoles::ResultIndexRole:
        result = row->index();
        break;
    case ItemDataRoles::ResultLineNumberRole:
        result = row->lineNumber();
        break;
    case ItemDataRoles::SearchTermStartRole:
        result = row->searchTermStart();
        break;
    case ItemDataRoles::SearchTermLengthRole:
        result = row->searchTermLength();
        break;
    case ItemDataRoles::TypeRole:
        result = "row";
        break;
    case ItemDataRoles::FileNameRole:
        {
            const SearchResultFile *file = dynamic_cast<const SearchResultFile *>(row->getParent());
            result = file->getFileName();
            break;
        }
    default:
        result = QVariant();
        break;
    }

    return result;
}

QVariant SearchResultTreeModel::data(const SearchResultFile *file, int role) const
{
    QVariant result;

    switch (role)
    {
    case Qt::BackgroundRole:
        result = QColor(qRgb(245, 245, 245));
        break;
    case Qt::FontRole:
        {
            QFont font;
            font.setPointSize(font.pointSize() + 1);
            result = font;
            break;
        }
    case Qt::DisplayRole:
        result = file->getFileName() + " (" + QString::number(file->getChildrenCount()) + ")";
        break;
    case ItemDataRoles::FileNameRole:
    case Qt::ToolTipRole:
        result = file->getFileName();
        break;
    case ItemDataRoles::ResultLinesCountRole:
        result = file->getChildrenCount();
        break;
    case ItemDataRoles::TypeRole:
        result = "file";
        break;
    default:
        result = QVariant();
        break;
    }

    return result;
}

QVariant SearchResultTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

void SearchResultTreeModel::appendResultFile(const QString &fileName)
{
    m_lastAppendedResultFile = new SearchResultFile(fileName, m_rootItem);

    beginInsertRows(QModelIndex(), m_rootItem->getChildrenCount(), m_rootItem->getChildrenCount());
    m_rootItem->appendChild(m_lastAppendedResultFile);
    endInsertRows();
}

void SearchResultTreeModel::appendResultLine(int index, int lineNumber, const QString &rowText, int searchTermStart,
    int searchTermLength)
{
    if (!m_lastAppendedResultFile)
        return;

    QModelIndex lastFile(createIndex(m_lastAppendedResultFile->getRowOfItem(), 0, m_lastAppendedResultFile));

    beginInsertRows(lastFile, m_lastAppendedResultFile->getChildrenCount(), m_lastAppendedResultFile->getChildrenCount());
    m_lastAppendedResultFile->appendResultLine(index, lineNumber, rowText, searchTermStart, searchTermLength);
    endInsertRows();

    dataChanged(lastFile, lastFile); // Make sure that the number after the file name gets updated
}

void SearchResultTreeModel::appendResultLine(int index, const QString &fileName, int lineNumber, const QString &rowText,
    int searchTermStart, int searchTermLength)
{
    if (!m_lastAppendedResultFile || (m_lastAppendedResultFile->getFileName() != fileName))
        appendResultFile(fileName);

    appendResultLine(index, lineNumber, rowText, searchTermStart, searchTermLength);
}

void SearchResultTreeModel::clear(void)
{
    m_lastAppendedResultFile = NULL;
    m_rootItem->clearChildren();
    reset();
}
