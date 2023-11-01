// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "store.h"

namespace Utils {

// Can be used for anything with a translatable, user-settable name with a fixed default value
// that gets set by a constructor or factory.
class QTCREATOR_UTILS_EXPORT DisplayName
{
public:
    // These return true if and only if the value of displayName() has changed.
    bool setValue(const QString &name);
    bool setDefaultValue(const QString &name);

    QString value() const;
    QString defaultValue() const { return m_defaultValue; }
    bool usesDefaultValue() const;
    void forceSerialization() { m_forceSerialization = true; }

    void toMap(Store &map, const Key &key) const;
    void fromMap(const Store &map, const Key &key);

private:
    QString m_value;
    QString m_defaultValue;
    bool m_forceSerialization = false;
};

bool QTCREATOR_UTILS_EXPORT operator==(const DisplayName &dn1, const DisplayName &dn2);
inline bool operator!=(const DisplayName &dn1, const DisplayName &dn2)
{
    return !(dn1 == dn2);
}

} // namespace Utils

