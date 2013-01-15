/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include <model.h>
#include <nodemetainfo.h>

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
bool ItemLibrarySortedModel<T>::setElementVisible(int libId, bool visible)
{
    int pos = findElement(libId);
    if (m_elementOrder.at(pos).visible == visible)
        return false;

    int visiblePos = visibleElementPosition(libId);
    if (visible)
        insert(visiblePos, *(m_elementModels.value(libId)));
    else
        remove(visiblePos);

    m_elementOrder[pos].visible = visible;
    return true;
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

template <class T>
int ItemLibrarySortedModel<T>::visibleElementPosition(int libId) const
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



ItemLibraryItemModel::ItemLibraryItemModel(QScriptEngine *scriptEngine, int itemLibId, const QString &itemName)
    : QScriptValue(scriptEngine->newObject()),
      m_scriptEngine(scriptEngine),
      m_libId(itemLibId),
      m_name(itemName),
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

void ItemLibraryItemModel::setItemIconPath(const QString &iconPath)
{
    m_iconPath = iconPath;

    setProperty(QLatin1String("itemLibraryIconPath"),
                QString(QLatin1String("image://qmldesigner_itemlibrary/") + iconPath));
}

void ItemLibraryItemModel::setItemIconSize(const QSize &itemIconSize)
{
    m_iconSize = itemIconSize;
//    qDebug() << "set icon size" << itemIconSize;
    setItemIconPath(m_iconPath);
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

    QMap<int, ItemLibraryItemModel *>::const_iterator itemIt = m_sectionEntries.elements().constBegin();
    while (itemIt != m_sectionEntries.elements().constEnd()) {

        bool itemVisible = itemIt.value()->itemName().toLower().contains(searchText),
            itemChanged = false;
        itemChanged = m_sectionEntries.setElementVisible(itemIt.key(), itemVisible);

        *changed |= itemChanged;

        if (itemVisible)
            haveVisibleItems = true;

        ++itemIt;
    }

    return haveVisibleItems;
}


void ItemLibrarySectionModel::updateItemIconSize(const QSize &itemIconSize)
{
    foreach (ItemLibraryItemModel *item, m_sectionEntries.elements()) {
        item->setItemIconSize(itemIconSize);
    }
}

bool ItemLibrarySectionModel::operator<(const ItemLibrarySectionModel &other) const
{
    if (sectionName() == QLatin1String("QML Components")) //Qml Components always come first
        return true;
    return sectionName() < other.sectionName();
}

ItemLibraryModel::ItemLibraryModel(QScriptEngine *scriptEngine, QObject *parent)
    : ItemLibrarySortedModel<ItemLibrarySectionModel>(parent),
      m_scriptEngine(scriptEngine),
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

    foreach (ItemLibrarySectionModel *section, elements().values())
        section->updateItemIconSize(itemIconSize);
}


int ItemLibraryModel::getItemSectionIndex(int itemLibId)
{
    if (m_sections.contains(itemLibId))
        return elementModel(m_sections.value(itemLibId))->visibleItemIndex(itemLibId);
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

    return elementModel(sectionLibId)->isItemVisible(itemLibId);
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

         if ((valid || entry.forceImport())
                 && (entry.requiredImport().isEmpty()
                     || model->hasImport(entryToImport(entry), true, false) || entry.forceImport())) {
            QString itemSectionName = entry.category();
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

            m_itemInfos.insert(itemId, entry);

            itemModel = new ItemLibraryItemModel(m_scriptEngine.data(), itemId, entry.name());

            // delayed creation of (default) icons
            if (entry.iconPath().isEmpty())
                entry.setIconPath(QLatin1String(":/ItemLibrary/images/item-default-icon.png"));
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
    bool changed = false;

    QMap<int, ItemLibrarySectionModel *>::const_iterator sectionIt = elements().constBegin();
    while (sectionIt != elements().constEnd()) {

        ItemLibrarySectionModel *sectionModel = sectionIt.value();
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
        if (property.name() == QLatin1String("width"))
            return property.value().toInt();
    }
    return 64;
}

int ItemLibraryModel::getHeight(const ItemLibraryEntry &itemLibraryEntry)
{
    foreach (const ItemLibraryEntry::Property &property, itemLibraryEntry.properties())
    {
        if (property.name() == QLatin1String("height"))
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

} // namespace Internal
} // namespace QmlDesigner

