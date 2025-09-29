// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibrarymodel.h"
#include "itemlibrarycategoriesmodel.h"
#include "itemlibrarycategory.h"
#include "itemlibraryentry.h"
#include "itemlibraryimport.h"
#include "itemlibraryitem.h"
#include "itemlibrarytracing.h"

#include <designermcumanager.h>
#include <model.h>
#include <nodehints.h>
#include <nodemetainfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmldesignerutils/version.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QIODevice>
#include <QLoggingCategory>
#include <QMetaProperty>
#include <QMimeData>
#include <QVariant>

namespace QmlDesigner {

using ItemLibraryTracing::category;

// sectionName can be an import url or a category name
void ItemLibraryModel::saveExpandedState(bool expanded, const QString &sectionName)
{
    NanotraceHR::Tracer tracer{"item library model save expanded state", category()};

    expandedStateHash.insert(sectionName, expanded);
}

bool ItemLibraryModel::loadExpandedState(const QString &sectionName)
{
    NanotraceHR::Tracer tracer{"item library model load expanded state", category()};
    return expandedStateHash.value(sectionName, true);
}

void ItemLibraryModel::saveCategoryVisibleState(bool isVisible, const QString &categoryName,
                                                const QString &importName)
{
    NanotraceHR::Tracer tracer{"item library model save category visible state", category()};

    categoryVisibleStateHash.insert(categoryName + '_' + importName, isVisible);
}

bool ItemLibraryModel::loadCategoryVisibleState(const QString &categoryName, const QString &importName)
{
    NanotraceHR::Tracer tracer{"item library model load category visible state", category()};

    return categoryVisibleStateHash.value(categoryName + '_' + importName, true);
}

void ItemLibraryModel::selectImportCategory(const QString &importUrl, int categoryIndex)
{
    NanotraceHR::Tracer tracer{"item library model select import category", category()};

    clearSelectedCategory();

    m_selectedImportUrl = importUrl;
    m_selectedCategoryIndex = categoryIndex;

    updateSelection();
}

void ItemLibraryModel::clearSelectedCategory()
{
    NanotraceHR::Tracer tracer{"item library model clear selected category", category()};

    if (m_selectedCategoryIndex != -1) {
        ItemLibraryImport *selectedImport = importByUrl(m_selectedImportUrl);
        if (selectedImport)
            selectedImport->clearSelectedCategory(m_selectedCategoryIndex);
    }
}

void ItemLibraryModel::selectImportFirstVisibleCategory()
{
    NanotraceHR::Tracer tracer{"item library model select import first visible category", category()};

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
    NanotraceHR::Tracer tracer{"item library model is any category hidden", category()};

    return m_isAnyCategoryHidden;
}

void ItemLibraryModel::setIsAnyCategoryHidden(bool state)
{
    NanotraceHR::Tracer tracer{"item library model set is any category hidden", category()};

    if (state != m_isAnyCategoryHidden) {
        m_isAnyCategoryHidden = state;
        emit isAnyCategoryHiddenChanged();
    }
}

bool ItemLibraryModel::importUnimportedSelected() const
{
    NanotraceHR::Tracer tracer{"item library model import unimported selected", category()};

    return m_importUnimportedSelected;
}

void ItemLibraryModel::setImportUnimportedSelected(bool state)
{
    NanotraceHR::Tracer tracer{"item library model set import unimported selected", category()};

    if (state != m_importUnimportedSelected) {
        m_importUnimportedSelected = state;
        emit importUnimportedSelectedChanged();
    }
}

QObject *ItemLibraryModel::itemsModel() const
{
    NanotraceHR::Tracer tracer{"item library model items model", category()};

    return m_itemsModel;
}

void ItemLibraryModel::setItemsModel(QObject *model)
{
    NanotraceHR::Tracer tracer{"item library model set items model", category()};

    m_itemsModel = model;
    emit itemsModelChanged();
}

void ItemLibraryModel::expandAll()
{
    NanotraceHR::Tracer tracer{"item library model expand all", category()};

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
    NanotraceHR::Tracer tracer{"item library model collapse all", category()};

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
    NanotraceHR::Tracer tracer{"item library model hide category", category()};

    ItemLibraryImport *import = importByUrl(importUrl);
    if (!import)
        return;

    import->hideCategory(categoryName);

    selectImportFirstVisibleCategory();
    setIsAnyCategoryHidden(true);
}

void ItemLibraryModel::showImportHiddenCategories(const QString &importUrl)
{
    NanotraceHR::Tracer tracer{"item library model show import hidden categories", category()};

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
    NanotraceHR::Tracer tracer{"item library model show all hidden categories", category()};

    for (const QPointer<ItemLibraryImport> &import : std::as_const(m_importList))
        import->showAllCategories();

    updateSelection(); // useful when all categories are hidden
    setIsAnyCategoryHidden(false);
    categoryVisibleStateHash.clear();
}

ItemLibraryModel::ItemLibraryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    NanotraceHR::Tracer tracer{"item library model constructor", category()};

