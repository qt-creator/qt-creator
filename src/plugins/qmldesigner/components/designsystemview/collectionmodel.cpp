// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "collectionmodel.h"
#include <designsystem/dsthemegroup.h>
#include <designsystem/dsthememanager.h>

#include "utils/qtcassert.h"

namespace QmlDesigner {

CollectionModel::CollectionModel(DSThemeManager *collection)
    : m_collection(collection)
{
    updateCache();
}

int CollectionModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_collection->themeCount());
}

QVariant CollectionModel::data(const QModelIndex &index, int role) const
{
    const auto themeId = findThemeId(index.column());
    const auto propInfo = findPropertyName(index.row());
    if (!propInfo)
        return {};

    const auto &[groupType, propName] = *propInfo;
    auto property = m_collection->property(themeId, groupType, propName);
    if (!property)
        return {};

    switch (role) {
    case Qt::DisplayRole:
        return property->value.toString();
    case static_cast<int>(Roles::GroupRole):
        return QVariant::fromValue<GroupType>(groupType);
    case static_cast<int>(Roles::BindingRole):
        return property->isBinding;
    }

    return {};
}

QModelIndex CollectionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < rowCount(parent) && column < columnCount(parent)) {
        return createIndex(row, column);
    }
    return {};
}

QModelIndex CollectionModel::parent([[maybe_unused]] const QModelIndex &index) const
{
    return {};
}

int CollectionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_collection->propertyCount());
}

QVariant CollectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return QString::fromLatin1(m_collection->themeName(findThemeId(section)));

    if (orientation == Qt::Vertical && role == Qt::DisplayRole) {
        if (auto propInfo = findPropertyName(section))
            return QString::fromLatin1(propInfo->second);
    }

    return {};
}

QHash<int, QByteArray> CollectionModel::roleNames() const
{
    auto roles = QAbstractItemModel::roleNames();
    roles.insert(static_cast<int>(Roles::GroupRole), "group");
    roles.insert(static_cast<int>(Roles::BindingRole), "isBinding");
    return roles;
}

void CollectionModel::updateCache()
{
    m_themeIdList = m_collection->allThemeIds();

    m_propertyInfoList.clear();
    m_collection->forAllGroups([this](GroupType gt, DSThemeGroup *themeGroup) {
        for (auto propName : themeGroup->propertyNames())
            m_propertyInfoList.push_back({gt, propName});
    });
}

ThemeId CollectionModel::findThemeId(int column) const
{
    QTC_ASSERT(column > -1 && column < static_cast<int>(m_themeIdList.size()), return 0);
    return m_themeIdList[column];
}

std::optional<PropInfo> CollectionModel::findPropertyName(int row) const
{
    QTC_ASSERT(row > -1 && row < static_cast<int>(m_propertyInfoList.size()), return {});
    return m_propertyInfoList[row];
}
} // namespace QmlDesigner
