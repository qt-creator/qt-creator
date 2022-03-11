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

#include "recentpresets.h"
#include "algorithm.h"

#include <QRegularExpression>

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

using namespace StudioWelcome;

constexpr char GROUP_NAME[] = "RecentPresets";
constexpr char WIZARDS[] = "Wizards";

void RecentPresetsStore::add(const QString &categoryId,
                             const QString &name,
                             const QString &sizeName,
                             bool isUserPreset)
{
    std::vector<RecentPresetData> existing = fetchAll();

    std::vector<RecentPresetData> recents
        = addRecentToExisting(RecentPresetData{categoryId, name, sizeName, isUserPreset}, existing);

    save(recents);
}

void RecentPresetsStore::save(const std::vector<RecentPresetData> &recents)
{
    QStringList encodedRecents = encodeRecentPresets(recents);

    m_settings->beginGroup(GROUP_NAME);
    m_settings->setValue(WIZARDS, encodedRecents);
    m_settings->endGroup();
    m_settings->sync();
}

std::vector<RecentPresetData> RecentPresetsStore::remove(const QString &categoryId, const QString &presetName)
{
    std::vector<RecentPresetData> recents = fetchAll();
    size_t countBefore = recents.size();

    /* NOTE: when removing one preset, it may happen that there are more than one recent for that
     * preset. In that case, we need to remove all associated recents, for the preset.*/

    Utils::erase(recents, [&](const RecentPresetData &p) {
        return p.category == categoryId && p.presetName == presetName;
    });

    if (recents.size() < countBefore)
        save(recents);

    return recents;
}

std::vector<RecentPresetData> RecentPresetsStore::addRecentToExisting(
    const RecentPresetData &preset, std::vector<RecentPresetData> &recents)
{
    Utils::erase_one(recents, preset);
    Utils::prepend(recents, preset);

    if (int(recents.size()) > m_max)
        recents.pop_back();

    return recents;
}

std::vector<RecentPresetData> RecentPresetsStore::fetchAll() const
{
    m_settings->beginGroup(GROUP_NAME);
    QVariant value = m_settings->value(WIZARDS);
    m_settings->endGroup();

    std::vector<RecentPresetData> result;

    if (value.type() == QVariant::String)
        result.push_back(decodeOneRecentPreset(value.toString()));
    else if (value.type() == QVariant::StringList)
        Utils::concat(result, decodeRecentPresets(value.toList()));

    const RecentPresetData empty;
    return Utils::filtered(result, [&empty](const RecentPresetData &recent) { return recent != empty; });
}

QStringList RecentPresetsStore::encodeRecentPresets(const std::vector<RecentPresetData> &recents)
{
    return Utils::transform<QList>(recents, [](const RecentPresetData &p) -> QString {
        QString name = p.presetName;
        if (p.isUserPreset)
            name.prepend("[U]");

        return p.category + "/" + name + ":" + p.sizeName;
    });
}

RecentPresetData RecentPresetsStore::decodeOneRecentPreset(const QString &encoded)
{
    QRegularExpression pattern{R"(^(\S+)/(.+):(\d+ x \d+)$)"};
    auto m = pattern.match(encoded);
    if (!m.hasMatch())
        return RecentPresetData{};

    QString category = m.captured(1);
    QString name = m.captured(2);
    QString size = m.captured(3);
    bool isUserPreset = name.startsWith("[U]");
    if (isUserPreset)
        name = name.split("[U]")[1];

    if (!QRegularExpression{R"(^\w[\w ]*$)"}.match(name).hasMatch())
        return RecentPresetData{};

    RecentPresetData result;
    result.category = category;
    result.presetName = name;
    result.sizeName = size;
    result.isUserPreset = isUserPreset;

    return result;
}

std::vector<RecentPresetData> RecentPresetsStore::decodeRecentPresets(const QVariantList &values)
{
    return Utils::transform<std::vector>(values, [](const QVariant &value) {
        return decodeOneRecentPreset(value.toString());
    });
}
