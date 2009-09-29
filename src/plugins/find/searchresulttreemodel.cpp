/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "searchresulttreemodel.h"
#include "searchresulttreeitems.h"
#include "searchresulttreeitemroles.h"

#include <QtGui/QApplication>
#include <QtGui/QFont>
#include <QtGui/QColor>
#include <QtGui/QPalette>
#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace Find::Internal;

SearchResultTreeModel::SearchResultTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_lastAppendedResultFile(0)
    , m_showReplaceUI(false)
{
    m_rootItem = new SearchResultTreeItem();
    m_textEditorFont = QFont("Courier");
}

SearchResultTreeModel::~SearchResultTreeModel()
{
    delete m_rootItem;
}

void SearchResultTreeModel::setShowReplaceUI(bool show)
{
    m_showReplaceUI = show;
}

void SearchResultTreeModel::setTextEditorFont(const QFont &font)
{
    m_textEditorFont = font;
}

Qt::ItemFlags SearchResultTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    if (index.isValid()) {
        if (const SearchResultTreeItem *item = static_cast<const SearchResultTreeItem*>(index.internalPointer())) {
            if (item->itemType() == SearchResultTreeItem::ResultRow && item->isUserCheckable()) {
                flags |= Qt::ItemIsUserCheckable;
            }
        }
    }

    return flags;
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

    const SearchResultTreeItem *childItem = parentItem->childAt(row);
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
    const SearchResultTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->rowOfItem(), 0, (void *)parentItem);
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

    return parentItem->childrenCount();
}

int SearchResultTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant SearchResultTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const SearchResultTreeItem *item = static_cast<const SearchResultTreeItem*>(index.internalPointer());

    QVariant result;

    if (item->itemType() == SearchResultTreeItem::ResultRow)
    {
        const SearchResultTextRow *row = static_cast<const SearchResultTextRow *>(item);
        result = data(row, role);
    }
    else if (item->itemType() == SearchResultTreeItem::ResultFile)
    {
        const SearchResultFile *file = static_cast<const SearchResultFile *>(item);
        result = data(file, role);
    }

    return result;
}

bool SearchResultTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        SearchResultTreeItem *item = static_cast<SearchResultTreeItem*>(index.internalPointer());
        SearchResultTextRow *row = static_cast<SearchResultTextRow *>(item);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());
        row->setCheckState(checkState);
        return true;
    }
    return QAbstractItemModel::setData(index, value, role);
}

QVariant SearchResultTreeModel::data(const SearchResultTextRow *row, int role) const
{
    QVariant result;

    switch (role)
    {
    case Qt::CheckStateRole:
        if (row->isUserCheckable())
            result = row->checkState();
        break;
    case Qt::ToolTipRole:
        result = row->rowText().trimmed();
        break;
    case Qt::FontRole:
        result = m_textEditorFont;
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
            if (row->parent()->itemType() == SearchResultTreeItem::ResultFile) {
                const SearchResultFile *file = static_cast<const SearchResultFile *>(row->parent());
                result = file->fileName();
            }
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
#if 0
    case Qt::CheckStateRole:
        if (file->isUserCheckable())
            result = file->checkState();
        break;
#endif
    case Qt::BackgroundRole: {
        const QColor baseColor = QApplication::palette().base().color();
        result = baseColor.darker(105);
        break;
    }
    case Qt::DisplayRole:
        result = QString(QDir::toNativeSeparators(file->fileName())
            + " (" + QString::number(file->childrenCount()) + ")");
        break;
    case ItemDataRoles::FileNameRole:
    case Qt::ToolTipRole:
        result = file->fileName();
        break;
    case ItemDataRoles::ResultLinesCountRole:
        result = file->childrenCount();
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
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

void SearchResultTreeModel::appendResultFile(const QString &fileName)
{
    m_lastAppendedResultFile = new SearchResultFile(fileName, m_rootItem);

    if (m_showReplaceUI) {
        m_lastAppendedResultFile->setIsUserCheckable(true);
        m_lastAppendedResultFile->setCheckState(Qt::Checked);
    }

    const int childrenCount = m_rootItem->childrenCount();
    beginInsertRows(QModelIndex(), childrenCount, childrenCount);
    m_rootItem->appendChild(m_lastAppendedResultFile);
    endInsertRows();
}

void SearchResultTreeModel::appendResultLine(int index, int lineNumber, const QString &rowText,
                                             int searchTermStart, int searchTermLength)
{
    if (!m_lastAppendedResultFile)
        return;

    QModelIndex lastFile(createIndex(m_lastAppendedResultFile->rowOfItem(), 0, m_lastAppendedResultFile));

    beginInsertRows(lastFile, m_lastAppendedResultFile->childrenCount(), m_lastAppendedResultFile->childrenCount());
    m_lastAppendedResultFile->appendResultLine(index, lineNumber, rowText, searchTermStart, searchTermLength);
    endInsertRows();

    dataChanged(lastFile, lastFile); // Make sure that the number after the file name gets updated
}

void SearchResultTreeModel::appendResultLine(int index, const QString &fileName, int lineNumber, const QString &rowText,
                                             int searchTermStart, int searchTermLength)
{
    if (!m_lastAppendedResultFile || (m_lastAppendedResultFile->fileName() != fileName))
        appendResultFile(fileName);

    appendResultLine(index, lineNumber, rowText, searchTermStart, searchTermLength);
}

void SearchResultTreeModel::clear()
{
    m_lastAppendedResultFile = NULL;
    m_rootItem->clearChildren();
    reset();
}

QModelIndex SearchResultTreeModel::next(const QModelIndex &idx) const
{
    QModelIndex parent = idx.parent();
    if (parent.isValid()) {
        int row = idx.row();
        if (row + 1 < rowCount(parent)) {
            // Same parent
            return index(row + 1, 0, parent);
        } else {
            // Next parent
            int parentRow = parent.row();
            QModelIndex nextParent;
            if (parentRow + 1 < rowCount()) {
                nextParent = index(parentRow + 1, 0);
            } else {
                // Wrap around
                nextParent = index(0,0);
            }
            return nextParent.child(0, 0);
        }
    } else {
        // We are on a top level item
        return idx.child(0,0);
    }
    return QModelIndex();
}

QModelIndex SearchResultTreeModel::prev(const QModelIndex &idx) const
{
    QModelIndex parent = idx.parent();
    if (parent.isValid()) {
        int row = idx.row();
        if (row  > 0) {
            // Same parent
            return index(row - 1, 0, parent);
        } else {
            // Prev parent
            int parentRow = parent.row();
            QModelIndex prevParent;
            if (parentRow > 0 ) {
                prevParent = index(parentRow - 1, 0);
            } else {
                // Wrap around
                prevParent = index(rowCount() - 1, 0);
            }
            return prevParent.child(rowCount(prevParent) - 1, 0);
        }
    } else {
        // We are on a top level item
        int row = idx.row();
        if (row > 0) {
            QModelIndex prevParent = index(row - 1, 0);
            return prevParent.child(rowCount(prevParent) ,0);
        }
    }
    return QModelIndex();
}
