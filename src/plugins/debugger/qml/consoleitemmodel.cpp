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

#include "consoleitemmodel.h"

namespace Debugger {
namespace Internal {

class ConsoleItem
 {
 public:
     ConsoleItem(const QString &data = QString(),
                 ConsoleItemModel::ItemType type = ConsoleItemModel::UndefinedType,
                 ConsoleItem *parent = 0);
     ~ConsoleItem();

     ConsoleItem *child(int number);
     int childCount() const;
     QString text() const;
     ConsoleItemModel::ItemType itemType() const;
     bool insertChildren(int position, int count);
     ConsoleItem *parent();
     bool removeChildren(int position, int count);
     bool removeColumns(int position, int columns);
     int childNumber() const;
     bool setData(const QString &text, ConsoleItemModel::ItemType itemType);
     bool setText(const QString &text);
     bool setItemType(ConsoleItemModel::ItemType itemType);

 private:
     QList<ConsoleItem *> m_childItems;
     QString m_text;
     ConsoleItemModel::ItemType m_itemType;
     ConsoleItem *m_parentItem;
 };

///////////////////////////////////////////////////////////////////////
//
// ConsoleItem
//
///////////////////////////////////////////////////////////////////////

ConsoleItem::ConsoleItem(const QString &text,
                         ConsoleItemModel::ItemType itemType,
                         ConsoleItem *parent)
    : m_text(text),
      m_itemType(itemType),
      m_parentItem(parent)

{
}

ConsoleItem::~ConsoleItem()
{
    qDeleteAll(m_childItems);
}

ConsoleItem *ConsoleItem::child(int number)
{
    return m_childItems.value(number);
}

int ConsoleItem::childCount() const
{
    return m_childItems.count();
}

int ConsoleItem::childNumber() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<ConsoleItem *>(this));

    return 0;
}

QString ConsoleItem::text() const
{
    return m_text;
}

ConsoleItemModel::ItemType ConsoleItem::itemType() const
{
    return m_itemType;
}
\
bool ConsoleItem::insertChildren(int position, int count)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        ConsoleItem *item = new ConsoleItem(QString(),
                                            ConsoleItemModel::UndefinedType, this);
        m_childItems.insert(position, item);
    }

    return true;
}

ConsoleItem *ConsoleItem::parent()
{
    return m_parentItem;
}

bool ConsoleItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > m_childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete m_childItems.takeAt(position);

    return true;
}

bool ConsoleItem::removeColumns(int /*position*/, int /*columns*/)
{
    return false;
}

bool ConsoleItem::setData(const QString &text,
                          ConsoleItemModel::ItemType itemType)
{
    m_text = text;
    m_itemType = itemType;
    return true;
}

bool ConsoleItem::setText(const QString &text)
{
    m_text = text;
    return true;
}

bool ConsoleItem::setItemType(ConsoleItemModel::ItemType itemType)
{
    m_itemType = itemType;
    return true;
}

///////////////////////////////////////////////////////////////////////
//
// ConsoleItemModel
//
///////////////////////////////////////////////////////////////////////

ConsoleItemModel::ConsoleItemModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    m_rootItem = new ConsoleItem();
}

ConsoleItemModel::~ConsoleItemModel()
{
    delete m_rootItem;
}

void ConsoleItemModel::clear()
{
    beginResetModel();
    reset();
    delete m_rootItem;
    m_rootItem = new ConsoleItem();
    endResetModel();

    //Insert an empty row
    appendEditableRow();
}

void ConsoleItemModel::appendItem(ItemType itemType, QString message)
{
    int position = m_rootItem->childCount() - 1;
    insertRow(position);
    QModelIndex idx = index(position, 0);
    if (getItem(idx)->setData(message, itemType))
        emit dataChanged(idx, idx);
}

void ConsoleItemModel::appendEditableRow()
{
    int position = m_rootItem->childCount();
    insertRow(position);
    QModelIndex idx = index(position, 0);
    if (getItem(idx)->setData(QString(), ConsoleItemModel::InputType)) {
        QModelIndex idx = index(position, 0);
        emit dataChanged(idx, idx);
        emit editableRowAppended(idx, QItemSelectionModel::ClearAndSelect);
    }
}

///////////////////////////////////////////////////////////////////////
//
// ConsoleEditor
//
///////////////////////////////////////////////////////////////////////

QVariant ConsoleItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    ConsoleItem *item = getItem(index);

    if (role == Qt::DisplayRole )
        return item->text();
    else if (role == ConsoleItemModel::TypeRole)
        return int(item->itemType());
    else
        return QVariant();
}

QModelIndex ConsoleItemModel::index(int row, int column,
                                    const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    if (column > 0)
        return QModelIndex();

    ConsoleItem *parentItem = getItem(parent);

    ConsoleItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ConsoleItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    ConsoleItem *childItem = getItem(index);
    ConsoleItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int ConsoleItemModel::rowCount(const QModelIndex &parent) const
{
    ConsoleItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

int ConsoleItemModel::columnCount(const QModelIndex & /* parent */) const
{
    return 1;
}

Qt::ItemFlags ConsoleItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    ConsoleItem *item = getItem(index);
    if (item->parent() == m_rootItem && index.row() == m_rootItem->childCount() - 1)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ConsoleItemModel::setData(const QModelIndex &index, const QVariant &value,
                               int role)
{
    ConsoleItem *item = getItem(index);
    bool result = false;
    if (role == Qt::DisplayRole )
        result = item->setText(value.toString());
    else if (role == ConsoleItemModel::TypeRole)
        result = item->setItemType((ItemType)value.toInt());
    else if (value.canConvert(QVariant::String))
        result = item->setText(value.toString());

    if (result)
        emit dataChanged(index, index);

    return result;
}

bool ConsoleItemModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    ConsoleItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows);
    endInsertRows();

    return success;
}

bool ConsoleItemModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    ConsoleItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

ConsoleItem *ConsoleItemModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        ConsoleItem *item = static_cast<ConsoleItem*>(index.internalPointer());
        if (item) return item;
    }
    return m_rootItem;
}

} //Internal
} //Debugger
