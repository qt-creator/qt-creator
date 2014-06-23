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
            ItemLibrarySection *sectionModel = sectionByName(itemSectionName);
            ItemLibraryItem *itemModel;

            if (sectionModel == 0) {
                sectionModel = new ItemLibrarySection(itemSectionName, this);
                m_sectionModels.append(sectionModel);
            }

            itemModel = new ItemLibraryItem(sectionModel);
            itemModel->setItemLibraryEntry(entry);
            sectionModel->addSectionEntry(itemModel);
        }
    }

    updateVisibility();
}

QMimeData *ItemLibraryModel::getMimeData(const ItemLibraryEntry &itemLibraryEntry)
{
    QMimeData *mimeData = new QMimeData();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << itemLibraryEntry;
    mimeData->setData(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo"), data);

    mimeData->removeFormat(QStringLiteral("text/plain"));

    return mimeData;
}

QList<ItemLibrarySection *> ItemLibraryModel::sections() const
{
    return m_sectionModels;
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

    foreach (ItemLibrarySection *section, m_sectionModels) {
        if (section->isVisible())
            ++visibleCount;
    }

    return visibleCount;
}

QList<ItemLibrarySection *> ItemLibraryModel::visibleSections() const
{
    QList<ItemLibrarySection *> visibleSectionList;

    foreach (ItemLibrarySection *section, m_sectionModels) {
        if (section->isVisible())
            visibleSectionList.append(section);
    }

    return visibleSectionList;
}

ItemLibrarySection *ItemLibraryModel::sectionByName(const QString &sectionName)
{
    foreach (ItemLibrarySection *itemLibrarySection, m_sectionModels) {
        if (itemLibrarySection->sectionName() == sectionName)
            return itemLibrarySection;
    }

    return 0;
}

void ItemLibraryModel::updateVisibility()
{
    bool changed = false;

    foreach (ItemLibrarySection *itemLibrarySection, m_sectionModels) {
        QString sectionSearchText = m_searchText;

        if (itemLibrarySection->sectionName().toLower().contains(m_searchText))
            sectionSearchText.clear();

        bool sectionChanged = false,
            sectionVisibility = itemLibrarySection->updateSectionVisibility(sectionSearchText,
                                                                      &sectionChanged);
        if (sectionChanged)
            changed = true;

        changed |= itemLibrarySection->setVisible(sectionVisibility);
    }
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

