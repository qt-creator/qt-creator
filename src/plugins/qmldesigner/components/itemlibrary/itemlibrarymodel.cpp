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
#include "itemlibrarysection.h"
#include "itemlibraryitem.h"
#include "itemlibrarysection.h"

#include <model.h>
#include <nodemetainfo.h>

#include <QVariant>
#include <QMetaProperty>
#include <QMimeData>
#include <QPainter>
#include <QPen>
#include <qdebug.h>

static bool inline registerItemLibrarySortedModel() {
    qmlRegisterType<QmlDesigner::ItemLibrarySectionModel>();
    return true;
}

namespace QmlDesigner {

static QHash<QString, bool> collapsedStateHash;


void ItemLibraryModel::setExpanded(bool expanded, const QString &section)
{
    if (collapsedStateHash.contains(section))
        collapsedStateHash.remove(section);

    if (!expanded) //default is true
        collapsedStateHash.insert(section, expanded);
}

ItemLibraryModel::ItemLibraryModel(QObject *parent)
    : QAbstractListModel(parent),
      m_itemIconSize(64, 64),
      m_nextLibId(0)
{
    addRoleNames();
}

ItemLibraryModel::~ItemLibraryModel()
{
    clearSections();
}

int ItemLibraryModel::rowCount(const QModelIndex & /*parent*/) const
{
    return visibleSectionCount();
}

QVariant ItemLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() +1 > visibleSectionCount())
        return QVariant();


    if (m_roleNames.contains(role)) {
        QVariant value = visibleSections().at(index.row())->property(m_roleNames.value(role));

        ItemLibrarySectionModel* model = qobject_cast<ItemLibrarySectionModel *>(value.value<QObject*>());
        if (model)
            return QVariant::fromValue(model);

        ItemLibraryModel* model2 = qobject_cast<ItemLibraryModel *>(value.value<QObject*>());
        if (model2)
            return QVariant::fromValue(model2);

        return value;
    }

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return QVariant();
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

    foreach (ItemLibrarySection* itemLibrarySectionModel, sections()) {
        itemLibrarySectionModel->updateItemIconSize(itemIconSize);
    }
}

int ItemLibraryModel::getSectionLibId(int itemLibId)
{
    return m_sections.value(itemLibId);
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

    clearSections();
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
            ItemLibrarySection *sectionModel;
            ItemLibraryItem *itemModel;
            int itemId = m_nextLibId++, sectionId;

            if (sections.contains(itemSectionName)) {
                sectionId = sections.value(itemSectionName);
                sectionModel = section(sectionId);
            } else {
                sectionId = m_nextLibId++;
                sectionModel = new ItemLibrarySection(sectionId, itemSectionName, this);
                addSection(sectionModel, sectionId);
                sections.insert(itemSectionName, sectionId);
            }

            m_itemInfos.insert(itemId, entry);

            itemModel = new ItemLibraryItem(itemId, entry.name(), sectionModel);

            itemModel->setItemIconPath(entry.libraryEntryIconPath());
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

    mimeData->removeFormat(QStringLiteral("text/plain"));

    return mimeData;
}


QPixmap ItemLibraryModel::getLibraryEntryIcon(int libId)
{
    return QPixmap(m_itemInfos.value(libId).libraryEntryIconPath());
}

ItemLibrarySection *ItemLibraryModel::section(int libraryId)
{
    return m_sectionModels.value(libraryId);
}

QList<ItemLibrarySection *> ItemLibraryModel::sections() const
{
    return m_sectionModels.values();
}

void ItemLibraryModel::addSection(ItemLibrarySection *sectionModel, int sectionId)
{
    m_sectionModels.insert(sectionId, sectionModel);
    sectionModel->setVisible(true);
}

void ItemLibraryModel::clearSections()
{
    beginResetModel();
    qDeleteAll(m_sectionModels);
    m_sectionModels.clear();
    endResetModel();
}

void ItemLibraryModel::registerQmlTypes()
{
    qmlRegisterType<QmlDesigner::ItemLibrarySectionModel>();
    qmlRegisterType<QmlDesigner::ItemLibraryModel>();
}

int ItemLibraryModel::visibleSectionCount() const
{
    int visibleCount = 0;

    QMap<int, ItemLibrarySection*>::const_iterator sectionIterator = m_sectionModels.constBegin();
    while (sectionIterator != m_sectionModels.constEnd()) {
        ItemLibrarySection *sectionModel = sectionIterator.value();
        if (sectionModel->isVisible())
            ++visibleCount;
        ++sectionIterator;
        qDebug() << __FUNCTION__ << visibleCount;
    }

    return visibleCount;
}

QList<ItemLibrarySection *> ItemLibraryModel::visibleSections() const
{
    QList<ItemLibrarySection *> visibleSectionList;

    QMap<int, ItemLibrarySection*>::const_iterator sectionIterator = m_sectionModels.constBegin();
    while (sectionIterator != m_sectionModels.constEnd()) {
        ItemLibrarySection *sectionModel = sectionIterator.value();
        if (sectionModel->isVisible())
            visibleSectionList.append(sectionModel);
        ++sectionIterator;
    }

    return visibleSectionList;
}

void ItemLibraryModel::updateVisibility()
{
    beginResetModel();
    endResetModel();
    bool changed = false;

    QMap<int, ItemLibrarySection*>::const_iterator sectionIterator = m_sectionModels.constBegin();
    while (sectionIterator != m_sectionModels.constEnd()) {
        ItemLibrarySection *sectionModel = sectionIterator.value();

        QString sectionSearchText = m_searchText;

        if (sectionModel->sectionName().toLower().contains(m_searchText))
            sectionSearchText.clear();

        bool sectionChanged = false,
            sectionVisibility = sectionModel->updateSectionVisibility(sectionSearchText,
                                                                      &sectionChanged);
        if (sectionChanged) {
            changed = true;
            if (sectionVisibility)
                emit sectionVisibilityChanged(sectionIterator.key());
        }

        changed |= sectionModel->setVisible(sectionVisibility);
        ++sectionIterator;
    }

    if (changed)
        emit visibilityChanged();
}

void ItemLibraryModel::addRoleNames()
{
    int role = 0;
    for (int propertyIndex = 0; propertyIndex < ItemLibrarySection::staticMetaObject.propertyCount(); ++propertyIndex) {
        QMetaProperty property = ItemLibrarySection::staticMetaObject.property(propertyIndex);
        m_roleNames.insert(role, property.name());
        ++role;
    }

    setRoleNames(m_roleNames);
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

void registerQmlTypes()
{
    registerItemLibrarySortedModel();
}

} // namespace QmlDesigner

