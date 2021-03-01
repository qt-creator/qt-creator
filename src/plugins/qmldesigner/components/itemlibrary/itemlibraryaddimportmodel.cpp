/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "itemlibraryaddimportmodel.h"

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
    return m_importList.count();
}

QVariant ItemLibraryAddImportModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_importList.count())
        return {};

    QString importUrl = m_importList[index.row()].url();

    if (m_roleNames[role] == "importUrl")
        return importUrl;

    if (m_roleNames[role] == "importVisible")
        return m_searchText.isEmpty() || importUrl.isEmpty() || m_importFilterList.contains(importUrl);

    if (m_roleNames[role] == "isSeparator")
        return importUrl.isEmpty();

    qWarning() << Q_FUNC_INFO << "invalid role requested";

    return {};
}

QHash<int, QByteArray> ItemLibraryAddImportModel::roleNames() const
{
    return m_roleNames;
}

void ItemLibraryAddImportModel::update(const QList<Import> &possibleImports)
{
    beginResetModel();
    m_importList.clear();

    const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();
    const bool isQtForMCUs = mcuManager.isMCUProject();
    QList<Import> filteredImports;
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

    Utils::sort(filteredImports, [this](const Import &firstImport, const Import &secondImport) {
        if (firstImport.url() == secondImport.url())
            return firstImport.toString() < secondImport.toString();

        if (firstImport.url() == "QtQuick")
            return true;

        if (secondImport.url() == "QtQuick")
            return false;

        const bool firstPriority = m_priorityImports.contains(firstImport.url());
        if (firstPriority != m_priorityImports.contains(secondImport.url()))
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
        if (import.isLibraryImport()) {
            bool currentIsPriority = m_priorityImports.contains(import.url());
            if (previousIsPriority && !currentIsPriority)
                m_importList.append(Import::empty()); // empty import acts as a separator
            m_importList.append(import);
            previousIsPriority = currentIsPriority;
        }
    }

    endResetModel();
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

void ItemLibraryAddImportModel::setPriorityImports(const QSet<QString> &priorityImports)
{
    m_priorityImports = priorityImports;
}

} // namespace QmlDesigner
