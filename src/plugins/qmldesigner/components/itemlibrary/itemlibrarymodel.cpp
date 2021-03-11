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

#include <designermcumanager.h>
#include <model.h>
#include <nodehints.h>
#include <nodemetainfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
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
    if (!index.isValid() || index.row() >= m_importList.count())
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
        updateVisibility(&changed, !m_searchText.isEmpty());
    }
}

Import ItemLibraryModel::entryToImport(const ItemLibraryEntry &entry)
{
    if (entry.majorVersion() == -1 && entry.minorVersion() == -1)
        return Import::createFileImport(entry.requiredImport());

    return Import::createLibraryImport(entry.requiredImport(), QString::number(entry.majorVersion()) + QLatin1Char('.') +
                                                               QString::number(entry.minorVersion()));

}

// Returns true if first import version is higher or equal to second import version
static bool compareVersions(const QString &version1, const QString &version2)
{
    if (version2.isEmpty() || version1 == version2)
        return true;
    const QStringList version1List = version1.split(QLatin1Char('.'));
    const QStringList version2List = version2.split(QLatin1Char('.'));
    if (version1List.count() == 2 && version2List.count() == 2) {
        int major1 = version1List.constFirst().toInt();
        int major2 = version2List.constFirst().toInt();
        if (major1 > major2) {
            return true;
        } else if (major1 == major2) {
            int minor1 = version1List.constLast().toInt();
            int minor2 = version2List.constLast().toInt();
            if (minor1 >= minor2)
                return true;
        }
    }
    return false;
}

void ItemLibraryModel::update(ItemLibraryInfo *itemLibraryInfo, Model *model)
{
    if (!model)
        return;

    beginResetModel();
    clearSections();

    Utils::FilePath qmlFileName = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName();
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::projectForFile(qmlFileName);
    QString projectName = project ? project->displayName() : "";

    // create import sections
    QHash<QString, ItemLibraryImport *> importHash;
    for (const Import &import : model->imports()) {
        if (import.isLibraryImport() && import.url() != projectName) {
            bool addNew = true;
            bool isQuick3DAsset = import.url().startsWith("Quick3DAssets.");
            QString importUrl = isQuick3DAsset ? ItemLibraryImport::quick3DAssetsTitle() : import.url();
            ItemLibraryImport *oldImport = importHash.value(importUrl);
            if (oldImport && oldImport->sectionType() == ItemLibraryImport::SectionType::Quick3DAssets
                && isQuick3DAsset) {
                addNew = false; // add only 1 Quick3DAssets import section
            } else if (oldImport && oldImport->importEntry().url() == import.url()) {
                // Retain the higher version if multiples exist
                if (compareVersions(oldImport->importEntry().version(), import.version()))
                    addNew = false;
                else
                    delete oldImport;
            }

            if (addNew) {
                auto sectionType = isQuick3DAsset ? ItemLibraryImport::SectionType::Quick3DAssets
                                                  : ItemLibraryImport::SectionType::Default;
                ItemLibraryImport *itemLibImport = new ItemLibraryImport(import, this, sectionType);
                importHash.insert(importUrl, itemLibImport);
            }
        }
    }

    for (const auto itemLibImport : qAsConst(importHash)) {
        m_importList.append(itemLibImport);
        itemLibImport->setImportExpanded(loadExpandedState(itemLibImport->importUrl()));
    }

    const QList<ItemLibraryEntry> itemLibEntries = itemLibraryInfo->entries();
    for (const ItemLibraryEntry &entry : itemLibEntries) {
        NodeMetaInfo metaInfo = model->metaInfo(entry.typeName());

        bool valid = metaInfo.isValid() && metaInfo.majorVersion() == entry.majorVersion();
        bool isItem = valid && metaInfo.isSubclassOf("QtQuick.Item");
        bool forceVisibility = valid && NodeHints::fromItemLibraryEntry(entry).visibleInLibrary();

        if (m_flowMode && metaInfo.isValid()) {
            isItem = metaInfo.isSubclassOf("FlowView.FlowItem")
                    || metaInfo.isSubclassOf("FlowView.FlowWildcard")
                    || metaInfo.isSubclassOf("FlowView.FlowDecision");
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
            isImportPossible = model->isImportPossible(import, true, true);
        bool isUsable = (valid && (isItem || forceVisibility))
                && (entry.requiredImport().isEmpty() || hasImport);
        if (!blocked && (isUsable || isImportPossible)) {
            ItemLibraryImport *importSection = nullptr;
            QString catName = entry.category();
            if (isUsable) {
                if (catName == ItemLibraryImport::userComponentsTitle()) {
                    // create an import section for user components
                    importSection = importByUrl(ItemLibraryImport::userComponentsTitle());
                    if (!importSection) {
                        importSection = new ItemLibraryImport(
                                    {}, this, ItemLibraryImport::SectionType::User);
                        m_importList.append(importSection);
                        importSection->setImportExpanded(loadExpandedState(catName));
                    }
                } else if (catName == "My Quick3D Components") {
                    importSection = importByUrl(ItemLibraryImport::quick3DAssetsTitle());
                } else {
                    if (catName.startsWith("Qt Quick - "))
                        catName = catName.mid(11); // remove "Qt Quick - "
                    importSection = importByUrl(entry.requiredImport());
                }
            } else {
                catName = ItemLibraryImport::unimportedComponentsTitle();
                importSection = importByUrl(catName);
                if (!importSection) {
                    importSection = new ItemLibraryImport(
                                {}, this, ItemLibraryImport::SectionType::Unimported);
                    m_importList.append(importSection);
                    importSection->setImportExpanded(loadExpandedState(catName));
                }
            }

            if (!importSection) {
                qWarning() << __FUNCTION__ << "No import section found! skipping entry: " << entry.name();
                continue;
            }

            // get or create category section
            ItemLibraryCategory *categorySection = importSection->getCategorySection(catName);
            if (!categorySection) {
                categorySection = new ItemLibraryCategory(catName, importSection);
                importSection->addCategory(categorySection);
                if (importSection->sectionType() == ItemLibraryImport::SectionType::Default)
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

void ItemLibraryModel::updateUsedImports(const QList<Import> &usedImports)
{
    // imports in the excludeList are not marked used and thus can always be removed even when in use.
    const QList<QString> excludeList = {"SimulinkConnector"};

    for (ItemLibraryImport *importSection : std::as_const(m_importList)) {
        if (!excludeList.contains(importSection->importUrl()))
            importSection->setImportUsed(usedImports.contains(importSection->importEntry()));
    }
}

void ItemLibraryModel::updateVisibility(bool *changed, bool expand)
{
    for (ItemLibraryImport *import : std::as_const(m_importList)) {
        bool categoryChanged = false;
        bool hasVisibleItems = import->updateCategoryVisibility(m_searchText, &categoryChanged, expand);
        *changed |= categoryChanged;

        if (import->sectionType() == ItemLibraryImport::SectionType::Unimported)
            *changed |= import->setVisible(!m_searchText.isEmpty());

        // expand import if it has an item matching search criteria
        if (expand && hasVisibleItems && !import->importExpanded())
            import->setImportExpanded();
    }

    if (changed) {
        beginResetModel();
        endResetModel();
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

