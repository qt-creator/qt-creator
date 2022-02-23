/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "presetmodel.h"
#include <utils/optional.h>

#include "algorithm.h"

using namespace StudioWelcome;

constexpr int NameRole = Qt::UserRole;
constexpr int ScreenSizeRole = Qt::UserRole + 1;
constexpr int IsUserPresetRole = Qt::UserRole + 2;

static const QString RecentsTabName = QObject::tr("Recents");
static const QString CustomTabName = QObject::tr("Custom");

/****************** PresetData ******************/

void PresetData::setData(const PresetsByCategory &presetsByCategory,
                         const std::vector<UserPresetData> &userPresetsData,
                         const std::vector<RecentPresetData> &loadedRecentsData)
{
    QTC_ASSERT(!presetsByCategory.empty(), return );
    m_recents = loadedRecentsData;
    m_userPresets = userPresetsData;

    if (!m_recents.empty()) {
        m_categories.push_back(RecentsTabName);
        m_presets.push_back({});
    }

    for (auto &[id, category] : presetsByCategory) {
        m_categories.push_back(category.name);
        m_presets.push_back(category.items);
    }

    PresetItems wizardPresets = Utils::flatten(m_presets);

    PresetItems recentPresets = makeRecentPresets(wizardPresets);
    if (!m_recents.empty()) {
        m_presets[0] = recentPresets;
    }

    PresetItems userPresetItems = makeUserPresets(wizardPresets);
    if (!userPresetItems.empty()) {
        m_categories.push_back(CustomTabName);
        m_presets.push_back(userPresetItems);
    }

    m_presetsByCategory = presetsByCategory;
}

void PresetData::reload(const std::vector<UserPresetData> &userPresetsData,
                        const std::vector<RecentPresetData> &loadedRecentsData)
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

PresetItems PresetData::makeUserPresets(const PresetItems &wizardPresets)
{
    PresetItems result;

    for (const UserPresetData &userPresetData : m_userPresets) {
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

std::shared_ptr<PresetItem> PresetData::findPresetItemForRecent(const RecentPresetData &recent, const PresetItems &wizardPresets)
{
    return Utils::findOrDefault(wizardPresets, [&recent](const std::shared_ptr<PresetItem> &item) {
        bool sameName = item->categoryId == recent.category
                        && item->displayName() == recent.presetName;

        bool sameType = (recent.isUserPreset ? item->isUserPreset() : !item->isUserPreset());

        return sameName && sameType;
    });
}

PresetItems PresetData::makeRecentPresets(const PresetItems &wizardPresets)
{
    PresetItems result;

    for (const RecentPresetData &recent : m_recents) {
        std::shared_ptr<PresetItem> preset = findPresetItemForRecent(recent, wizardPresets);

        if (preset) {
            auto clone = std::shared_ptr<PresetItem>{preset->clone()};
            clone->screenSizeName = recent.sizeName;
            result.push_back(clone);
        }
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
