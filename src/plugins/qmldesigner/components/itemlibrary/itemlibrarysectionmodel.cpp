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

#include "itemlibraryitemmodel.h"

namespace QmlDesigner {

ItemLibrarySortedModel::ItemLibrarySortedModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

ItemLibrarySortedModel::~ItemLibrarySortedModel()
{
    clearElements();
}

int ItemLibrarySortedModel::rowCount(const QModelIndex &) const
{
    return m_privList.count();
}

QVariant ItemLibrarySortedModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row()+1 > m_privList.count()) {
        qDebug() << Q_FUNC_INFO << "invalid index requested";
        return QVariant();
    }

    if (m_roleNames.contains(role)) {
        QVariant value = m_privList.at(index.row())->property(m_roleNames.value(role));

        if (ItemLibrarySortedModel* model = qobject_cast<ItemLibrarySortedModel *>(value.value<QObject*>()))
            return QVariant::fromValue(model);

        return m_privList.at(index.row())->property(m_roleNames.value(role));
    }

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return QVariant();
}

void ItemLibrarySortedModel::clearElements()
{
    beginResetModel();
    while (m_elementOrder.count() > 0)
        removeElement(m_elementOrder.at(0).libId);
    endResetModel();
}

static bool compareFunction(QObject *first, QObject *second)
{
    static const char sortRoleName[] = "sortingRole";

    return first->property(sortRoleName).toString() < second->property(sortRoleName).toString();
}

void ItemLibrarySortedModel::addElement(QObject *element, int libId)
{
    struct order_struct orderEntry;
    orderEntry.libId = libId;
    orderEntry.visible = false;

    int pos = 0;
    while ((pos < m_elementOrder.count()) &&
           compareFunction(m_elementModels.value(m_elementOrder.at(pos).libId), element))
        ++pos;

    m_elementModels.insert(libId, element);
    m_elementOrder.insert(pos, orderEntry);

    setElementVisible(libId, true);
}

void ItemLibrarySortedModel::removeElement(int libId)
{
    QObject *element = m_elementModels.value(libId);
    int pos = findElement(libId);

    setElementVisible(libId, false);

    m_elementModels.remove(libId);
    m_elementOrder.removeAt(pos);

    delete element;
}

bool ItemLibrarySortedModel::elementVisible(int libId) const
{
    int pos = findElement(libId);
    return m_elementOrder.at(pos).visible;
}

bool ItemLibrarySortedModel::setElementVisible(int libId, bool visible)
{
    int pos = findElement(libId);
    if (m_elementOrder.at(pos).visible == visible)
        return false;

    int visiblePos = visibleElementPosition(libId);
    if (visible)
        privateInsert(visiblePos, (m_elementModels.value(libId)));
    else
        privateRemove(visiblePos);

    m_elementOrder[pos].visible = visible;
    return true;
}

void ItemLibrarySortedModel::privateInsert(int pos, QObject *element)
{
    QObject *object = element;

    for (int i = 0; i < object->metaObject()->propertyCount(); ++i) {
        QMetaProperty property = object->metaObject()->property(i);
        addRoleName(property.name());
    }

    m_privList.insert(pos, element);
}

void ItemLibrarySortedModel::privateRemove(int pos)
{
    m_privList.removeAt(pos);
}

const QMap<int, QObject *> &ItemLibrarySortedModel::elements() const
{
    return m_elementModels;
}

template<typename T>
const QList<T> ItemLibrarySortedModel::elementsByType() const
{
    QList<T> objectList;

    foreach (QObject *item, elements()) {
        T object = qobject_cast<T>(item);
        if (object)
            objectList.append(object);
    }

    return objectList;
}

QObject *ItemLibrarySortedModel::element(int libId)
{
    return m_elementModels.value(libId);
}

template<typename T>
T ItemLibrarySortedModel::elementByType(int libId)
{
    return qobject_cast<T>(element(libId));
}

