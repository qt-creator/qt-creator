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

using Core::ICore;
using Utils::QtcSettings;

using namespace StudioWelcome;

constexpr char GROUP_NAME[] = "RecentPresets";
constexpr char WIZARDS[] = "Wizards";

void RecentPresetsStore::add(const QString &categoryId, const QString &name, const QString &sizeName)
{
    std::vector<RecentPreset> existing = fetchAll();
    QStringList encodedRecents = addRecentToExisting(RecentPreset{categoryId, name, sizeName},
                                                     existing);

    m_settings->beginGroup(GROUP_NAME);
    m_settings->setValue(WIZARDS, encodedRecents);
    m_settings->endGroup();
    m_settings->sync();
}

QStringList RecentPresetsStore::addRecentToExisting(const RecentPreset &preset,
                                                    std::vector<RecentPreset> &recents)
{
    Utils::erase_one(recents, preset);
    Utils::prepend(recents, preset);

    if (int(recents.size()) > m_max)
        recents.pop_back();

    return encodeRecentPresets(recents);
}

std::vector<RecentPreset> RecentPresetsStore::fetchAll() const
{
    m_settings->beginGroup(GROUP_NAME);
    QVariant value = m_settings->value(WIZARDS);
    m_settings->endGroup();

    std::vector<RecentPreset> result;

    if (value.type() == QVariant::String)
        result.push_back(decodeOneRecentPreset(value.toString()));
    else if (value.type() == QVariant::StringList)
        Utils::concat(result, decodeRecentPresets(value.toList()));

    const RecentPreset empty;
    return Utils::filtered(result, [&empty](const RecentPreset &recent) { return recent != empty; });
}

QStringList RecentPresetsStore::encodeRecentPresets(const std::vector<RecentPreset> &recents)
{
    return Utils::transform<QList>(recents, [](const RecentPreset &p) -> QString {
        return std::get<0>(p) + "/" + std::get<1>(p) + ":" + std::get<2>(p);
    });
}

RecentPreset RecentPresetsStore::decodeOneRecentPreset(const QString &encoded)
{
    QRegularExpression pattern{R"(^(\S+)/(.+):(\d+ x \d+))"};
    auto m = pattern.match(encoded);
    if (!m.hasMatch())
        return RecentPreset{};

    QString category = m.captured(1);
    QString name = m.captured(2);
    QString size = m.captured(3);

    return std::make_tuple(category, name, size);
}

std::vector<RecentPreset> RecentPresetsStore::decodeRecentPresets(const QVariantList &values)
{
    return Utils::transform<std::vector>(values, [](const QVariant &value) {
        return decodeOneRecentPreset(value.toString());
    });
}
