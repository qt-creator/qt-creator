// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QDebug>
#include <QUrl>

#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include "userpresets.h"

#include <optional>

namespace Utils {
class Wizard;
}

namespace StudioWelcome {

struct UserPresetItem;

struct PresetItem
{
    PresetItem() = default;
    PresetItem(const QString &wizardName, const QString &category, const QString &sizeName = "")
        : wizardName{wizardName}
        , categoryId{category}
        , screenSizeName{sizeName}
    {}

    virtual ~PresetItem() {}
    virtual QString displayName() const { return wizardName; }
    virtual QString screenSize() const { return screenSizeName; }
    virtual std::unique_ptr<PresetItem> clone() const
    {
        return std::unique_ptr<PresetItem>{new PresetItem{*this}};
    }

    virtual bool isUserPreset() const { return false; }
    virtual UserPresetItem *asUserPreset() { return nullptr; }
    std::function<Utils::Wizard *(const Utils::FilePath &path)> create;

public:
    QString wizardName;
    QString categoryId;
    QString screenSizeName;
    QString description;
    QUrl qmlPath;
    QString fontIconCode;
};

struct UserPresetItem : public PresetItem
{
    UserPresetItem() = default;
    UserPresetItem(const QString &userName,
                   const QString &wizardName,
                   const QString &category,
                   const QString &sizeName = "")
        : PresetItem{wizardName, category, sizeName}
        , userName{userName}
    {}

    QString displayName() const override { return userName; }
    std::unique_ptr<PresetItem> clone() const override
    {
        return std::unique_ptr<PresetItem>{new UserPresetItem{*this}};
    }

    bool isUserPreset() const override { return true; }
    UserPresetItem *asUserPreset() override { return this; }

    bool isValid() const
    {
        return !categoryId.isEmpty() && !wizardName.isEmpty() && !userName.isEmpty();
    }

public:
    QString userName;
    bool useQtVirtualKeyboard;
    QString qtVersion;
    QString styleName;
};

inline QDebug &operator<<(QDebug &d, const UserPresetItem &item);

inline QDebug &operator<<(QDebug &d, const PresetItem &item)
{
    d << "wizardName=" << item.wizardName;
    d << "; category = " << item.categoryId;
    d << "; size = " << item.screenSizeName;

    if (item.isUserPreset())
        d << "; " << (UserPresetItem &) item;

    return d;
}

inline QDebug &operator<<(QDebug &d, const UserPresetItem &item)
{
    d << "userName=" << item.userName;

    return d;
}

inline bool operator==(const PresetItem &lhs, const PresetItem &rhs)
{
    return lhs.categoryId == rhs.categoryId && lhs.wizardName == rhs.wizardName;
}

using PresetItems = std::vector<std::shared_ptr<PresetItem>>;

struct WizardCategory
{
    QString id;
    QString name;
    PresetItems items;
};

inline QDebug &operator<<(QDebug &d, const std::shared_ptr<PresetItem> &preset)
{
    if (preset)
        d << *preset;
    else
        d << "(null)";

    return d;
}

inline QDebug &operator<<(QDebug &d, const WizardCategory &cat)
{
    d << "id=" << cat.id;
    d << "; name=" << cat.name;
    d << "; items=" << cat.items;

    return d;
}

using PresetsByCategory = std::map<QString, WizardCategory>;
using Categories = std::vector<QString>;

/****************** PresetData  ******************/

class PresetData
{
public:
    void reload(const std::vector<UserPresetData> &userPresets,
                const std::vector<UserPresetData> &loadedRecents);
    void setData(const PresetsByCategory &presets,
                 const std::vector<UserPresetData> &userPresets,
                 const std::vector<UserPresetData> &recents);

    const std::vector<PresetItems> &presets() const { return m_presets; }
    const Categories &categories() const { return m_categories; }

    static QString recentsTabName();

private:
    PresetItems makeUserPresets(const PresetItems &wizardPresets, const std::vector<UserPresetData> &data);
    std::shared_ptr<PresetItem> findPresetItemForUserPreset(const UserPresetData &preset, const PresetItems &wizardPresets);

private:
    std::vector<PresetItems> m_presets;
    Categories m_categories;
    std::vector<UserPresetData> m_recents;
    std::vector<UserPresetData> m_userPresets;
    PresetsByCategory m_presetsByCategory;
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

    const std::shared_ptr<PresetItem> preset(size_t selection) const
    {
        auto presets = m_data->presets();
        if (presets.empty())
            return {};

        if (m_page < presets.size()) {
            const PresetItems presetsOfCategory = presets.at(m_page);
            if (selection < presetsOfCategory.size())
                return presets.at(m_page).at(selection);
        }
        return {};
    }

    bool empty() const { return m_data->presets().empty(); }

private:
    const PresetItems presetsOfCurrentCategory() const
    {
        QTC_ASSERT(m_page < m_data->presets().size(), return {});

        return m_data->presets().at(m_page);
    }

    std::vector<PresetItems> presets() const { return m_data->presets(); }

private:
    size_t m_page = 0;
};

} // namespace StudioWelcome
