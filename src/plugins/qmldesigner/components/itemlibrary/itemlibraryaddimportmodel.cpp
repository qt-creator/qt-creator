// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryaddimportmodel.h"
#include "itemlibraryconstants.h"

#include <designermcumanager.h>
#include <utils/algorithm.h>

#include <QDebug>
#include <QVariant>
#include <QMetaProperty>

namespace QmlDesigner {

ItemLibraryAddImportModel::ItemLibraryAddImportModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // add role names
    m_roleNames.insert(Qt::UserRole + 1, "importUrl");
    m_roleNames.insert(Qt::UserRole + 2, "importVisible");
    m_roleNames.insert(Qt::UserRole + 3, "isSeparator");
}

ItemLibraryAddImportModel::~ItemLibraryAddImportModel()
{

}

int ItemLibraryAddImportModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_importList.size();
}

QVariant ItemLibraryAddImportModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_importList.size())
        return {};

    Import import = m_importList[index.row()];
    if (m_roleNames[role] == "importUrl")
        return m_importList[index.row()].toString(true, true);

    if (m_roleNames[role] == "importVisible")
        return m_searchText.isEmpty() || import.url().isEmpty() || m_importFilterList.contains(import.url());

    if (m_roleNames[role] == "isSeparator")
        return import.isEmpty();

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return {};
}

QHash<int, QByteArray> ItemLibraryAddImportModel::roleNames() const
{
    return m_roleNames;
}

namespace {
bool isPriorityImport(QStringView importUrl)
{
    return std::find(std::begin(priorityImports), std::end(priorityImports), importUrl)
           != std::end(priorityImports);
}
} // namespace

void ItemLibraryAddImportModel::update(const Imports &possibleImports)
{
    beginResetModel();
    m_importList.clear();

    const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();
    const bool isQtForMCUs = mcuManager.isMCUProject();
    Imports filteredImports;
    if (isQtForMCUs) {
        const QStringList mcuAllowedList = mcuManager.allowedImports();
        const QStringList mcuBannedList = mcuManager.bannedImports();
        filteredImports = Utils::filtered(possibleImports,
                                          [&](const Import &import) {
                                              return (mcuAllowedList.contains(import.url())
                                                      || !import.url().startsWith("Qt"))
                                                     && !mcuBannedList.contains(import.url());
                                          });
    } else {
        filteredImports = possibleImports;
    }

    Utils::sort(filteredImports, [](const Import &firstImport, const Import &secondImport) {
        if (firstImport.url() == secondImport.url())
            return firstImport.toString() < secondImport.toString();

        if (firstImport.url() == "QtQuick")
            return true;

        if (secondImport.url() == "QtQuick")
            return false;

        const bool firstPriority = isPriorityImport(firstImport.url());
        if (firstPriority != isPriorityImport(secondImport.url()))
            return firstPriority;

        if (firstImport.isLibraryImport() && secondImport.isFileImport())
            return false;

        if (firstImport.isFileImport() && secondImport.isLibraryImport())
            return true;

        if (firstImport.isFileImport() && secondImport.isFileImport())
            return QString::localeAwareCompare(firstImport.file(), secondImport.file()) < 0;

        if (firstImport.isLibraryImport() && secondImport.isLibraryImport())
            return QString::localeAwareCompare(firstImport.url(), secondImport.url()) < 0;

        return false;
    });

    // create import sections
    bool previousIsPriority = false;
    for (const Import &import : std::as_const(filteredImports)) {
        bool currentIsPriority = isPriorityImport(import.url());
        if (previousIsPriority && !currentIsPriority)
            m_importList.append(Import::empty()); // empty import acts as a separator
        m_importList.append(import);
        previousIsPriority = currentIsPriority;
    }

    endResetModel();
}

Import ItemLibraryAddImportModel::getImport(const QString &importUrl) const
{
    for (const Import &import : std::as_const(m_importList))
        if (import.url() == importUrl)
            return import;

    return {};
}

void ItemLibraryAddImportModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText != lowerSearchText) {
        beginResetModel();
        m_searchText = lowerSearchText;

        for (const Import &import : std::as_const(m_importList)) {
            if (import.url().toLower().contains(lowerSearchText))
                m_importFilterList.insert(import.url());
            else
                m_importFilterList.remove(import.url());
        }
        endResetModel();
    }
}

Import ItemLibraryAddImportModel::getImportAt(int index) const
{
    return m_importList.at(index);
}

} // namespace QmlDesigner
