// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibrarymodel.h"
#include "itemlibrarycategoriesmodel.h"
#include "itemlibraryimport.h"
#include "itemlibrarycategory.h"
#include "itemlibraryitem.h"
#include "itemlibraryinfo.h"

#include <designermcumanager.h>
#include <model.h>
#include <nodehints.h>
#include <nodemetainfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QIODevice>
#include <QLoggingCategory>
#include <QMetaProperty>
#include <QMimeData>
#include <QVariant>

namespace QmlDesigner {

// sectionName can be an import url or a category name
void ItemLibraryModel::saveExpandedState(bool expanded, const QString &sectionName)
{
    expandedStateHash.insert(sectionName, expanded);
}

bool ItemLibraryModel::loadExpandedState(const QString &sectionName)
{
    return expandedStateHash.value(sectionName, true);
}

void ItemLibraryModel::saveCategoryVisibleState(bool isVisible, const QString &categoryName,
                                                const QString &importName)
{
    categoryVisibleStateHash.insert(categoryName + '_' + importName, isVisible);
}

bool ItemLibraryModel::loadCategoryVisibleState(const QString &categoryName, const QString &importName)
{
    return categoryVisibleStateHash.value(categoryName + '_' + importName, true);
}

void ItemLibraryModel::selectImportCategory(const QString &importUrl, int categoryIndex)
{
    clearSelectedCategory();

    m_selectedImportUrl = importUrl;
    m_selectedCategoryIndex = categoryIndex;

    updateSelection();
}

void ItemLibraryModel::clearSelectedCategory()
{
    if (m_selectedCategoryIndex != -1) {
        ItemLibraryImport *selectedImport = importByUrl(m_selectedImportUrl);
        if (selectedImport)
            selectedImport->clearSelectedCategory(m_selectedCategoryIndex);
    }
}

void ItemLibraryModel::selectImportFirstVisibleCategory()
{
    if (m_selectedCategoryIndex != -1) {
        ItemLibraryImport *selectedImport = importByUrl(m_selectedImportUrl);
        if (selectedImport) {
            ItemLibraryCategory *selectedCategory = selectedImport->getCategoryAt(m_selectedCategoryIndex);
            if (selectedCategory) {
                bool isUnimported = selectedImport->sectionType() == ItemLibraryImport::SectionType::Unimported;
                // unimported category is always visible so checking its Import visibility instead
                bool isVisible = isUnimported ? selectedImport->importVisible()
                                              : selectedCategory->isCategoryVisible();
                if (isVisible)
                    return; // there is already a selected visible category

                clearSelectedCategory();
            }
        }
    }

    for (const QPointer<ItemLibraryImport> &import : std::as_const(m_importList)) {
        if (!import->isAllCategoriesHidden()) {
            m_selectedImportUrl = import->importUrl();
            m_selectedCategoryIndex = import->selectFirstVisibleCategory();

            ItemLibraryCategory *selectedCategory = import->getCategoryAt(m_selectedCategoryIndex);
            if (selectedCategory) {
                setItemsModel(selectedCategory->itemModel());
                setImportUnimportedSelected(import->importUnimported());
                return;
            }
        }
    }

    m_selectedImportUrl.clear();
    m_selectedCategoryIndex = -1;
    setItemsModel(nullptr);
}

bool ItemLibraryModel::isAnyCategoryHidden() const
{
    return m_isAnyCategoryHidden;
}

void ItemLibraryModel::setIsAnyCategoryHidden(bool state)
{
    if (state != m_isAnyCategoryHidden) {
        m_isAnyCategoryHidden = state;
        emit isAnyCategoryHiddenChanged();
    }
}

bool ItemLibraryModel::importUnimportedSelected() const
{
    return m_importUnimportedSelected;
}

void ItemLibraryModel::setImportUnimportedSelected(bool state)
{
    if (state != m_importUnimportedSelected) {
        m_importUnimportedSelected = state;
        emit importUnimportedSelectedChanged();
    }
}

QObject *ItemLibraryModel::itemsModel() const
{
    return m_itemsModel;
}

void ItemLibraryModel::setItemsModel(QObject *model)
{
    m_itemsModel = model;
    emit itemsModelChanged();
}

void ItemLibraryModel::expandAll()
{
    int i = 0;
    for (const QPointer<ItemLibraryImport> &import : std::as_const(m_importList)) {
        if (!import->importExpanded()) {
            import->setImportExpanded();
            emit dataChanged(index(i), index(i), {m_roleNames.key("importExpanded")});
            saveExpandedState(true, import->importUrl());
        }
        import->expandCategories(true);
        ++i;
    }
}

void ItemLibraryModel::collapseAll()
{
    int i = 0;
    for (const QPointer<ItemLibraryImport> &import : std::as_const(m_importList)) {
        if (import->hasCategories() && import->importExpanded()) {
            import->setImportExpanded(false);
            emit dataChanged(index(i), index(i), {m_roleNames.key("importExpanded")});
            saveExpandedState(false, import->importUrl());
        }
        ++i;
    }
}

void ItemLibraryModel::hideCategory(const QString &importUrl, const QString &categoryName)
{
    ItemLibraryImport *import = importByUrl(importUrl);
    if (!import)
        return;

    import->hideCategory(categoryName);

    selectImportFirstVisibleCategory();
    setIsAnyCategoryHidden(true);
}

void ItemLibraryModel::showImportHiddenCategories(const QString &importUrl)
{
    ItemLibraryImport *targetImport = nullptr;
    bool hiddenCatsExist = false;
    for (const QPointer<ItemLibraryImport> &import : std::as_const(m_importList)) {
        if (import->importUrl() == importUrl)
            targetImport = import;
        else
            hiddenCatsExist |= !import->allCategoriesVisible();
    }

    if (targetImport) {
        targetImport->showAllCategories();
        updateSelection(); // useful when all categories are hidden
        setIsAnyCategoryHidden(hiddenCatsExist);
    }
}

void ItemLibraryModel::showAllHiddenCategories()
{
    for (const QPointer<ItemLibraryImport> &import : std::as_const(m_importList))
        import->showAllCategories();

    updateSelection(); // useful when all categories are hidden
    setIsAnyCategoryHidden(false);
    categoryVisibleStateHash.clear();
}

void ItemLibraryModel::setFlowMode(bool b)
{
    m_flowMode = b;
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
    return m_importList.size();
}

QVariant ItemLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_importList.size())
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

