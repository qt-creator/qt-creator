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

#pragma once

#include <QAbstractListModel>
#include <QDebug>
#include <QUrl>

#include <utils/filepath.h>
#include <utils/optional.h>

#include "recentpresets.h"

namespace Utils {
class Wizard;
}

namespace StudioWelcome {

struct PresetItem
{
    QString name;
    QString categoryId;
    QString screenSizeName;
    QString description;
    QUrl qmlPath;
    QString fontIconCode;
    std::function<Utils::Wizard *(const Utils::FilePath &path)> create;
};

inline QDebug &operator<<(QDebug &d, const PresetItem &item)
{
    d << "name=" << item.name;
    d << "; category = " << item.categoryId;
    d << "; size = " << item.screenSizeName;

    return d;
}

inline bool operator==(const PresetItem &lhs, const PresetItem &rhs)
{
    return lhs.categoryId == rhs.categoryId && lhs.name == rhs.name;
}

struct WizardCategory
{
    QString id;
    QString name;
    std::vector<PresetItem> items;
};

inline QDebug &operator<<(QDebug &d, const WizardCategory &cat)
{
    d << "id=" << cat.id;
    d << "; name=" << cat.name;
    d << "; items=" << cat.items;

    return d;
}

using PresetsByCategory = std::map<QString, WizardCategory>;
using PresetItems = std::vector<PresetItem>;
using Categories = std::vector<QString>;

/****************** PresetData  ******************/

class PresetData
{
public:
    void setData(const PresetsByCategory &presets, const std::vector<RecentPreset> &recents);

    const std::vector<PresetItems> &presets() const { return m_presets; }
    const Categories &categories() const { return m_categories; }

private:
    std::vector<PresetItem> makeRecentPresets(const PresetItems &wizardPresets);

private:
    std::vector<PresetItems> m_presets;
    Categories m_categories;
    std::vector<RecentPreset> m_recents;
};

/****************** PresetCategoryModel ******************/

class BasePresetModel : public QAbstractListModel
{
public:
    BasePresetModel(const PresetData *data, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;

    void reset()
    {
        beginResetModel();
        endResetModel();
    }

protected:
    const PresetData *m_data = nullptr;
};

/****************** PresetCategoryModel ******************/

class PresetCategoryModel : public BasePresetModel
{
public:
    PresetCategoryModel(const PresetData *data, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};

/****************** PresetModel ******************/

class PresetModel : public BasePresetModel
{
    Q_OBJECT

public:
    PresetModel(const PresetData *data, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void setPage(int index); // called from QML when view's header item is clicked
    Q_INVOKABLE QString fontIconCode(int index) const;

    int page() const { return static_cast<int>(m_page); }

    Utils::optional<PresetItem> preset(size_t selection) const
    {
        auto presets = m_data->presets();
        if (presets.empty())
            return {};

        if (m_page < presets.size()) {
            const std::vector<PresetItem> presetsOfCategory = presets.at(m_page);
            if (selection < presetsOfCategory.size())
                return presets.at(m_page).at(selection);
        }
        return {};
    }

    bool empty() const { return m_data->presets().empty(); }

private:
    const std::vector<PresetItem> presetsOfCurrentCategory() const
    {
        return m_data->presets().at(m_page);
    }

    std::vector<PresetItems> presets() const { return m_data->presets(); }

private:
    size_t m_page = 0;
};

} // namespace StudioWelcome
