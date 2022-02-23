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

#include "userpresets.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace StudioWelcome;

constexpr char PREFIX[] = "UserPresets";

UserPresetsStore::UserPresetsStore()
{
    m_settings = std::make_unique<QSettings>(fullFilePath(), QSettings::IniFormat);
}

UserPresetsStore::UserPresetsStore(std::unique_ptr<QSettings> &&settings)
    : m_settings{std::move(settings)}
{}

void UserPresetsStore::savePresets(const std::vector<UserPresetData> &presets)
{
    m_settings->beginWriteArray(PREFIX, static_cast<int>(presets.size()));

    for (size_t i = 0; i < presets.size(); ++i) {
        m_settings->setArrayIndex(static_cast<int>(i));
        const auto &preset = presets[i];

        m_settings->setValue("categoryId", preset.categoryId);
        m_settings->setValue("wizardName", preset.wizardName);
        m_settings->setValue("name", preset.name);
        m_settings->setValue("screenSize", preset.screenSize);
        m_settings->setValue("useQtVirtualKeyboard", preset.useQtVirtualKeyboard);
        m_settings->setValue("qtVersion", preset.qtVersion);
        m_settings->setValue("styleName", preset.styleName);
    }
    m_settings->endArray();
    m_settings->sync();

}

bool UserPresetsStore::save(const UserPresetData &newPreset)
{
    QTC_ASSERT(newPreset.isValid(), return false);

    std::vector<UserPresetData> presetItems = fetchAll();
    if (Utils::anyOf(presetItems,
                     [&newPreset](const UserPresetData &p) { return p.name == newPreset.name; })) {
        return false;
    }

    presetItems.push_back(newPreset);
    savePresets(presetItems);

    return true;
}

void UserPresetsStore::remove(const QString &category, const QString &name)
{
    std::vector<UserPresetData> presetItems = fetchAll();
    auto item = Utils::take(presetItems, [&](const UserPresetData &p) {
        return p.categoryId == category && p.name == name;
    });

    if (!item)
        return;

    savePresets(presetItems);
}

std::vector<UserPresetData> UserPresetsStore::fetchAll() const
{
    std::vector<UserPresetData> result;
    int size = m_settings->beginReadArray(PREFIX);
    if (size >= 0)
        result.reserve(static_cast<size_t>(size) + 1);

    for (int i = 0; i < size; ++i) {
        m_settings->setArrayIndex(i);

        UserPresetData preset;
        preset.categoryId = m_settings->value("categoryId").toString();
        preset.wizardName = m_settings->value("wizardName").toString();
        preset.name = m_settings->value("name").toString();
        preset.screenSize = m_settings->value("screenSize").toString();
        preset.useQtVirtualKeyboard = m_settings->value("useQtVirtualKeyboard").toBool();
        preset.qtVersion = m_settings->value("qtVersion").toString();
        preset.styleName = m_settings->value("styleName").toString();

        if (preset.isValid())
            result.push_back(std::move(preset));
    }
    m_settings->endArray();

    return result;
}

QString UserPresetsStore::fullFilePath() const
{
    return Core::ICore::userResourcePath("UserPresets.ini").toString();
}
