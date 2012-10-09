/**************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlconsoleitemmodel.h"

#include <utils/qtcassert.h>

#include <QFontMetrics>

namespace QmlJSTools {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// QmlConsoleItemModel
//
///////////////////////////////////////////////////////////////////////

QmlConsoleItemModel::QmlConsoleItemModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_hasEditableRow(false),
    m_rootItem(new QmlConsoleItem(0)),
    m_maxSizeOfFileName(0)
{
}

QmlConsoleItemModel::~QmlConsoleItemModel()
{
    delete m_rootItem;
}

void QmlConsoleItemModel::clear()
{
    beginResetModel();
    reset();
    delete m_rootItem;
    m_rootItem = new QmlConsoleItem(0);
    endResetModel();

    if (m_hasEditableRow)
        appendEditableRow();
}

bool QmlConsoleItemModel::appendItem(QmlConsoleItem *item, int position)
{
    if (position < 0)
        position = m_rootItem->childCount() - 1;

    if (position < 0)
        position = 0;

    beginInsertRows(QModelIndex(), position, position);
    bool success = m_rootItem->insertChild(position, item);
    endInsertRows();

    return success;
}

bool QmlConsoleItemModel::appendMessage(QmlConsoleItem::ItemType itemType,
                                        const QString &message, int position)
{
    return appendItem(new QmlConsoleItem(m_rootItem, itemType, message), position);
}

void QmlConsoleItemModel::setHasEditableRow(bool hasEditableRow)
{
    if (m_hasEditableRow && !hasEditableRow)
        removeEditableRow();

    if (!m_hasEditableRow && hasEditableRow)
        appendEditableRow();

    m_hasEditableRow = hasEditableRow;
}

bool QmlConsoleItemModel::hasEditableRow() const
{
    return m_hasEditableRow;
}

void QmlConsoleItemModel::appendEditableRow()
{
    int position = m_rootItem->childCount();
    if (appendItem(new QmlConsoleItem(m_rootItem, QmlConsoleItem::InputType), position))
        emit selectEditableRow(index(position, 0), QItemSelectionModel::ClearAndSelect);
}

void QmlConsoleItemModel::removeEditableRow()
{
    if (m_rootItem->child(m_rootItem->childCount() - 1)->itemType == QmlConsoleItem::InputType)
        removeRow(m_rootItem->childCount() - 1);
}

int QmlConsoleItemModel::sizeOfFile(const QFont &font)
{
    int lastReadOnlyRow = m_rootItem->childCount();
    if (m_hasEditableRow)
        lastReadOnlyRow -= 2;
    else
        lastReadOnlyRow -= 1;
    if (lastReadOnlyRow < 0)
        return 0;
    QString filename = m_rootItem->child(lastReadOnlyRow)->file;
    const int pos = filename.lastIndexOf(QLatin1Char('/'));
    if (pos != -1)
        filename = filename.mid(pos + 1);

    QFontMetrics fm(font);
    m_maxSizeOfFileName = qMax(m_maxSizeOfFileName, fm.width(filename));

    return m_maxSizeOfFileName;
}

int QmlConsoleItemModel::sizeOfLineNumber(const QFont &font)
{
    QFontMetrics fm(font);
    return fm.width(QLatin1String("88888"));
}

QVariant QmlConsoleItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QmlConsoleItem *item = getItem(index);

    if (role == Qt::DisplayRole )
        return item->text();
    else if (role == QmlConsoleItemModel::TypeRole)
        return int(item->itemType);
    else if (role == QmlConsoleItemModel::FileRole)
        return item->file;
    else if (role == QmlConsoleItemModel::LineRole)
        return item->line;
    else
        return QVariant();
}

QModelIndex QmlConsoleItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    if (column > 0)
        return QModelIndex();

    QmlConsoleItem *parentItem = getItem(parent);

    QmlConsoleItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex QmlConsoleItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QmlConsoleItem *childItem = getItem(index);
    QmlConsoleItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    if (!parentItem)
        return QModelIndex();
    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int QmlConsoleItemModel::rowCount(const QModelIndex &parent) const
{
    QmlConsoleItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

int QmlConsoleItemModel::columnCount(const QModelIndex & /* parent */) const
{
    return 1;
}

Qt::ItemFlags QmlConsoleItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    QmlConsoleItem *item = getItem(index);
    if (m_hasEditableRow && item->parent() == m_rootItem
            && index.row() == m_rootItem->childCount() - 1)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool QmlConsoleItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QmlConsoleItem *item = getItem(index);
    bool result = false;
    if (role == Qt::DisplayRole) {
        item->setText(value.toString());
        result = true;
    } else if (role == QmlConsoleItemModel::TypeRole) {
        item->itemType = (QmlConsoleItem::ItemType)value.toInt();
        result = true;
    } else if (role == QmlConsoleItemModel::FileRole) {
        item->file = value.toString();
        result = true;
    } else if (role == QmlConsoleItemModel::LineRole) {
        item->line = value.toInt();
        result = true;
    }

    if (result)
        emit dataChanged(index, index);

    return result;
}

bool QmlConsoleItemModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    QmlConsoleItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows);
    endInsertRows();

    return success;
}

bool QmlConsoleItemModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    QmlConsoleItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

QmlConsoleItem *QmlConsoleItemModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        QmlConsoleItem *item = static_cast<QmlConsoleItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return m_rootItem;
}

} // Internal
} // QmlJSTools
