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

#include "itemlibrarymodel.h"
#include "itemlibraryinfo.h"
#include "itemlibrarysectionmodel.h"

#include <model.h>
#include <nodemetainfo.h>

#include <QVariant>
#include <QMetaProperty>
#include <QMimeData>
#include <QPainter>
#include <QPen>
#include <qdebug.h>

static bool inline registerItemLibrarySortedModel() {
    qmlRegisterType<QmlDesigner::ItemLibrarySortedModel>();
    return true;
}

namespace QmlDesigner {

static QHash<QString, bool> collapsedStateHash;

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

ItemLibraryItemModel::ItemLibraryItemModel(int itemLibId, const QString &itemName, QObject *parent)
    : QObject(parent),
      m_libId(itemLibId),
      m_name(itemName),
      m_iconSize(64, 64)
{

}


ItemLibraryItemModel::~ItemLibraryItemModel()
{

}


int ItemLibraryItemModel::itemLibId() const
{
    return m_libId;
}


QString ItemLibraryItemModel::itemName() const
{
    return m_name;
}

QString ItemLibraryItemModel::itemLibraryIconPath() const
{
    //Prepend image provider prefix
    return QStringLiteral("image://qmldesigner_itemlibrary/")+ m_iconPath;
}

QVariant ItemLibraryItemModel::sortingRole() const
{
    return itemName();
}

void ItemLibraryItemModel::setItemIconPath(const QString &iconPath)
{
    m_iconPath = iconPath;
}

void ItemLibraryItemModel::setItemIconSize(const QSize &itemIconSize)
{
    m_iconSize = itemIconSize;
    setItemIconPath(m_iconPath);
}


void ItemLibraryModel::setExpanded(bool expanded, const QString &section)
{
    if (collapsedStateHash.contains(section))
        collapsedStateHash.remove(section);

    if (!expanded) //default is true
        collapsedStateHash.insert(section, expanded);
}

ItemLibraryModel::ItemLibraryModel(QObject *parent)
    : ItemLibrarySortedModel(parent),
      m_searchText(""),
      m_itemIconSize(64, 64),
      m_nextLibId(0)
{
}

ItemLibraryModel::~ItemLibraryModel()
{
}


QString ItemLibraryModel::searchText() const
{
    return m_searchText;
}


void ItemLibraryModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText != lowerSearchText) {
        m_searchText = lowerSearchText;
        emit searchTextChanged();

        updateVisibility();
    }
}


void ItemLibraryModel::setItemIconSize(const QSize &itemIconSize)
{
    m_itemIconSize = itemIconSize;

    foreach (ItemLibrarySectionModel* itemLibrarySectionModel, sections()) {
        itemLibrarySectionModel->updateItemIconSize(itemIconSize);
    }
}


int ItemLibraryModel::getItemSectionIndex(int itemLibId)
{
    if (m_sections.contains(itemLibId))
        return section(m_sections.value(itemLibId))->visibleItemIndex(itemLibId);
    else
        return -1;
}


int ItemLibraryModel::getSectionLibId(int itemLibId)
{
    return m_sections.value(itemLibId);
}


bool ItemLibraryModel::isItemVisible(int itemLibId)
{
    if (!m_sections.contains(itemLibId))
        return false;

    int sectionLibId = m_sections.value(itemLibId);
    if (!elementVisible(sectionLibId))
        return false;

    return section(sectionLibId)->isItemVisible(itemLibId);
}

Import entryToImport(const ItemLibraryEntry &entry)
{
    if (entry.majorVersion() == -1 && entry.minorVersion() == -1)
        return Import::createFileImport(entry.requiredImport());

    return Import::createLibraryImport(entry.requiredImport(), QString::number(entry.majorVersion()) + QLatin1Char('.') +
                                                               QString::number(entry.minorVersion()));

}

