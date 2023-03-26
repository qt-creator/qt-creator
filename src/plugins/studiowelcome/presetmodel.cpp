// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "presetmodel.h"

#include "algorithm.h"

#include <optional>

using namespace StudioWelcome;

constexpr int NameRole = Qt::UserRole;
constexpr int ScreenSizeRole = Qt::UserRole + 1;
constexpr int IsUserPresetRole = Qt::UserRole + 2;

static const QString RecentsTabName = ::StudioWelcome::PresetModel::tr("Recents");
static const QString CustomTabName = ::StudioWelcome::PresetModel::tr("Custom");

/****************** PresetData ******************/


QString PresetData::recentsTabName()
{
    return RecentsTabName;
}

void PresetData::setData(const PresetsByCategory &presetsByCategory,
                         const std::vector<UserPresetData> &userPresetsData,
                         const std::vector<UserPresetData> &loadedRecentsData)
{
    QTC_ASSERT(!presetsByCategory.empty(), return );
    m_recents = loadedRecentsData;
    m_userPresets = userPresetsData;

    for (auto &[id, category] : presetsByCategory) {
        m_categories.push_back(category.name);
        m_presets.push_back(category.items);
    }

    PresetItems wizardPresets = Utils::flatten(m_presets);

    PresetItems userPresetItems = makeUserPresets(wizardPresets, m_userPresets);
    if (!userPresetItems.empty()) {
        m_categories.push_back(CustomTabName);
        m_presets.push_back(userPresetItems);
    }

    PresetItems recentPresets = makeUserPresets(wizardPresets, m_recents);
    if (!recentPresets.empty()) {
        Utils::prepend(m_categories, RecentsTabName);
        Utils::prepend(m_presets, recentPresets);
    }

    m_presetsByCategory = presetsByCategory;
}

void PresetData::reload(const std::vector<UserPresetData> &userPresetsData,
                        const std::vector<UserPresetData> &loadedRecentsData)
{
    m_categories.clear();
    m_presets.clear();
    m_recents.clear();
    m_userPresets.clear();
    setData(m_presetsByCategory, userPresetsData, loadedRecentsData);
}

std::shared_ptr<PresetItem> PresetData::findPresetItemForUserPreset(const UserPresetData &preset,
                                                                    const PresetItems &wizardPresets)
{
    return Utils::findOrDefault(wizardPresets, [&preset](const std::shared_ptr<PresetItem> &item) {
        return item->wizardName == preset.wizardName && item->categoryId == preset.categoryId;
    });
}

PresetItems PresetData::makeUserPresets(const PresetItems &wizardPresets,
                                        const std::vector<UserPresetData> &data)
{
    PresetItems result;

    for (const UserPresetData &userPresetData : data) {
        std::shared_ptr<PresetItem> foundPreset = findPresetItemForUserPreset(userPresetData,
                                                                              wizardPresets);
        if (!foundPreset)
            continue;

        auto presetItem = std::make_shared<UserPresetItem>();

        presetItem->categoryId = userPresetData.categoryId;
        presetItem->wizardName = userPresetData.wizardName;
        presetItem->screenSizeName = userPresetData.screenSize;

        presetItem->userName = userPresetData.name;
        presetItem->qtVersion = userPresetData.qtVersion;
        presetItem->styleName = userPresetData.styleName;
        presetItem->useQtVirtualKeyboard = userPresetData.useQtVirtualKeyboard;

        presetItem->create = foundPreset->create;
        presetItem->description = foundPreset->description;
        presetItem->fontIconCode = foundPreset->fontIconCode;
        presetItem->qmlPath = foundPreset->qmlPath;

        result.push_back(presetItem);
    }

    return result;
}

/****************** BasePresetModel ******************/

BasePresetModel::BasePresetModel(const PresetData *data, QObject *parent)
    : QAbstractListModel(parent)
    , m_data{data}
{}

QHash<int, QByteArray> BasePresetModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{NameRole, "name"},
                                            {ScreenSizeRole, "resolution"}};
    return roleNames;
}

/****************** PresetCategoryModel ******************/

PresetCategoryModel::PresetCategoryModel(const PresetData *data, QObject *parent)
    : BasePresetModel(data, parent)
{}

int PresetCategoryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(m_data->categories().size());
}

QVariant PresetCategoryModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    return m_data->categories().at(index.row());
}

/****************** PresetModel ******************/

PresetModel::PresetModel(const PresetData *data, QObject *parent)
    : BasePresetModel(data, parent)
{}

QHash<int, QByteArray> PresetModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{NameRole, "name"},
                                            {ScreenSizeRole, "resolution"},
                                            {IsUserPresetRole, "isUserPreset"}};
    return roleNames;
}

int PresetModel::rowCount(const QModelIndex &) const
{
    if (m_data->presets().empty())
        return 0;

    return static_cast<int>(presetsOfCurrentCategory().size());
}

QVariant PresetModel::data(const QModelIndex &index, int role) const
{
    std::shared_ptr<PresetItem> preset = presetsOfCurrentCategory().at(index.row());

    if (role == NameRole)
        return preset->displayName();
    else if (role == ScreenSizeRole)
        return preset->screenSize();
    else if (role == IsUserPresetRole)
        return preset->isUserPreset();
    else
        return {};
}

void PresetModel::setPage(int index)
{
    beginResetModel();

    m_page = static_cast<size_t>(index);

    endResetModel();
}

QString PresetModel::fontIconCode(int index) const
{
    std::shared_ptr<PresetItem> presetItem = preset(index);
    if (!presetItem)
        return {};

    return presetItem->fontIconCode;
}
