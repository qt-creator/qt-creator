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

// preset category, preset name, size name
using RecentPreset = std::tuple<QString, QString, QString>;

class RecentPresetsStore
{
public:
    explicit RecentPresetsStore(QSettings *settings)
        : m_settings{settings}
    {}

    void setMaximum(int n) { m_max = n; }
    void add(const QString &categoryId, const QString &name, const QString &sizeName);
    std::vector<RecentPreset> fetchAll() const;

private:
    QStringList addRecentToExisting(const RecentPreset &preset, std::vector<RecentPreset> &recents);
    static QStringList encodeRecentPresets(const std::vector<RecentPreset> &recents);
    static std::vector<RecentPreset> decodeRecentPresets(const QVariantList &values);
    static RecentPreset decodeOneRecentPreset(const QString &encoded);

private:
    QSettings *m_settings = nullptr;
    int m_max = 10;
};

} // namespace StudioWelcome
