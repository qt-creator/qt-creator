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
#include "itemlibrarycategoriesmodel.h"
#include "itemlibraryimport.h"
#include "itemlibrarycategory.h"
#include "itemlibraryitem.h"
#include "itemlibraryinfo.h"

#include <model.h>
#include <nodehints.h>
#include <nodemetainfo.h>

#include <designermcumanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QIODevice>
#include <QLoggingCategory>
#include <QMetaProperty>
#include <QMimeData>
#include <QVariant>

namespace QmlDesigner {

// sectionName can be an import or category section
void ItemLibraryModel::setExpanded(bool expanded, const QString &sectionName)
{
    collapsedStateHash.insert(sectionName, expanded);
}

bool ItemLibraryModel::sectionExpanded(const QString &sectionName) const
{
    return collapsedStateHash.value(sectionName, true);
}

void ItemLibraryModel::setFlowMode(bool b)
{
    m_flowMode = b;
    bool changed;
    updateVisibility(&changed);
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
    return m_importList.count();
}

QVariant ItemLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() +1 > m_importList.count())
        return {};

    if (m_roleNames.contains(role)) {
        QVariant value = m_importList.at(index.row())->property(m_roleNames.value(role));

        auto model = qobject_cast<ItemLibraryCategoriesModel *>(value.value<QObject *>());
        if (model)
            return QVariant::fromValue(model);

        return value;
    }

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return {};
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

        bool changed = false;
        updateVisibility(&changed);
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

    beginResetModel();
    clearSections();

    // create import sections
    for (const Import &import : model->imports()) {
        if (import.isLibraryImport()) {
            ItemLibraryImport *itemLibImport = new ItemLibraryImport(import, this);
            m_importList.append(itemLibImport);
            itemLibImport->setExpanded(sectionExpanded(import.url()));
        }
    }

    const QList<ItemLibraryEntry> itemLibEntries = itemLibraryInfo->entries();
    for (const ItemLibraryEntry &entry : itemLibEntries) {
        NodeMetaInfo metaInfo = model->metaInfo(entry.typeName());

        bool valid = metaInfo.isValid() && metaInfo.majorVersion() == entry.majorVersion();
        bool isItem = valid && metaInfo.isSubclassOf("QtQuick.Item");
        bool forceVisiblity = valid && NodeHints::fromItemLibraryEntry(entry).visibleInLibrary();

        if (m_flowMode && metaInfo.isValid()) {
            isItem = metaInfo.isSubclassOf("FlowView.FlowItem")
                    || metaInfo.isSubclassOf("FlowView.FlowWildcard")
                    || metaInfo.isSubclassOf("FlowView.FlowDecision");
            forceVisiblity = isItem;
        }

        const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();
        if (mcuManager.isMCUProject()) {
            const QSet<QString> blockTypes = mcuManager.bannedItems();

            if (blockTypes.contains(QString::fromUtf8(entry.typeName())))
                valid = false;
        }

        if (valid && (isItem || forceVisiblity) // We can change if the navigator does support pure QObjects
            && (entry.requiredImport().isEmpty()
                || model->hasImport(entryToImport(entry), true, true))) {

            ItemLibraryImport *importSection = nullptr;

            QString catName = entry.category();
            if (catName == ItemLibraryImport::userComponentsTitle()) {
                // create an import section for user components
                importSection = importByUrl(ItemLibraryImport::userComponentsTitle());
                if (!importSection) {
                    importSection = new ItemLibraryImport({}, this);
                    m_importList.append(importSection);
                    importSection->setExpanded(sectionExpanded(catName));
                }
            } else {
                if (catName.startsWith("Qt Quick - "))
                    catName = catName.mid(11); // remove "Qt Quick - "

                importSection = importByUrl(entry.requiredImport());
            }

            if (!importSection) { // should not happen, but just in case
                qWarning() << __FUNCTION__ << "No import section found! skipping entry: " << entry.name();
                continue;
            }

            // get or create category section
            ItemLibraryCategory *categorySection = importSection->getCategorySection(catName);
            if (!categorySection) {
                categorySection = new ItemLibraryCategory(catName, importSection);
                importSection->addCategory(categorySection);
                categorySection->setExpanded(sectionExpanded(categorySection->categoryName()));
            }

            // create item
            auto item = new ItemLibraryItem(entry, categorySection);
            categorySection->addItem(item);
        }
    }

    sortSections();
    bool changed = false;
    updateVisibility(&changed);
    endResetModel();
}

QMimeData *ItemLibraryModel::getMimeData(const ItemLibraryEntry &itemLibraryEntry)
{
    auto mimeData = new QMimeData();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << itemLibraryEntry;
    mimeData->setData(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo"), data);

    mimeData->removeFormat(QStringLiteral("text/plain"));

    return mimeData;
}

void ItemLibraryModel::clearSections()
{
    qDeleteAll(m_importList);
    m_importList.clear();
}

void ItemLibraryModel::registerQmlTypes()
{
    qmlRegisterAnonymousType<QmlDesigner::ItemLibraryModel>("ItemLibraryModel", 1);
}

ItemLibraryImport *ItemLibraryModel::importByUrl(const QString &importUrl) const
{
    for (ItemLibraryImport *itemLibraryImport : std::as_const(m_importList)) {
        if (itemLibraryImport->importUrl() == importUrl
            || (importUrl.isEmpty() && itemLibraryImport->importUrl() == "QtQuick")
            || (importUrl == ItemLibraryImport::userComponentsTitle()
                && itemLibraryImport->importName() == ItemLibraryImport::userComponentsTitle())) {
            return itemLibraryImport;
        }
    }

    return nullptr;
}

void ItemLibraryModel::updateUsedImports(const QList<Import> &usedImports)
{
    // imports in the excludeList are not marked used and thus can always be removed even when in use.
    const QList<QString> excludeList = {"SimulinkConnector"};

    for (ItemLibraryImport *importSection : std::as_const(m_importList)) {
        if (!excludeList.contains(importSection->importUrl()))
            importSection->setImportUsed(usedImports.contains(importSection->importEntry()));
    }
}

void ItemLibraryModel::updateVisibility(bool *changed)
{
    for (ItemLibraryImport *import : std::as_const(m_importList)) {
        bool categoryChanged = false;
        import->updateCategoryVisibility(m_searchText, &categoryChanged);

        *changed |= categoryChanged;
    }
}

void ItemLibraryModel::addRoleNames()
{
    int role = 0;
    const QMetaObject meta = ItemLibraryImport::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(role++, meta.property(i).name());
}

void ItemLibraryModel::sortSections()
{
    auto sectionSort = [](ItemLibraryImport *first, ItemLibraryImport *second) {
        return QString::localeAwareCompare(first->sortingName(), second->sortingName()) < 0;
    };

    std::sort(m_importList.begin(), m_importList.end(), sectionSort);

    for (ItemLibraryImport *itemLibImport : qAsConst(m_importList))
        itemLibImport->sortCategorySections();
}

} // namespace QmlDesigner