    addRoleNames();
}

ItemLibraryModel::~ItemLibraryModel()
{
    NanotraceHR::Tracer tracer{"item library model destructor", category()};

    clearSections();
}

int ItemLibraryModel::rowCount(const QModelIndex & /*parent*/) const
{
    NanotraceHR::Tracer tracer{"item library model row count", category()};

    return m_importList.size();
}

QVariant ItemLibraryModel::data(const QModelIndex &index, int role) const
{
    NanotraceHR::Tracer tracer{"item library model data", category()};

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
    NanotraceHR::Tracer tracer{"item library model set data", category()};

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
    NanotraceHR::Tracer tracer{"item library model role names", category()};

    return m_roleNames;
}

QString ItemLibraryModel::searchText() const
{
    NanotraceHR::Tracer tracer{"item library model search text", category()};

    return m_searchText;
}

void ItemLibraryModel::setSearchText(const QString &searchText)
{
    NanotraceHR::Tracer tracer{"item library model set search text", category()};

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
    NanotraceHR::Tracer tracer{"item library model entry to import", category()};

    return Import::createLibraryImport(entry.requiredImport(), QString::number(entry.majorVersion()) + QLatin1Char('.') +
                                                               QString::number(entry.minorVersion()));

}

void ItemLibraryModel::update(Model *model)
{
    NanotraceHR::Tracer tracer{"item library model update", category()};

    if (!model)
        return;

    beginResetModel();
    clearSections();

    const QString projectName = DocumentManager::currentProjectName();

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    QStringList excludedImports {
        projectName
    };

    // create import sections
    const Imports usedImports = model->usedImports();
    QHash<QString, ItemLibraryImport *> importHash;
    const QString generatedPrefix = compUtils.generatedComponentTypePrefix();
    for (const Import &import : model->imports()) {
        if (excludedImports.contains(import.url()) || import.url().startsWith(generatedPrefix))
            continue;

        bool addNew = true;
        QString importUrl = import.url();
        if (import.isFileImport())
            importUrl = import.toString(true, true).remove("\"");

        ItemLibraryImport *oldImport = importHash.value(importUrl);
        if (oldImport && oldImport->importEntry().url() == import.url()) {
            // Retain the higher version if multiples exist
            if (oldImport->importEntry().toVersion() >= import.toVersion() || import.hasVersion())
                addNew = false;
            else
                delete oldImport;
        }

        if (addNew) {
            auto sectionType = ItemLibraryImport::SectionType::Default;
            ItemLibraryImport *itemLibImport = new ItemLibraryImport(import, this, sectionType);
            itemLibImport->setImportUsed(usedImports.contains(import));
            importHash.insert(importUrl, itemLibImport);
        }
    }

    for (const auto itemLibImport : std::as_const(importHash)) {
        m_importList.append(itemLibImport);
        itemLibImport->setImportExpanded(loadExpandedState(itemLibImport->importUrl()));
    }

    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    const bool blockNewImports = document->inFileComponentModelActive();

    SourceId currentDocumentSourceId = model->fileUrlSourceId();

    QList<ItemLibraryEntry> itemLibEntries = model->allItemLibraryEntries();
    itemLibEntries.append(model->directoryImportsItemLibraryEntries());
    const QString ultralite = "QtQuickUltralite";
    for (const ItemLibraryEntry &entry : std::as_const(itemLibEntries)) {
        NodeMetaInfo metaInfo;

        metaInfo = NodeMetaInfo{entry.typeId(), model->projectStorage()};

        bool valid = metaInfo.isValid();

        bool isItem = valid && metaInfo.isQtQuickItem();
        bool forceVisibility = valid
                               && NodeHints::fromItemLibraryEntry(entry, model).visibleInLibrary();

        bool blocked = false;
        const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();
        if (mcuManager.isMCUProject()) {
            const QSet<QString> blockTypes = mcuManager.bannedItems();

            if (blockTypes.contains(QString::fromUtf8(entry.typeName())))
                blocked = true;

            // we need to exclude all items from unsupported imports but only if they are not user-defined modules
            if (!(entry.category() == ItemLibraryImport::userComponentsTitle())
                && !entry.requiredImport().isEmpty()
                && !mcuManager.allowedImports().contains(entry.requiredImport())) {
                blocked = true;
            }
        } else {
            blocked = entry.requiredImport().startsWith(ultralite);
        }

        Import import = entryToImport(entry);
        bool hasImport = model->hasImport(import, true, true);

        bool isImportPossible = !blockNewImports;

        bool isUsable = (valid && (isItem || forceVisibility))
                        && (entry.requiredImport().isEmpty() || hasImport);
        if (!blocked && (isUsable || isImportPossible)) {
            ItemLibraryImport *importSection = nullptr;
            QString catName = entry.category();
            if (isUsable) {
                if (entry.sourceId()) {
                    if (entry.sourceId().directoryPathId()
                        == currentDocumentSourceId.directoryPathId()) { // user components
                        if (currentDocumentSourceId == entry.sourceId())
                            continue;
                        importSection = importHash[ItemLibraryImport::userComponentsTitle()];
                        if (!importSection) {
                            importSection = new ItemLibraryImport(
                                        {}, this, ItemLibraryImport::SectionType::User);
                            m_importList.append(importSection);
                            importHash.insert(ItemLibraryImport::userComponentsTitle(), importSection);
                            importSection->setImportExpanded(loadExpandedState(catName));
                        }
                    } else { // directory import
                        importSection = importHash[entry.category()];
                    }
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
                if (entry.requiredImport().startsWith(generatedPrefix))
                    continue;
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

std::unique_ptr<QMimeData> ItemLibraryModel::getMimeData(const ItemLibraryEntry &itemLibraryEntry)
{
    NanotraceHR::Tracer tracer{"item library model get mime data", category()};

    auto mimeData = std::make_unique<QMimeData>();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << itemLibraryEntry;
    mimeData->setData(Constants::MIME_TYPE_ITEM_LIBRARY_INFO, data);

    mimeData->removeFormat("text/plain");

    return mimeData;
}

void ItemLibraryModel::clearSections()
{
    NanotraceHR::Tracer tracer{"item library model clear sections", category()};

    qDeleteAll(m_importList);
    m_importList.clear();
}

void ItemLibraryModel::updateSelection()
{
    NanotraceHR::Tracer tracer{"item library model update selection", category()};

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
    NanotraceHR::Tracer tracer{"item library model register QML types", category()};

    qmlRegisterAnonymousType<QmlDesigner::ItemLibraryModel>("ItemLibraryModel", 1);
}

ItemLibraryImport *ItemLibraryModel::importByUrl(const QString &importUrl) const
{
    NanotraceHR::Tracer tracer{"item library model import by URL", category()};

    for (ItemLibraryImport *itemLibraryImport : std::as_const(m_importList)) {
        if (itemLibraryImport->importUrl() == importUrl
            || (importUrl.isEmpty() && itemLibraryImport->importUrl() == "QtQuick")
            || (importUrl == ItemLibraryImport::userComponentsTitle()
                && itemLibraryImport->sectionType() == ItemLibraryImport::SectionType::User)
            || (importUrl == ItemLibraryImport::unimportedComponentsTitle()
                && itemLibraryImport->sectionType() == ItemLibraryImport::SectionType::Unimported)) {
            return itemLibraryImport;
        }
    }

    return nullptr;
}

void ItemLibraryModel::updateUsedImports(const Imports &usedImports)
{
    NanotraceHR::Tracer tracer{"item library model update used imports", category()};

    // imports in the excludeList are not marked used and thus can always be removed even when in use.
    const QList<QString> excludeList = {"SimulinkConnector"};

    for (ItemLibraryImport *importSection : std::as_const(m_importList)) {
        if (!excludeList.contains(importSection->importUrl()))
            importSection->setImportUsed(usedImports.contains(importSection->importEntry()));
    }
}

void ItemLibraryModel::updateVisibility(bool *changed)
{
    NanotraceHR::Tracer tracer{"item library model update visibility", category()};

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
    NanotraceHR::Tracer tracer{"item library model add role names", category()};

    int role = 0;
    const QMetaObject meta = ItemLibraryImport::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(role++, meta.property(i).name());
}

void ItemLibraryModel::sortSections()
{
    NanotraceHR::Tracer tracer{"item library model sort sections", category()};

    auto sectionSort = [](ItemLibraryImport *first, ItemLibraryImport *second) {
        return QString::localeAwareCompare(first->sortingName(), second->sortingName()) < 0;
    };

    std::sort(m_importList.begin(), m_importList.end(), sectionSort);

    for (ItemLibraryImport *itemLibImport : std::as_const(m_importList))
        itemLibImport->sortCategorySections();
}

} // namespace QmlDesigner

