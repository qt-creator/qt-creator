// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageutils_global.h"

QT_BEGIN_NAMESPACE
class QCryptographicHash;
QT_END_NAMESPACE

namespace LanguageUtils {

class LANGUAGEUTILS_EXPORT ComponentVersion
{
    int _major;
    int _minor;

public:
    static const int NoVersion;
    static const int MaxVersion;

    ComponentVersion();
    ComponentVersion(int major, int minor);
    explicit ComponentVersion(const QString &versionString);
    ~ComponentVersion();

    int majorVersion() const { return _major; }
    int minorVersion() const { return _minor; }

    friend bool LANGUAGEUTILS_EXPORT operator<(const ComponentVersion &lhs, const ComponentVersion &rhs);
    friend bool LANGUAGEUTILS_EXPORT operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs);
    friend bool LANGUAGEUTILS_EXPORT operator>(const ComponentVersion &lhs, const ComponentVersion &rhs);
    friend bool LANGUAGEUTILS_EXPORT operator>=(const ComponentVersion &lhs, const ComponentVersion &rhs);
    friend bool LANGUAGEUTILS_EXPORT operator==(const ComponentVersion &lhs, const ComponentVersion &rhs);
    friend bool LANGUAGEUTILS_EXPORT operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs);

    bool isValid() const;
    QString toString() const;
    void addToHash(QCryptographicHash &hash) const;
};

} // namespace LanguageUtils
