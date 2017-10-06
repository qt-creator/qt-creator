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

#include "itemlibrarysectionmodel.h"

#include "itemlibraryitem.h"

#include <QDebug>

namespace QmlDesigner {

ItemLibrarySectionModel::ItemLibrarySectionModel(QObject *parent) :
    QAbstractListModel(parent)
{
    addRoleNames();
}

ItemLibrarySectionModel::~ItemLibrarySectionModel()
{
    clearItems();
}

int ItemLibrarySectionModel::rowCount(const QModelIndex &) const
{
    return m_itemList.count();
}

QVariant ItemLibrarySectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() + 1 > m_itemList.count()) {
        qDebug() << Q_FUNC_INFO << "invalid index requested";
        return QVariant();
    }

    if (m_roleNames.contains(role)) {
        QVariant value = m_itemList.at(index.row())->property(m_roleNames.value(role));

        if (ItemLibrarySectionModel* model = qobject_cast<ItemLibrarySectionModel *>(value.value<QObject*>()))
            return QVariant::fromValue(model);

        return m_itemList.at(index.row())->property(m_roleNames.value(role));
    }

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return QVariant();
}

QHash<int, QByteArray> ItemLibrarySectionModel::roleNames() const
{
    return m_roleNames;
}

void ItemLibrarySectionModel::clearItems()
{
    beginResetModel();
    endResetModel();
}

void ItemLibrarySectionModel::addItem(ItemLibraryItem *element)
{
    m_itemList.append(element);

    element->setVisible(true);
}

const QList<ItemLibraryItem *> &ItemLibrarySectionModel::items() const
{
    return m_itemList;
}

void ItemLibrarySectionModel::sortItems()
{
    auto itemSort = [](ItemLibraryItem *first, ItemLibraryItem *second) {
        return QString::localeAwareCompare(first->itemName(), second->itemName()) < 1;
    };

    std::sort(m_itemList.begin(), m_itemList.end(), itemSort);
}

void ItemLibrarySectionModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ItemLibrarySectionModel::addRoleNames()
{
    int role = 0;
    for (int propertyIndex = 0; propertyIndex < ItemLibraryItem::staticMetaObject.propertyCount(); ++propertyIndex) {
        QMetaProperty property = ItemLibraryItem::staticMetaObject.property(propertyIndex);
        m_roleNames.insert(role, property.name());
        ++role;
    }
}

} // namespace QmlDesigner
