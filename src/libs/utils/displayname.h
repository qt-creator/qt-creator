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

#pragma once

#include "utils_global.h"

#include <QString>
#include <QVariantMap>

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

    void toMap(QVariantMap &map, const QString &key) const;
    void fromMap(const QVariantMap &map, const QString &key);

private:
    QString m_value;
    QString m_defaultValue;
};

bool QTCREATOR_UTILS_EXPORT operator==(const DisplayName &dn1, const DisplayName &dn2);
inline bool operator!=(const DisplayName &dn1, const DisplayName &dn2)
{
    return !(dn1 == dn2);
}

} // namespace Utils

