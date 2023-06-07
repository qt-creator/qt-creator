// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    return m_itemList.size();
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