void ItemLibraryModel::update(ItemLibraryInfo *itemLibraryInfo, Model *model)
{
    if (!model)
        return;

    QMap<QString, int> sections;

    clearElements();
    m_itemInfos.clear();
    m_sections.clear();
    m_nextLibId = 0;

    QStringList imports;
    foreach (const Import &import, model->imports())
        if (import.isLibraryImport())
            imports << import.url() + QLatin1Char(' ') + import.version();

    foreach (ItemLibraryEntry entry, itemLibraryInfo->entries()) {

         NodeMetaInfo metaInfo = model->metaInfo(entry.typeName(), -1, -1);
         bool valid = metaInfo.isValid() && metaInfo.majorVersion() == entry.majorVersion();

         if (valid
                 && (entry.requiredImport().isEmpty()
                     || model->hasImport(entryToImport(entry), true, true))) {
            QString itemSectionName = entry.category();
            ItemLibrarySectionModel *sectionModel;
            ItemLibraryItemModel *itemModel;
            int itemId = m_nextLibId++, sectionId;

            if (sections.contains(itemSectionName)) {
                sectionId = sections.value(itemSectionName);
                sectionModel = section(sectionId);
            } else {
                sectionId = m_nextLibId++;
                sectionModel = new ItemLibrarySectionModel(sectionId, itemSectionName, this);
                addElement(sectionModel, sectionId);
                sections.insert(itemSectionName, sectionId);
            }

            m_itemInfos.insert(itemId, entry);

            itemModel = new ItemLibraryItemModel(itemId, entry.name(), sectionModel);

            // delayed creation of (default) icons
            if (entry.iconPath().isEmpty())
                entry.setIconPath(QStringLiteral(":/ItemLibrary/images/item-default-icon.png"));
            if (entry.dragIcon().isNull())
                entry.setDragIcon(createDragPixmap(getWidth(entry), getHeight(entry)));

            itemModel->setItemIconPath(entry.iconPath());
            itemModel->setItemIconSize(m_itemIconSize);
            sectionModel->addSectionEntry(itemModel);
            m_sections.insert(itemId, sectionId);
        }
    }

    updateVisibility();
}


QString ItemLibraryModel::getTypeName(int libId)
{
    return m_itemInfos.value(libId).typeName();
}


QMimeData *ItemLibraryModel::getMimeData(int libId)
{
    QMimeData *mimeData = new QMimeData();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << m_itemInfos.value(libId);
    mimeData->setData(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo"), data);

    const QIcon icon = m_itemInfos.value(libId).dragIcon();
    if (!icon.isNull()) {
        const QList<QSize> sizes = icon.availableSizes();
        if (!sizes.isEmpty())
            mimeData->setImageData(icon.pixmap(sizes.front()).toImage());
    }

    mimeData->removeFormat(QStringLiteral("text/plain"));

    return mimeData;
}


QIcon ItemLibraryModel::getIcon(int libId)
{
    return m_itemInfos.value(libId).icon();
}

ItemLibrarySectionModel *ItemLibraryModel::section(int libId)
{
    return elementByType<ItemLibrarySectionModel*>(libId);
}

QList<ItemLibrarySectionModel *> ItemLibraryModel::sections() const
{
    return elementsByType<ItemLibrarySectionModel*>();
}

void ItemLibraryModel::updateVisibility()
{
    beginResetModel();
    endResetModel();
    bool changed = false;

    QMap<int, QObject *>::const_iterator sectionIt = elements().constBegin();
    while (sectionIt != elements().constEnd()) {
        ItemLibrarySectionModel *sectionModel = section(sectionIt.key());

        QString sectionSearchText = m_searchText;

        if (sectionModel->sectionName().toLower().contains(m_searchText))
            sectionSearchText = "";

        bool sectionChanged = false,
            sectionVisibility = sectionModel->updateSectionVisibility(sectionSearchText,
                                                                      &sectionChanged);
        if (sectionChanged) {
            changed = true;
            if (sectionVisibility)
                emit sectionVisibilityChanged(sectionIt.key());
        }

        changed |= setElementVisible(sectionIt.key(), sectionVisibility);
        ++sectionIt;
    }

    if (changed)
        emit visibilityChanged();
}

int ItemLibraryModel::getWidth(const ItemLibraryEntry &itemLibraryEntry)
{
    foreach (const ItemLibraryEntry::Property &property, itemLibraryEntry.properties())
    {
        if (property.name() == "width")
            return property.value().toInt();
    }
    return 64;
}

int ItemLibraryModel::getHeight(const ItemLibraryEntry &itemLibraryEntry)
{
    foreach (const ItemLibraryEntry::Property &property, itemLibraryEntry.properties())
    {
        if (property.name() == "height")
            return property.value().toInt();
    }
    return 64;
}

QPixmap ItemLibraryModel::createDragPixmap(int , int )
{
    QImage dragImage(10, 10, QImage::Format_ARGB32); // TODO: draw item drag icon
    dragImage.fill(0x00ffffff); //### todo for now we disable the preview image
    QPainter p(&dragImage);
    QPen pen(Qt::gray);
//    pen.setWidth(2);
//    p.setPen(pen);
//    p.drawRect(1, 1, dragImage.width() - 2, dragImage.height() - 2);
    return QPixmap::fromImage(dragImage);
}

void registerQmlTypes()
{
    registerItemLibrarySortedModel();
}

} // namespace QmlDesigner

