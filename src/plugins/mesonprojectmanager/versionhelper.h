// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QRegularExpression>

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
