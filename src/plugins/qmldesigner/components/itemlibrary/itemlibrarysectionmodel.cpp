/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "itemlibrarysectionmodel.h"

#include "itemlibraryitem.h"

namespace QmlDesigner {

ItemLibrarySectionModel::ItemLibrarySectionModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

ItemLibrarySectionModel::~ItemLibrarySectionModel()
{
    clearItems();
}

int ItemLibrarySectionModel::rowCount(const QModelIndex &) const
{
    return m_privList.count();
}

QVariant ItemLibrarySectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row()+1 > m_privList.count()) {
        qDebug() << Q_FUNC_INFO << "invalid index requested";
        return QVariant();
    }

    if (m_roleNames.contains(role)) {
        QVariant value = m_privList.at(index.row())->property(m_roleNames.value(role));

        if (ItemLibrarySectionModel* model = qobject_cast<ItemLibrarySectionModel *>(value.value<QObject*>()))
            return QVariant::fromValue(model);

        return m_privList.at(index.row())->property(m_roleNames.value(role));
    }

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return QVariant();
}

void ItemLibrarySectionModel::clearItems()
{
    beginResetModel();
    while (m_itemOrder.count() > 0)
        removeItem(m_itemOrder.at(0).libId);
    endResetModel();
}

static bool compareFunction(QObject *first, QObject *second)
{
    static const char sortRoleName[] = "sortingRole";

    return first->property(sortRoleName).toString() < second->property(sortRoleName).toString();
}

void ItemLibrarySectionModel::addItem(ItemLibraryItem *element, int libId)
{
    struct order_struct orderEntry;
    orderEntry.libId = libId;
    orderEntry.visible = false;

    int pos = 0;
    while ((pos < m_itemOrder.count()) &&
           compareFunction(m_itemModels.value(m_itemOrder.at(pos).libId), element))
        ++pos;

    m_itemModels.insert(libId, element);
    m_itemOrder.insert(pos, orderEntry);

    setItemVisible(libId, true);
}

void ItemLibrarySectionModel::removeItem(int libId)
{
    QObject *element = m_itemModels.value(libId);
    int pos = findItem(libId);

    setItemVisible(libId, false);

    m_itemModels.remove(libId);
    m_itemOrder.removeAt(pos);

    delete element;
}

bool ItemLibrarySectionModel::itemVisible(int libId) const
{
    int pos = findItem(libId);
    return m_itemOrder.at(pos).visible;
}

bool ItemLibrarySectionModel::setItemVisible(int libId, bool visible)
{
    int pos = findItem(libId);
    if (m_itemOrder.at(pos).visible == visible)
        return false;

    int visiblePos = visibleItemPosition(libId);
    if (visible)
        privateInsert(visiblePos, (m_itemModels.value(libId)));
    else
        privateRemove(visiblePos);

    m_itemOrder[pos].visible = visible;
    return true;
}

void ItemLibrarySectionModel::privateInsert(int pos, QObject *element)
{
    QObject *object = element;

    for (int i = 0; i < object->metaObject()->propertyCount(); ++i) {
        QMetaProperty property = object->metaObject()->property(i);
        addRoleName(property.name());
    }

    m_privList.insert(pos, element);
}

void ItemLibrarySectionModel::privateRemove(int pos)
{
    m_privList.removeAt(pos);
}

const QMap<int, ItemLibraryItem*> &ItemLibrarySectionModel::items() const
{
    return m_itemModels;
}

const QList<ItemLibraryItem *> ItemLibrarySectionModel::itemList() const
{
    return m_itemModels.values();
}

ItemLibraryItem *ItemLibrarySectionModel::item(int libId)
{
    return m_itemModels.value(libId);
}

int ItemLibrarySectionModel::findItem(int libId) const
{
    int i = 0;
    QListIterator<struct order_struct> it(m_itemOrder);

    while (it.hasNext()) {
        if (it.next().libId == libId)
            return i;
        ++i;
    }

    return -1;
}

int ItemLibrarySectionModel::visibleItemPosition(int libId) const
{
    int i = 0;
    QListIterator<struct order_struct> it(m_itemOrder);

    while (it.hasNext()) {
        struct order_struct order = it.next();
        if (order.libId == libId)
            return i;
        if (order.visible)
            ++i;
    }

    return -1;
}

void ItemLibrarySectionModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ItemLibrarySectionModel::addRoleName(const QByteArray &roleName)
{
    if (m_roleNames.values().contains(roleName))
        return;

    int key = m_roleNames.count();
    m_roleNames.insert(key, roleName);
    setRoleNames(m_roleNames);
}

} // namespace QmlDesigner
