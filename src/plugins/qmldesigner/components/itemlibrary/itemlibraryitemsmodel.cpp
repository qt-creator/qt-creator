/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "itemlibraryitemsmodel.h"
#include "itemlibraryitem.h"
#include <utils/qtcassert.h>
#include <QDebug>
#include <QMetaProperty>

namespace QmlDesigner {

ItemLibraryItemsModel::ItemLibraryItemsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    addRoleNames();
}

ItemLibraryItemsModel::~ItemLibraryItemsModel()
{
}

int ItemLibraryItemsModel::rowCount(const QModelIndex &) const
{
    return m_itemList.count();
}

QVariant ItemLibraryItemsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_itemList.count()) {
        qDebug() << Q_FUNC_INFO << "invalid index requested";
        return {};
    }

    if (m_roleNames.contains(role))
        return m_itemList.at(index.row())->property(m_roleNames.value(role));

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return {};
}

QHash<int, QByteArray> ItemLibraryItemsModel::roleNames() const
{
    return m_roleNames;
}

void ItemLibraryItemsModel::addItem(ItemLibraryItem *element)
{
    m_itemList.append(element);

    element->setVisible(element->isUsable());
}

const QList<QPointer<ItemLibraryItem>> &ItemLibraryItemsModel::items() const
{
    return m_itemList;
}

void ItemLibraryItemsModel::sortItems()
{
    int nullPointerSectionCount = m_itemList.removeAll(QPointer<ItemLibraryItem>());
    QTC_ASSERT(nullPointerSectionCount == 0,;);
    auto itemSort = [](ItemLibraryItem *first, ItemLibraryItem *second) {
        return QString::localeAwareCompare(first->itemName(), second->itemName()) < 0;
    };

    std::sort(m_itemList.begin(), m_itemList.end(), itemSort);
}

void ItemLibraryItemsModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ItemLibraryItemsModel::addRoleNames()
{
    int role = 0;
    const QMetaObject meta = ItemLibraryItem::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(role++, meta.property(i).name());
}

} // namespace QmlDesigner