int ItemLibrarySortedModel::findElement(int libId) const
{
    int i = 0;
    QListIterator<struct order_struct> it(m_elementOrder);

    while (it.hasNext()) {
        if (it.next().libId == libId)
            return i;
        ++i;
    }

    return -1;
}

int ItemLibrarySortedModel::visibleElementPosition(int libId) const
{
    int i = 0;
    QListIterator<struct order_struct> it(m_elementOrder);

    while (it.hasNext()) {
        struct order_struct order = it.next();
        if (order.libId == libId)
            return i;
        if (order.visible)
            ++i;
    }

    return -1;
}

void ItemLibrarySortedModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ItemLibrarySortedModel::addRoleName(const QByteArray &roleName)
{
    if (m_roleNames.values().contains(roleName))
        return;

    int key = m_roleNames.count();
    m_roleNames.insert(key, roleName);
    setRoleNames(m_roleNames);
}

ItemLibrarySectionModel::ItemLibrarySectionModel(int sectionLibId, const QString &sectionName, QObject *parent)
    : QObject(parent),
      m_name(sectionName),
      m_sectionLibraryId(sectionLibId),
      m_sectionExpanded(true),
      m_sectionEntries(parent),
      m_isVisible(false)
{
//    if (collapsedStateHash.contains(sectionName))
//        m_sectionExpanded=  collapsedStateHash.value(sectionName);
}


QString ItemLibrarySectionModel::sectionName() const
{
    return m_name;
}

int ItemLibrarySectionModel::sectionLibraryId() const
{
    return m_sectionLibraryId;
}

bool ItemLibrarySectionModel::sectionExpanded() const
{
    return m_sectionExpanded;
}

QVariant ItemLibrarySectionModel::sortingRole() const
{

    if (sectionName() == QStringLiteral("QML Components")) //Qml Components always come first
        return QVariant(QStringLiteral("AA.this_comes_first"));

    return sectionName();
}

void ItemLibrarySectionModel::addSectionEntry(ItemLibraryItemModel *sectionEntry)
{
    m_sectionEntries.addElement(sectionEntry, sectionEntry->itemLibId());
}


void ItemLibrarySectionModel::removeSectionEntry(int itemLibId)
{
    m_sectionEntries.removeElement(itemLibId);
}

QObject *ItemLibrarySectionModel::sectionEntries()
{
    return &m_sectionEntries;
}

int ItemLibrarySectionModel::visibleItemIndex(int itemLibId)
{
    return m_sectionEntries.visibleElementPosition(itemLibId);
}


bool ItemLibrarySectionModel::isItemVisible(int itemLibId)
{
    return m_sectionEntries.elementVisible(itemLibId);
}


bool ItemLibrarySectionModel::updateSectionVisibility(const QString &searchText, bool *changed)
{
    bool haveVisibleItems = false;

    *changed = false;

    QMap<int, QObject *>::const_iterator itemIt = m_sectionEntries.elements().constBegin();
    while (itemIt != m_sectionEntries.elements().constEnd()) {

        bool itemVisible = m_sectionEntries.elementByType<ItemLibraryItemModel*>(
                    itemIt.key())->itemName().toLower().contains(searchText);

        bool itemChanged = false;
        itemChanged = m_sectionEntries.setElementVisible(itemIt.key(), itemVisible);

        *changed |= itemChanged;

        if (itemVisible)
            haveVisibleItems = true;

        ++itemIt;
    }

    m_sectionEntries.resetModel();

    emit sectionEntriesChanged();

    return haveVisibleItems;
}


void ItemLibrarySectionModel::updateItemIconSize(const QSize &itemIconSize)
{
//    foreach (ItemLibraryItemModel* itemLibraryItemModel, m_sectionEntries.elementsByType<ItemLibraryItemModel*>()) {
//        itemLibraryItemModel->setItemIconSize(itemIconSize);
    //    }
}

bool ItemLibrarySectionModel::setVisible(bool isVisible)
{
    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        return true;
    }

    return false;
}

bool ItemLibrarySectionModel::isVisible() const
{
    return m_isVisible;
}

} // namespace QmlDesigner
