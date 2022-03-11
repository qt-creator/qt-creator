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

#include <vector>
#include <QPair>
#include <QSettings>

namespace StudioWelcome {

struct RecentPresetData
{
    RecentPresetData() = default;
    RecentPresetData(const QString &category,
                 const QString &name,
                 const QString &size,
                 bool isUserPreset = false)
        : category{category}
        , presetName{name}
        , sizeName{size}
        , isUserPreset{isUserPreset}
    {}

    QString category;
    QString presetName;
    QString sizeName;
    bool isUserPreset = false;
};

inline bool operator==(const RecentPresetData &lhs, const RecentPresetData &rhs)
{
    return lhs.category == rhs.category && lhs.presetName == rhs.presetName
           && lhs.sizeName == rhs.sizeName && lhs.isUserPreset == rhs.isUserPreset;
}

inline bool operator!=(const RecentPresetData &lhs, const RecentPresetData &rhs)
{
    return !(lhs == rhs);
}

inline QDebug &operator<<(QDebug &d, const RecentPresetData &preset)
{
    d << "RecentPreset{category=" << preset.category << "; name=" << preset.presetName
      << "; size=" << preset.sizeName << "; isUserPreset=" << preset.isUserPreset << "}";

    return d;
}

class RecentPresetsStore
{
public:
    explicit RecentPresetsStore(QSettings *settings)
        : m_settings{settings}
    {}

    void setMaximum(int n) { m_max = n; }
    void add(const QString &categoryId,
             const QString &name,
             const QString &sizeName,
             bool isUserPreset = false);

    std::vector<RecentPresetData> remove(const QString &categoryId, const QString &presetName);
    std::vector<RecentPresetData> fetchAll() const;

private:
    std::vector<RecentPresetData> addRecentToExisting(const RecentPresetData &preset,
                                                      std::vector<RecentPresetData> &recents);
    static QStringList encodeRecentPresets(const std::vector<RecentPresetData> &recents);
    static std::vector<RecentPresetData> decodeRecentPresets(const QVariantList &values);
    static RecentPresetData decodeOneRecentPreset(const QString &encoded);
    void save(const std::vector<RecentPresetData> &recents);

private:
    QSettings *m_settings = nullptr;
    int m_max = 10;
};

} // namespace StudioWelcome