bool ItemLibraryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // currently only importExpanded property is updatable
    if (index.isValid() && m_roleNames.contains(role)) {
        QVariant currValue = m_importList.at(index.row())->property(m_roleNames.value(role));
        if (currValue != value) {
            m_importList[index.row()]->setProperty(m_roleNames.value(role), value);
            if (m_roleNames.value(role) == "importExpanded")
                saveExpandedState(value.toBool(), m_importList[index.row()]->importUrl());
            emit dataChanged(index, index, {role});
            return true;
        }
    }
    return false;
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
        if (updateVisibility(&changed); changed) {
            beginResetModel();
            endResetModel();
        }

        selectImportFirstVisibleCategory();
    }
}

Import ItemLibraryModel::entryToImport(const ItemLibraryEntry &entry)
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

    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    Utils::FilePath qmlFileName = document->fileName();
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::projectForFile(qmlFileName);
    QString projectName = project ? project->displayName() : "";

    QStringList excludedImports {
        QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER).mid(1) + ".MaterialBundle",
        QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER).mid(1) + ".EffectBundle"
    };

    // create import sections
    const Imports usedImports = model->usedImports();
    QHash<QString, ItemLibraryImport *> importHash;
    for (const Import &import : model->imports()) {
        if (import.url() != projectName) {
            if (excludedImports.contains(import.url()))
                continue;
            bool addNew = true;
            bool isQuick3DAsset = import.url().startsWith("Quick3DAssets.");
            QString importUrl = import.url();
            if (isQuick3DAsset)
                importUrl = ItemLibraryImport::quick3DAssetsTitle();
            else if (import.isFileImport())
                importUrl = import.toString(true, true).remove("\"");

            ItemLibraryImport *oldImport = importHash.value(importUrl);
            if (oldImport && oldImport->sectionType() == ItemLibraryImport::SectionType::Quick3DAssets
                && isQuick3DAsset) {
                addNew = false; // add only 1 Quick3DAssets import section
            } else if (oldImport && oldImport->importEntry().url() == import.url()) {
                // Retain the higher version if multiples exist
                if (oldImport->importEntry().toVersion() >= import.toVersion() || import.hasVersion())
                    addNew = false;
                else
                    delete oldImport;
            }

            if (addNew) {
                auto sectionType = isQuick3DAsset ? ItemLibraryImport::SectionType::Quick3DAssets
                                                  : ItemLibraryImport::SectionType::Default;
                ItemLibraryImport *itemLibImport = new ItemLibraryImport(import, this, sectionType);
                itemLibImport->setImportUsed(usedImports.contains(import));
                importHash.insert(importUrl, itemLibImport);
            }
        }
    }

    for (const auto itemLibImport : std::as_const(importHash)) {
        m_importList.append(itemLibImport);
        itemLibImport->setImportExpanded(loadExpandedState(itemLibImport->importUrl()));
    }

    const bool blockNewImports = document->inFileComponentModelActive();
    const QList<ItemLibraryEntry> itemLibEntries = itemLibraryInfo->entries();
    for (const ItemLibraryEntry &entry : itemLibEntries) {
        NodeMetaInfo metaInfo = model->metaInfo(entry.typeName());

        bool valid = metaInfo.isValid()
                     && (metaInfo.majorVersion() >= entry.majorVersion()
                         || metaInfo.majorVersion() < 0);

        bool isItem = valid && metaInfo.isQtQuickItem();
        bool forceVisibility = valid && NodeHints::fromItemLibraryEntry(entry).visibleInLibrary();

        if (m_flowMode) {
            isItem = metaInfo.isFlowViewItem();
            forceVisibility = isItem;
        }

        bool blocked = false;
        const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();
        if (mcuManager.isMCUProject()) {
            const QSet<QString> blockTypes = mcuManager.bannedItems();

            if (blockTypes.contains(QString::fromUtf8(entry.typeName())))
                blocked = true;
        }

        Import import = entryToImport(entry);
        bool hasImport = model->hasImport(import, true, true);
        bool isImportPossible = false;
        if (!hasImport)
            isImportPossible = !blockNewImports && model->isImportPossible(import, true, true);
        bool isUsable = (valid && (isItem || forceVisibility))
                && (entry.requiredImport().isEmpty() || hasImport);
        if (!blocked && (isUsable || isImportPossible)) {
            ItemLibraryImport *importSection = nullptr;
            QString catName = entry.category();
            if (isUsable) {
                if (catName == ItemLibraryImport::userComponentsTitle()) {
                    if (entry.requiredImport().isEmpty()) { // user components
                        importSection = importHash[ItemLibraryImport::userComponentsTitle()];
                        if (!importSection) {
                            importSection = new ItemLibraryImport(
                                        {}, this, ItemLibraryImport::SectionType::User);
                            m_importList.append(importSection);
                            importHash.insert(ItemLibraryImport::userComponentsTitle(), importSection);
                            importSection->setImportExpanded(loadExpandedState(catName));
                        }
                    } else { // directory import
                        importSection = importHash[entry.requiredImport()];

                    }
                } else if (catName == ItemLibraryImport::quick3DAssetsTitle()) {
                    importSection = importHash[ItemLibraryImport::quick3DAssetsTitle()];
                } else {
                    if (catName.contains("Qt Quick - ")) {
                        QString sortingName = catName;
                        catName = catName.mid(11 + catName.indexOf("Qt Quick - ")); // remove "Qt Quick - " or "x.Qt Quick - "
                        categorySortingHash.insert(catName, sortingName);
                    }

                    importSection = importHash[entry.requiredImport().isEmpty() ? "QtQuick"
                                                                                : entry.requiredImport()];
                }
            } else {
                catName = ItemLibraryImport::unimportedComponentsTitle();
                importSection = importHash[catName];
                if (!importSection) {
                    importSection = new ItemLibraryImport(
                                {}, this, ItemLibraryImport::SectionType::Unimported);
                    m_importList.append(importSection);
                    importHash.insert(ItemLibraryImport::unimportedComponentsTitle(), importSection);
                    importSection->setImportExpanded(loadExpandedState(catName));
                }
            }

            if (!importSection) {
                qWarning() << __FUNCTION__ << "No import section found! skipping entry: " << entry.name();
                continue;
            }

            // get or create category section
            ItemLibraryCategory *categorySection = importSection->getCategoryByName(catName);
            if (!categorySection) {
                categorySection = new ItemLibraryCategory(catName, importSection);
                importSection->addCategory(categorySection);
            }
            if (importSection->sectionType() == ItemLibraryImport::SectionType::Default
                && !importSection->hasSingleCategory()) {
                categorySection->setExpanded(loadExpandedState(categorySection->categoryName()));
            }

            // create item
            auto item = new ItemLibraryItem(entry, isUsable, categorySection);
            categorySection->addItem(item);
        }
    }

    sortSections();

    bool changed = false;
    updateVisibility(&changed);

    updateSelection();
    endResetModel();
}

