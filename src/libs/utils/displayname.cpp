// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "displayname.h"

namespace Utils {

bool DisplayName::setValue(const QString &name)
{
    if (value() == name)
        return false;
    if (name == m_defaultValue)
        m_value.clear();
    else
        m_value = name;
    return true;
}

bool DisplayName::setDefaultValue(const QString &name)
{
    if (m_defaultValue == name)
        return false;
    const QString originalName = value();
    m_defaultValue = name;
    return originalName != value();
}

QString DisplayName::value() const
{
    return m_value.isEmpty() ? m_defaultValue : m_value;
}

bool DisplayName::usesDefaultValue() const
{
    return m_value.isEmpty();
}

void DisplayName::toMap(Store &map, const Key &key) const
{
    if (m_forceSerialization || !usesDefaultValue())
        map.insert(key, m_value);
}

void DisplayName::fromMap(const Store &map, const Key &key)
{
    m_value = map.value(key).toString();
}

bool operator==(const DisplayName &dn1, const DisplayName &dn2)
{
    return dn1.value() == dn2.value() && dn1.defaultValue() == dn2.defaultValue();
}

} // namespace Utils
