/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qtmessageloghandler.h"

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// QtMessageLogItem
//
///////////////////////////////////////////////////////////////////////

QtMessageLogItem::QtMessageLogItem(QtMessageLogHandler::ItemType itemType,
                             const QString &text, QtMessageLogItem *parent)
    : m_text(text),
      m_itemType(itemType),
      m_parentItem(parent)

{
}

QtMessageLogItem::~QtMessageLogItem()
{
    qDeleteAll(m_childItems);
}

QtMessageLogItem *QtMessageLogItem::child(int number)
{
    return m_childItems.value(number);
}

int QtMessageLogItem::childCount() const
{
    return m_childItems.count();
}

int QtMessageLogItem::childNumber() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<QtMessageLogItem *>(this));

    return 0;
}

QString QtMessageLogItem::text() const
{
    return m_text;
}

QtMessageLogHandler::ItemType QtMessageLogItem::itemType() const
{
    return m_itemType;
}
\
bool QtMessageLogItem::insertChildren(int position, int count)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QtMessageLogItem *item = new QtMessageLogItem(QtMessageLogHandler::UndefinedType, QString(), this);
        m_childItems.insert(position, item);
    }

    return true;
}

bool QtMessageLogItem::insertChild(int position, QtMessageLogItem *item)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    if (item->parent())
        item->parent()->detachChild(item->childNumber());

    item->m_parentItem = this;
    m_childItems.insert(position, item);

    return true;
}

QtMessageLogItem *QtMessageLogItem::parent()
{
    return m_parentItem;
}

bool QtMessageLogItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > m_childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete m_childItems.takeAt(position);

    return true;
}

bool QtMessageLogItem::detachChild(int position)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    m_childItems.removeAt(position);

    return true;
}

bool QtMessageLogItem::setText(const QString &text)
{
    m_text = text;
    return true;
}

bool QtMessageLogItem::setItemType(QtMessageLogHandler::ItemType itemType)
{
    m_itemType = itemType;
    return true;
}

///////////////////////////////////////////////////////////////////////
//
// QtMessageLogHandler
//
///////////////////////////////////////////////////////////////////////

QtMessageLogHandler::QtMessageLogHandler(QObject *parent) :
    QAbstractItemModel(parent),
    m_hasEditableRow(false),
    m_rootItem(new QtMessageLogItem())
{
}

QtMessageLogHandler::~QtMessageLogHandler()
{
    delete m_rootItem;
}

void QtMessageLogHandler::clear()
{
    beginResetModel();
    reset();
    delete m_rootItem;
    m_rootItem = new QtMessageLogItem();
    endResetModel();

    if (m_hasEditableRow)
        appendEditableRow();
}

bool QtMessageLogHandler::appendItem(QtMessageLogItem *item, int position)
{
    if (position < 0)
        position = m_rootItem->childCount() - 1;

    beginInsertRows(QModelIndex(), position, position);
    bool success = m_rootItem->insertChild(position, item);
    endInsertRows();

    return success;
}

bool QtMessageLogHandler::appendMessage(QtMessageLogHandler::ItemType itemType,
                   const QString &message, int position)
{
    return appendItem(new QtMessageLogItem(itemType, message), position);
}

void QtMessageLogHandler::setHasEditableRow(bool hasEditableRow)
{
    if (m_hasEditableRow && !hasEditableRow)
        removeEditableRow();

    if (!m_hasEditableRow && hasEditableRow)
        appendEditableRow();

    m_hasEditableRow = hasEditableRow;
}

bool QtMessageLogHandler::hasEditableRow() const
{
    return m_hasEditableRow;
}

void QtMessageLogHandler::appendEditableRow()
{
    int position = m_rootItem->childCount();
    if (appendItem(new QtMessageLogItem(QtMessageLogHandler::InputType), position))
        emit selectEditableRow(index(position, 0),
                                 QItemSelectionModel::ClearAndSelect);
}

void QtMessageLogHandler::removeEditableRow()
{
    if (m_rootItem->child(m_rootItem->childCount() - 1)->itemType() == QtMessageLogHandler::InputType)
        removeRow(m_rootItem->childCount() - 1);
}

QVariant QtMessageLogHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QtMessageLogItem *item = getItem(index);

    if (role == Qt::DisplayRole )
        return item->text();
    else if (role == QtMessageLogHandler::TypeRole)
        return int(item->itemType());
    else
        return QVariant();
}

QModelIndex QtMessageLogHandler::index(int row, int column,
                                    const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    if (column > 0)
        return QModelIndex();

    QtMessageLogItem *parentItem = getItem(parent);

    QtMessageLogItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex QtMessageLogHandler::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QtMessageLogItem *childItem = getItem(index);
    QtMessageLogItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int QtMessageLogHandler::rowCount(const QModelIndex &parent) const
{
    QtMessageLogItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

int QtMessageLogHandler::columnCount(const QModelIndex & /* parent */) const
{
    return 1;
}

Qt::ItemFlags QtMessageLogHandler::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    QtMessageLogItem *item = getItem(index);
    if (m_hasEditableRow && item->parent() == m_rootItem
            && index.row() == m_rootItem->childCount() - 1)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool QtMessageLogHandler::setData(const QModelIndex &index, const QVariant &value,
                               int role)
{
    QtMessageLogItem *item = getItem(index);
    bool result = false;
    if (role == Qt::DisplayRole )
        result = item->setText(value.toString());
    else if (role == QtMessageLogHandler::TypeRole)
        result = item->setItemType((QtMessageLogHandler::ItemType)value.toInt());
    else if (value.canConvert(QVariant::String))
        result = item->setText(value.toString());

    if (result)
        emit dataChanged(index, index);

    return result;
}

bool QtMessageLogHandler::insertRows(int position, int rows, const QModelIndex &parent)
{
    QtMessageLogItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows);
    endInsertRows();

    return success;
}

bool QtMessageLogHandler::removeRows(int position, int rows, const QModelIndex &parent)
{
    QtMessageLogItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

QtMessageLogItem *QtMessageLogHandler::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        QtMessageLogItem *item = static_cast<QtMessageLogItem*>(index.internalPointer());
        if (item) return item;
    }
    return m_rootItem;
}

} //Internal
} //Debugger
