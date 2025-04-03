// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "collectionmodel.h"
#include <designsystem/dsstore.h>
#include <designsystem/dsthemegroup.h>
#include <designsystem/dsthememanager.h>

#include "utils/qtcassert.h"

namespace QmlDesigner {

CollectionModel::CollectionModel(DSThemeManager *collection, DSStore *store)
    : m_collection(collection)
    , m_store(store)
{
    m_saveCompressionTimer.setSingleShot(true);
    m_saveCompressionTimer.setInterval(200);
    connect(&m_saveCompressionTimer, &QTimer::timeout, this, &CollectionModel::save);

    updateCache();
}

QStringList CollectionModel::themeNameList() const
{
    QStringList themeNames(m_themeIdList.size());
    std::transform(m_themeIdList.begin(), m_themeIdList.end(), themeNames.begin(), [this](ThemeId id) {
        return QString::fromLatin1(m_collection->themeName(id));
    });
    return themeNames;
}

void CollectionModel::setActiveTheme(const QString &themeName)
{
    if (const auto themeId = m_collection->themeId(themeName.toLatin1())) {
        m_collection->setActiveTheme(*themeId);
        aboutToSave();

        // Update the active status
        if (rowCount() && columnCount()) {
            emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
            updateBoundValues();
        }
    }
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

    const QVariant propertyValue = property->value.toString();
    const QVariant displayValue = property->isBinding
                                      ? m_store->resolvedDSBinding(propertyValue.toString())
                                            .value_or(ThemeProperty{})
                                            .value
                                      : property->value;

    switch (role) {
    case Qt::DisplayRole:
    case Roles::ResolvedValueRole:
        return displayValue;
    case Roles::PropertyValueRole:
        return propertyValue;
    case Roles::GroupRole:
        return QVariant::fromValue<GroupType>(groupType);
    case Roles::BindingRole:
        return property->isBinding;
    case Roles::ActiveThemeRole:
        return m_collection->activeTheme() == themeId;
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
    if (orientation == Qt::Horizontal) {
        auto themeId = findThemeId(section);
        switch (role) {
        case Qt::DisplayRole:
            return QString::fromLatin1(m_collection->themeName(themeId));
        case Roles::ActiveThemeRole:
            return m_collection->activeTheme() == themeId;
        default:
            break;
        }
    }

    if (orientation == Qt::Vertical) {
        if (auto propInfo = findPropertyName(section)) {
            if (role == Qt::DisplayRole)
                return QString::fromLatin1(propInfo->second);
            if (role == Roles::GroupRole)
                return QVariant::fromValue<GroupType>(propInfo->first);
        }
    }
    return {};
}

Qt::ItemFlags CollectionModel::flags(const QModelIndex &index) const
{
    // If group type is FLAGS and not binding block editable
    if (data(index, Roles::GroupRole).value<GroupType>() == GroupType::Flags
        && !data(index, Roles::BindingRole).toBool() && !m_editableOverride)
        return QAbstractItemModel::flags(index);

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

QHash<int, QByteArray> CollectionModel::roleNames() const
{
    auto roles = QAbstractItemModel::roleNames();
    roles.insert(Roles::ResolvedValueRole, "resolvedValue");
    roles.insert(Roles::GroupRole, "group");
    roles.insert(Roles::BindingRole, "isBinding");
    roles.insert(Roles::ActiveThemeRole, "isActive");
    roles.insert(Roles::PropertyValueRole, "propertyValue");
    return roles;
}

bool CollectionModel::insertColumns([[maybe_unused]] int column, int count, const QModelIndex &parent)
{
    // Append column only
    if (parent.isValid() || count < 0)
        return false;

    bool addSuccess = false;
    while (count--)
        addSuccess |= m_collection->addTheme(QByteArrayLiteral("theme")).has_value();

    if (addSuccess) {
        beginResetModel();
        updateCache();
        endResetModel();
        aboutToSave();
        emit themeNameChanged();
    }
    return true;
}

bool CollectionModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    const auto sentinelIndex = column + count;
    if (parent.isValid() || column < 0 || count < 1 || sentinelIndex > columnCount(parent))
        return false;

    beginResetModel();
    while (column < sentinelIndex)
        m_collection->removeTheme(m_themeIdList[column++]);

    updateCache();
    endResetModel();
    aboutToSave();
    emit themeNameChanged();
    return true;
}

bool CollectionModel::removeRows(int row, int count, const QModelIndex &parent)
{
    const auto sentinelIndex = row + count;
    if (parent.isValid() || row < 0 || count < 1 || sentinelIndex > rowCount(parent))
        return false;

    beginResetModel();
    while (row < sentinelIndex) {
        auto [groupType, name] = m_propertyInfoList[row++];
        m_store->breakBindings(m_collection, name);
        m_collection->removeProperty(groupType, name);
    }
    updateCache();
    endResetModel();
    aboutToSave();
    return true;
}

void CollectionModel::updateCache()
{
    m_themeIdList.clear();
    m_propertyInfoList.clear();

    if (m_collection) {
        m_themeIdList = m_collection->allThemeIds();

        m_collection->forAllGroups([this](GroupType gt, DSThemeGroup *themeGroup) {
            for (auto propName : themeGroup->propertyNames())
                m_propertyInfoList.push_back({gt, propName});
        });
    }
}

void CollectionModel::addProperty(GroupType group, const QString &name, const QVariant &value, bool isBinding)
{
    if (m_collection->addProperty(group, {name.toUtf8(), value, isBinding})) {
        beginResetModel();
        updateCache();
        endResetModel();

        aboutToSave();
    }
}

bool CollectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool result = false;

    ThemeProperty p = value.value<ThemeProperty>();
    switch (role) {
    case Qt::EditRole: {
        const auto [groupType, propName] = m_propertyInfoList[index.row()];
        p.name = propName;
        if (p.isBinding) {
            // Check if binding is valid design system binding.
            const QString collectionName = m_store->typeName(m_collection).value_or("");
            const QString propName = QString::fromLatin1(p.name);
            CollectionBinding currentPropBinding{collectionName, propName};
            if (!m_store->resolvedDSBinding(p.value.toString(), currentPropBinding))
                return false; // Invalid binding, it must resolved to a valid property.
        }

        const ThemeId id = m_themeIdList[index.column()];
        if (m_collection->updateProperty(id, groupType, p)) {
            updateCache();

            emit dataChanged(index, index);
            updateBoundValues();
            result = true;
        }
    }
    default:
        break;
    }

    if (result) {
        aboutToSave();
        if (p.isBinding)
            updateBoundValues();
    }

    return result;
}

bool CollectionModel::setHeaderData(int section,
                                    Qt::Orientation orientation,
                                    const QVariant &value,
                                    int role)
{
    if (role != Qt::EditRole || section < 0
        || (orientation == Qt::Horizontal && section >= columnCount())
        || (orientation == Qt::Vertical && section >= rowCount())) {
        return false; // Out of bounds
    }

    const auto &newName = value.toString().toUtf8();
    bool success = false;
    if (orientation == Qt::Vertical) {
        // Property Name
        if (auto propInfo = findPropertyName(section)) {
            auto [groupType, propName] = *propInfo;
            success = m_collection->renameProperty(groupType, propName, newName);
            if (success)
                m_store->refactorBindings(m_collection, propName, newName);
        }
    } else {
        // Theme
        const auto themeId = findThemeId(section);
        success = m_collection->renameTheme(themeId, newName);
        if (success)
            emit themeNameChanged();
    }

    if (success) {
        beginResetModel();
        updateCache();
        endResetModel();

        aboutToSave();
    }

    return success;
}

bool CollectionModel::editableOverride() const
{
    return m_editableOverride;
}

void CollectionModel::setEditableOverride(bool value)
{
    if (value == m_editableOverride)
        return;

    m_editableOverride = value;
    emit editableOverrideChanged();
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

void CollectionModel::save()
{
    QTC_ASSERT(m_store, return);
    m_store->save();
}

void CollectionModel::aboutToSave()
{
    m_saveCompressionTimer.start();
}

void CollectionModel::updateBoundValues()
{
    // Re-evaluate the value of the all bound properties.
    for (int themeIdCol = 0; themeIdCol < columnCount(); ++themeIdCol) {
        const auto themeId = findThemeId(themeIdCol);
        for (int propIndex = 0; propIndex < rowCount(); ++propIndex) {
            const auto propInfo = findPropertyName(propIndex);
            if (!propInfo)
                continue;
            const auto &[groupType, propName] = *propInfo;
            if (auto property = m_collection->property(themeId, groupType, propName)) {
                if (property->isValid() && property->isBinding) {
                    auto modelIndex = index(propIndex, themeIdCol);
                    emit dataChanged(modelIndex, modelIndex);
                }
            }
        }
    }
}

} // namespace QmlDesigner
