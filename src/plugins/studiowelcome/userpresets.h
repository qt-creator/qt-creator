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
#include <QSettings>

namespace StudioWelcome {

struct UserPresetData
{
    QString categoryId;
    QString wizardName;
    QString name;
    QString screenSize;

    bool useQtVirtualKeyboard;
    QString qtVersion;
    QString styleName;

    bool isValid() const
    {
        return !categoryId.isEmpty()
            && !wizardName.isEmpty()
            && !name.isEmpty();
    }
};

inline QDebug &operator<<(QDebug &d, const UserPresetData &preset)
{
    d << "UserPreset{category = " << preset.categoryId;
    d << "; wizardName = " << preset.wizardName;
    d << "; name = " << preset.name;
    d << "; screenSize = " << preset.screenSize;
    d << "; keyboard = " << preset.useQtVirtualKeyboard;
    d << "; qt = " << preset.qtVersion;
    d << "; style = " << preset.styleName;
    d << "}";

    return d;
}

inline bool operator==(const UserPresetData &lhs, const UserPresetData &rhs)
{
    return lhs.categoryId == rhs.categoryId && lhs.wizardName == rhs.wizardName
           && lhs.name == rhs.name && lhs.screenSize == rhs.screenSize
           && lhs.useQtVirtualKeyboard == rhs.useQtVirtualKeyboard && lhs.qtVersion == rhs.qtVersion
           && lhs.styleName == rhs.styleName;
}

class UserPresetsStore
{
public:
    UserPresetsStore();
    UserPresetsStore(std::unique_ptr<QSettings> &&settings);

    bool save(const UserPresetData &preset);
    void remove(const QString &category, const QString &name);
    std::vector<UserPresetData> fetchAll() const;

private:
    QString fullFilePath() const;
    void savePresets(const std::vector<UserPresetData> &presets);

    std::unique_ptr<QSettings> m_settings;
};

} // namespace StudioWelcome
