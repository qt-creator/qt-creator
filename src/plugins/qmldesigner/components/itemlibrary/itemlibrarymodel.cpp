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

#include "itemlibrarymodel.h"
#include "itemlibraryinfo.h"
#include "itemlibrarysection.h"
#include "itemlibraryitem.h"
#include "itemlibrarysection.h"

#include <model.h>
#include <nodemetainfo.h>

#include <utils/algorithm.h>

#include <QVariant>
#include <QMetaProperty>
#include <QLoggingCategory>
#include <QMimeData>
#include <QPainter>
#include <QPen>
#include <qdebug.h>

static Q_LOGGING_CATEGORY(itemlibraryPopulate, "qtc.itemlibrary.populate")

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
    : QAbstractListModel(parent)
{
    addRoleNames();
}

ItemLibraryModel::~ItemLibraryModel()
{
    clearSections();
}

int ItemLibraryModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_sections.count();
}

QVariant ItemLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() +1 > m_sections.count())
        return QVariant();


    if (m_roleNames.contains(role)) {
        QVariant value = m_sections.at(index.row())->property(m_roleNames.value(role));

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

QHash<int, QByteArray> ItemLibraryModel::roleNames() const
{
    return m_roleNames;
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

        bool changed = false;
        updateVisibility(&changed);
        if (changed)
            dataChanged(QModelIndex(), QModelIndex());
    }
}

Import entryToImport(const ItemLibraryEntry &entry)
{
    if (entry.majorVersion() == -1 && entry.minorVersion() == -1)
        return Import::createFileImport(entry.requiredImport());

    return Import::createLibraryImport(entry.requiredImport(), QString::number(entry.majorVersion()) + QLatin1Char('.') +
                                                               QString::number(entry.minorVersion()));

}

bool sectionExapanded(const QString &sectionName)
{
    if (collapsedStateHash.contains(sectionName))
        return collapsedStateHash.value(sectionName);

    return true;
}

void ItemLibraryModel::update(ItemLibraryInfo *itemLibraryInfo, Model *model)
{
    if (!model)
        return;

    beginResetModel();
    clearSections();

    QStringList imports;
    foreach (const Import &import, model->imports())
        if (import.isLibraryImport())
            imports << import.url() + QLatin1Char(' ') + import.version();


    qCInfo(itemlibraryPopulate) << Q_FUNC_INFO;
    foreach (ItemLibraryEntry entry, itemLibraryInfo->entries()) {

        qCInfo(itemlibraryPopulate) << entry.typeName() << entry.majorVersion() << entry.minorVersion();

        NodeMetaInfo metaInfo = model->metaInfo(entry.typeName());

        qCInfo(itemlibraryPopulate) << "valid: " << metaInfo.isValid() << metaInfo.majorVersion() << metaInfo.minorVersion();

        bool valid = metaInfo.isValid() && metaInfo.majorVersion() == entry.majorVersion();
        bool isItem = valid && metaInfo.isSubclassOf("QtQuick.Item");

        qCInfo(itemlibraryPopulate) << "isItem: " << isItem;

        qCInfo(itemlibraryPopulate) << "required import: " << entry.requiredImport() << entryToImport(entry).toImportString();

        if (!isItem && valid) {
            qDebug() << Q_FUNC_INFO;
            qDebug() << metaInfo.typeName() << "is not a QtQuick.Item";
            qDebug() << Utils::transform(metaInfo.superClasses(), &NodeMetaInfo::typeName);
        }

        if (valid
                && isItem //We can change if the navigator does support pure QObjects
                && (entry.requiredImport().isEmpty()
                    || model->hasImport(entryToImport(entry), true, true))) {
            QString itemSectionName = entry.category();
            qCInfo(itemlibraryPopulate) << "Adding:" << entry.typeName() << "to:" << entry.category();
            ItemLibrarySection *sectionModel = sectionByName(itemSectionName);

            if (sectionModel == 0) {
                sectionModel = new ItemLibrarySection(itemSectionName, this);
                m_sections.append(sectionModel);
                sectionModel->setSectionExpanded(sectionExapanded(itemSectionName));
            }

            ItemLibraryItem *item = new ItemLibraryItem(sectionModel);
            item->setItemLibraryEntry(entry);
            sectionModel->addSectionEntry(item);
        }
    }

    sortSections();
    bool changed = false;
    updateVisibility(&changed);
    endResetModel();
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
    return m_sections;
}

void ItemLibraryModel::clearSections()
{
    qDeleteAll(m_sections);
    m_sections.clear();
}

void ItemLibraryModel::registerQmlTypes()
{
    qmlRegisterType<QmlDesigner::ItemLibrarySectionModel>();
    qmlRegisterType<QmlDesigner::ItemLibraryModel>();
}

ItemLibrarySection *ItemLibraryModel::sectionByName(const QString &sectionName)
{
    foreach (ItemLibrarySection *itemLibrarySection, m_sections) {
        if (itemLibrarySection->sectionName() == sectionName)
            return itemLibrarySection;
    }

    return 0;
}

void ItemLibraryModel::updateVisibility(bool *changed)
{
    foreach (ItemLibrarySection *itemLibrarySection, m_sections) {
        QString sectionSearchText = m_searchText;

        bool sectionChanged = false;
        bool sectionVisibility = itemLibrarySection->updateSectionVisibility(sectionSearchText,
                                                                             &sectionChanged);
        *changed |= sectionChanged;
        *changed |= itemLibrarySection->setVisible(sectionVisibility);
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
}

void ItemLibraryModel::sortSections()
{
    auto sectionSort = [](ItemLibrarySection *first, ItemLibrarySection *second) {
        return QString::localeAwareCompare(first->sortingName(), second->sortingName()) < 1;
    };

    std::sort(m_sections.begin(), m_sections.end(), sectionSort);

    foreach (ItemLibrarySection *itemLibrarySection, m_sections)
        itemLibrarySection->sortItems();
}

void registerQmlTypes()
{
    registerItemLibrarySortedModel();
}

} // namespace QmlDesigner

