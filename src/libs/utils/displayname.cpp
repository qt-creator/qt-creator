/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

void DisplayName::toMap(QVariantMap &map, const QString &key) const
{
    if (!usesDefaultValue())
        map.insert(key, m_value);
}

void DisplayName::fromMap(const QVariantMap &map, const QString &key)
{
    m_value = map.value(key).toString();
}

bool operator==(const DisplayName &dn1, const DisplayName &dn2)
{
    return dn1.value() == dn2.value() && dn1.defaultValue() == dn2.defaultValue();
}

} // namespace Utils
