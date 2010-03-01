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

#include "itemlibrarymodel.h"
#include "metainfo.h"

#include <QVariant>
#include <QMimeData>
#include <QPainter>
#include <QPen>
#include <qdebug.h>


namespace QmlDesigner {

namespace Internal {


template <class T>
ItemLibrarySortedModel<T>::ItemLibrarySortedModel(QObject *parent) :
    QDeclarativeListModel(parent)
{
}


template <class T>
ItemLibrarySortedModel<T>::~ItemLibrarySortedModel()
{
    clearElements();
}


template <class T>
void ItemLibrarySortedModel<T>::clearElements()
{
    while (m_elementOrder.count() > 0)
        removeElement(m_elementOrder.at(0).libId);
}


template <class T>
void ItemLibrarySortedModel<T>::addElement(T *element, int libId)
{
    struct order_struct orderEntry;
    orderEntry.libId = libId;
    orderEntry.visible = false;

    int pos = 0;
    while ((pos < m_elementOrder.count()) &&
           (*(m_elementModels.value(m_elementOrder.at(pos).libId)) < *element))
        ++pos;

    m_elementModels.insert(libId, element);
    m_elementOrder.insert(pos, orderEntry);

    setElementVisible(libId, true);
}

template <class T>
void ItemLibrarySortedModel<T>::removeElement(int libId)
{
    T *element = m_elementModels.value(libId);
    int pos = findElement(libId);
    struct order_struct orderEntry = m_elementOrder.at(pos);

    setElementVisible(libId, false);

    m_elementModels.remove(libId);
    m_elementOrder.removeAt(pos);

    delete element;
}


template <class T>
bool ItemLibrarySortedModel<T>::elementVisible(int libId) const
{
    int pos = findElement(libId);
    return m_elementOrder.at(pos).visible;
}


template <class T>
void ItemLibrarySortedModel<T>::setElementVisible(int libId, bool visible)
{
    int pos = findElement(libId),
          offset = 0;

    if (m_elementOrder.at(pos).visible == visible)
        return;

    for (int i = 0; (i + offset) < pos;) {
        if (m_elementOrder.at(i + offset).visible)
            ++i;
        else
            ++offset;
    }

    if (visible)
        insert(pos - offset, *(m_elementModels.value(libId)));
    else
        remove(pos - offset);

    m_elementOrder[pos].visible = visible;
}


template <class T>
const QMap<int, T *> &ItemLibrarySortedModel<T>::elements() const
{
    return m_elementModels;
}


template <class T>
T *ItemLibrarySortedModel<T>::elementModel(int libId)
{
    return m_elementModels.value(libId);
}


template <class T>
int ItemLibrarySortedModel<T>::findElement(int libId) const
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




ItemLibraryItemModel::ItemLibraryItemModel(QScriptEngine *scriptEngine, int itemLibId, const QString &itemName)
    : QScriptValue(scriptEngine->newObject()),
      m_scriptEngine(scriptEngine),
      m_libId(itemLibId),
      m_name(itemName),
      m_icon(),
      m_iconSize(64, 64)
{
    QScriptValue pixmapScriptValue(m_scriptEngine->newVariant(QPixmap()));

    setProperty(QLatin1String("itemLibId"), itemLibId);
    setProperty(QLatin1String("itemName"), itemName);
    setProperty(QLatin1String("itemPixmap"), pixmapScriptValue);
}


ItemLibraryItemModel::~ItemLibraryItemModel()
{
    setProperty(QLatin1String("itemPixmap"), QVariant::Invalid);
}


int ItemLibraryItemModel::itemLibId() const
{
    return m_libId;
}


QString ItemLibraryItemModel::itemName() const
{
    return m_name;
}


void ItemLibraryItemModel::setItemIcon(const QIcon &itemIcon)
{
    m_icon = itemIcon;

    QScriptValue pixmapScriptValue(m_scriptEngine->newVariant(m_icon.pixmap(m_iconSize)));
    setProperty(QLatin1String("itemPixmap"), pixmapScriptValue);
}


void ItemLibraryItemModel::setItemIconSize(const QSize &itemIconSize)
{
    m_iconSize = itemIconSize;
//    qDebug() << "set icon size" << itemIconSize;
    setItemIcon(m_icon);
}


bool ItemLibraryItemModel::operator<(const ItemLibraryItemModel &other) const
{
    return itemName() < other.itemName();
}




ItemLibrarySectionModel::ItemLibrarySectionModel(QScriptEngine *scriptEngine, int sectionLibId, const QString &sectionName, QObject *parent)
    : QScriptValue(scriptEngine->newObject()),
      m_name(sectionName),
      m_sectionEntries(parent)
{
    QScriptValue::setProperty(QLatin1String("sectionLibId"), sectionLibId);
    QScriptValue::setProperty(QLatin1String("sectionName"), sectionName);
    QScriptValue::setProperty(QLatin1String("sectionEntries"),
        scriptEngine->newVariant(QVariant::fromValue(static_cast<QDeclarativeListModel *>(&m_sectionEntries))));
}


QString ItemLibrarySectionModel::sectionName() const
{
    return m_name;
}


void ItemLibrarySectionModel::addSectionEntry(ItemLibraryItemModel *sectionEntry)
{
    m_sectionEntries.addElement(sectionEntry, sectionEntry->itemLibId());
}


void ItemLibrarySectionModel::removeSectionEntry(int itemLibId)
{
    m_sectionEntries.removeElement(itemLibId);
}


bool ItemLibrarySectionModel::updateSectionVisibility(const QString &searchText)
{
    bool haveVisibleItems = false;
    QMap<int, ItemLibraryItemModel *>::const_iterator itemIt = m_sectionEntries.elements().constBegin();
    while (itemIt != m_sectionEntries.elements().constEnd()) {

        bool itemVisible = itemIt.value()->itemName().toLower().contains(searchText);
        m_sectionEntries.setElementVisible(itemIt.key(), itemVisible);

        if (itemVisible)
            haveVisibleItems = true;

        ++itemIt;
    }

    return haveVisibleItems;
}


void ItemLibrarySectionModel::updateItemIconSize(const QSize &itemIconSize)
{
    foreach (ItemLibraryItemModel *item, m_sectionEntries.elements().values()) {
        item->setItemIconSize(itemIconSize);
    }
}


bool ItemLibrarySectionModel::operator<(const ItemLibrarySectionModel &other) const
{
    return sectionName() < other.sectionName();
}





ItemLibraryModel::ItemLibraryModel(QScriptEngine *scriptEngine, QObject *parent)
    : ItemLibrarySortedModel<ItemLibrarySectionModel>(parent),
      m_scriptEngine(scriptEngine),
      m_metaInfo(0),
      m_searchText(""),
      m_itemIconSize(64, 64),
      m_nextLibId(0)
{
}


ItemLibraryModel::~ItemLibraryModel()
{
    if (m_metaInfo)
        delete m_metaInfo;
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

    foreach (ItemLibrarySectionModel *section, elements().values())
        section->updateItemIconSize(itemIconSize);
}


void ItemLibraryModel::update(const MetaInfo &metaInfo)
{
    QMap<QString, int> sections;

    clearElements();
    m_itemInfos.clear();

    if (!m_metaInfo) {
        m_metaInfo = new MetaInfo(metaInfo);
    } else {
        *m_metaInfo = metaInfo;
    }

    foreach (const QString &type, metaInfo.itemLibraryItems()) {
        foreach (const ItemLibraryInfo &itemLibraryRepresentation, itemLibraryRepresentations(type)) {

            QString itemSectionName = itemLibraryRepresentation.category();
            ItemLibrarySectionModel *sectionModel;
            ItemLibraryItemModel *itemModel;
            int itemId = m_nextLibId++, sectionId;

            if (sections.contains(itemSectionName)) {
                sectionId = sections.value(itemSectionName);
                sectionModel = elementModel(sectionId);
            } else {
                sectionId = m_nextLibId++;
                sectionModel = new ItemLibrarySectionModel(m_scriptEngine.data(), sectionId, itemSectionName, this);
                addElement(sectionModel, sectionId);
                sections.insert(itemSectionName, sectionId);
            }

            m_itemInfos.insert(itemId, itemLibraryRepresentation);

            itemModel = new ItemLibraryItemModel(m_scriptEngine.data(), itemId, itemLibraryRepresentation.name());
            itemModel->setItemIcon(itemLibraryRepresentation.icon());
            itemModel->setItemIconSize(m_itemIconSize);
            sectionModel->addSectionEntry(itemModel);
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
    mimeData->setData(QLatin1String("application/vnd.bauhaus.itemlibraryinfo"), data);

    const QIcon icon = m_itemInfos.value(libId).dragIcon();
    if (!icon.isNull()) {
        const QList<QSize> sizes = icon.availableSizes();
        if (!sizes.isEmpty())
            mimeData->setImageData(icon.pixmap(sizes.front()).toImage());
    }

    mimeData->removeFormat(QLatin1String("text/plain"));

    return mimeData;
}


QIcon ItemLibraryModel::getIcon(int libId)
{
    return m_itemInfos.value(libId).icon();
}


void ItemLibraryModel::updateVisibility()
{
    QMap<int, ItemLibrarySectionModel *>::const_iterator sectionIt = elements().constBegin();
    while (sectionIt != elements().constEnd()) {

        ItemLibrarySectionModel *sectionModel = sectionIt.value();
        QString sectionSearchText = m_searchText;

        if (sectionModel->sectionName().toLower().contains(m_searchText))
            sectionSearchText = "";

        bool sectionVisibility = sectionModel->updateSectionVisibility(sectionSearchText);
        setElementVisible(sectionIt.key(), sectionVisibility);

        ++sectionIt;
    }

    emit visibilityUpdated();
}


QList<ItemLibraryInfo> ItemLibraryModel::itemLibraryRepresentations(const QString &type)
{
    NodeMetaInfo nodeInfo = m_metaInfo->nodeMetaInfo(type);
    QList<ItemLibraryInfo> itemLibraryRepresentationList = m_metaInfo->itemLibraryRepresentations(nodeInfo);

    QImage dragImage(64, 64, QImage::Format_RGB32); // TODO: draw item drag icon
    dragImage.fill(0xffffffff);
    QPainter p(&dragImage);
    QPen pen(Qt::gray);
    pen.setWidth(2);
    p.setPen(pen);
    p.drawRect(1, 1, dragImage.width() - 2, dragImage.height() - 2);
    QPixmap dragPixmap(QPixmap::fromImage(dragImage));

    if (!m_metaInfo->hasNodeMetaInfo(type))
        qWarning() << "ItemLibrary: type not declared: " << type;

    static QIcon defaultIcon(QLatin1String(":/ItemLibrary/images/item-default-icon.png"));

    if (itemLibraryRepresentationList.isEmpty() || !m_metaInfo->hasNodeMetaInfo(type)) {
        QIcon icon = nodeInfo.icon();
        if (icon.isNull())
            icon = defaultIcon;

        ItemLibraryInfo itemLibraryInfo;
        itemLibraryInfo.setName(type);
        itemLibraryInfo.setTypeName(nodeInfo.typeName());
        itemLibraryInfo.setCategory(nodeInfo.category());
        itemLibraryInfo.setIcon(icon);
        itemLibraryInfo.setDragIcon(dragPixmap);
        itemLibraryInfo.setMajorVersion(nodeInfo.majorVersion());
        itemLibraryInfo.setMinorVersion(nodeInfo.minorVersion());
        itemLibraryRepresentationList.append(itemLibraryInfo);
    }
    else {
        foreach (ItemLibraryInfo itemLibraryRepresentation, itemLibraryRepresentationList) {
            QIcon icon = itemLibraryRepresentation.icon();
            if (itemLibraryRepresentation.icon().isNull())
                itemLibraryRepresentation.setIcon(defaultIcon);

            if (itemLibraryRepresentation.dragIcon().isNull())
                itemLibraryRepresentation.setDragIcon(dragPixmap);

            if (itemLibraryRepresentation.category().isEmpty())
                itemLibraryRepresentation.setCategory(nodeInfo.category());
        }
    }

    return itemLibraryRepresentationList;
}

} // namespace Internal
} // namespace QmlDesigner