QMimeData *ItemLibraryModel::getMimeData(const ItemLibraryEntry &itemLibraryEntry)
{
    auto mimeData = new QMimeData();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << itemLibraryEntry;
    mimeData->setData(Constants::MIME_TYPE_ITEM_LIBRARY_INFO, data);

    mimeData->removeFormat("text/plain");

    return mimeData;
}

void ItemLibraryModel::clearSections()
{
    qDeleteAll(m_importList);
    m_importList.clear();
}

void ItemLibraryModel::updateSelection()
{
    if (m_selectedCategoryIndex != -1) {
        ItemLibraryImport *selectedImport = importByUrl(m_selectedImportUrl);
        if (selectedImport) {
            ItemLibraryCategory *selectedCategory = selectedImport->selectCategory(m_selectedCategoryIndex);
            if (selectedCategory) {
                setItemsModel(selectedCategory->itemModel());
                setImportUnimportedSelected(selectedImport->importUnimported());
                return;
            }
        }
    }

    selectImportFirstVisibleCategory();
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
                && itemLibraryImport->sectionType() == ItemLibraryImport::SectionType::User)
            || (importUrl == ItemLibraryImport::quick3DAssetsTitle()
                && itemLibraryImport->sectionType() == ItemLibraryImport::SectionType::Quick3DAssets)
            || (importUrl == ItemLibraryImport::unimportedComponentsTitle()
                && itemLibraryImport->sectionType() == ItemLibraryImport::SectionType::Unimported)) {
            return itemLibraryImport;
        }
    }

    return nullptr;
}

void ItemLibraryModel::updateUsedImports(const Imports &usedImports)
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
        bool hasVisibleItems = import->updateCategoryVisibility(m_searchText, &categoryChanged);
        *changed |= categoryChanged;

        if (import->sectionType() == ItemLibraryImport::SectionType::Unimported)
            *changed |= import->setVisible(!m_searchText.isEmpty());

        // expand import if it has an item matching search criteria
        if (!m_searchText.isEmpty() && hasVisibleItems && !import->importExpanded())
            import->setImportExpanded();
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

    for (ItemLibraryImport *itemLibImport : std::as_const(m_importList))
        itemLibImport->sortCategorySections();
}

} // namespace QmlDesigner

