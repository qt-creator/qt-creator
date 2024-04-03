// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageutils_global.h"

QT_BEGIN_NAMESPACE
class QCryptographicHash;
QT_END_NAMESPACE

#include <QStringView>

namespace LanguageUtils {

class LANGUAGEUTILS_EXPORT ComponentVersion
{
    int _major;
    int _minor;

public:
    static const int NoVersion = -1;
    static const int MaxVersion = -1;

    ComponentVersion()
        : _major(NoVersion)
        , _minor(NoVersion)
    {}

    ComponentVersion(int major, int minor)
        : _major(major)
        , _minor(minor)
    {}

    explicit ComponentVersion(QStringView versionString);
    ~ComponentVersion() = default;

    int majorVersion() const { return _major; }
    int minorVersion() const { return _minor; }

    friend bool operator<(const ComponentVersion &lhs, const ComponentVersion &rhs)
    {
        return std::tie(lhs._major, lhs._minor) < std::tie(rhs._major, rhs._minor);
    }

    friend bool operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs)
    {
        return std::tie(lhs._major, lhs._minor) <= std::tie(rhs._major, rhs._minor);
    }

    friend bool operator>(const ComponentVersion &lhs, const ComponentVersion &rhs)
    {
        return rhs < lhs;
    }

    friend bool operator>=(const ComponentVersion &lhs, const ComponentVersion &rhs)
    {
        return rhs <= lhs;
    }

    friend bool operator==(const ComponentVersion &lhs, const ComponentVersion &rhs)
    {
        return lhs.majorVersion() == rhs.majorVersion() && lhs.minorVersion() == rhs.minorVersion();
    }

    friend bool operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs)
    {
        return !(lhs == rhs);
    }

    bool isValid() const { return _major >= 0 && _minor >= 0; }
    QString toString() const;
    void addToHash(QCryptographicHash &hash) const;
};

} // namespace LanguageUtils
