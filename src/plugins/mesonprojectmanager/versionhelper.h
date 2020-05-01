/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include <QRegularExpression>
#include <QString>

namespace MesonProjectManager {
namespace Internal {
struct Version
{
    int major = -1;
    int minor = -1;
    int patch = -1;
    bool isValid = false;
    Version() = default;
    Version(const Version &) = default;
    Version(Version &&) = default;
    Version &operator=(const Version &) = default;
    Version &operator=(Version &&) = default;

    bool operator==(const Version &other)
    {
        return other.isValid && isValid && major == other.major && minor == other.minor
               && patch == other.patch;
    }

    Version(int major, int minor, int patch)
        : major{major}
        , minor{minor}
        , patch{patch}
        , isValid{major != -1 && minor != -1 && patch != -1}
    {}
    QString toQString() const noexcept
    {
        return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
    }
    static inline Version fromString(const QString &str)
    {
        QRegularExpression regex{R"((\d+).(\d+).(\d+))"};
        auto matched = regex.match(str);
        if (matched.hasMatch())
            return Version{matched.captured(1).toInt(),
                           matched.captured(2).toInt(),
                           matched.captured(3).toInt()};
        return Version{};
    }
};

} // namespace Internal
} // namespace MesonProjectManager
