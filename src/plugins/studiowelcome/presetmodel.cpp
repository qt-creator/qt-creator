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
#include <utils/qtcassert.h>

#include "algorithm.h"

using namespace StudioWelcome;

/****************** PresetData ******************/

void PresetData::setData(const PresetsByCategory &presetsByCategory,
                         const std::vector<RecentPreset> &loadedRecents)
{
    QTC_ASSERT(!presetsByCategory.empty(), return);
    m_recents = loadedRecents;

    if (!m_recents.empty()) {
        m_categories.push_back("Recents");
        m_presets.push_back({});
    }

    for (auto &[id, category] : presetsByCategory) {
        m_categories.push_back(category.name);
        m_presets.push_back(category.items);
    }

    PresetItems presets = Utils::flatten(m_presets);

    std::vector<PresetItem> recentPresets = makeRecentPresets(presets);

    if (!m_recents.empty())
        m_presets[0] = recentPresets;
}

std::vector<PresetItem> PresetData::makeRecentPresets(const PresetItems &wizardPresets)
{
    static const PresetItem empty;

    PresetItems result;

    for (const RecentPreset &recent : m_recents) {
        auto item = Utils::findOptional(wizardPresets, [&recent](const PresetItem &item) {
            return item.categoryId == std::get<0>(recent) && item.name == std::get<1>(recent);
        });

        if (item) {
            item->screenSizeName = std::get<2>(recent);
            result.push_back(item.value());
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
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::UserRole] = "name";
    return roleNames;
}

/****************** PresetCategoryModel ******************/

PresetCategoryModel::PresetCategoryModel(const PresetData *data, QObject *parent)
    : BasePresetModel(data, parent)
{}

int PresetCategoryModel::rowCount(const QModelIndex &) const
{
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
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::UserRole] = "name";
    roleNames[Qt::UserRole + 1] = "size";
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
    Q_UNUSED(role)
    PresetItem preset = presetsOfCurrentCategory().at(index.row());
    return QVariant::fromValue<QString>(preset.name + "\n" + preset.screenSizeName);
}

void PresetModel::setPage(int index)
{
    beginResetModel();

    m_page = static_cast<size_t>(index);

    endResetModel();
}

QString PresetModel::fontIconCode(int index) const
{
    Utils::optional<PresetItem> presetItem = preset(index);
    if (!presetItem)
        return {};

    return presetItem->fontIconCode;
}
