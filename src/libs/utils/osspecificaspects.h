/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#define QTC_WIN_EXE_SUFFIX ".exe"

namespace Utils {

// Add more as needed.
enum OsType { OsTypeWindows, OsTypeLinux, OsTypeMac, OsTypeOtherUnix, OsTypeOther };

class QTCREATOR_UTILS_EXPORT OsSpecificAspects
{
public:
    OsSpecificAspects(OsType osType) : m_osType(osType) { }

    QString withExecutableSuffix(const QString &executable) const {
        QString finalName = executable;
        if (m_osType == OsTypeWindows)
            finalName += QLatin1String(QTC_WIN_EXE_SUFFIX);
        return finalName;
    }

    Qt::CaseSensitivity fileNameCaseSensitivity() const {
        return m_osType == OsTypeWindows || m_osType == OsTypeMac ? Qt::CaseInsensitive : Qt::CaseSensitive;
    }

    QChar pathListSeparator() const {
        return QLatin1Char(m_osType == OsTypeWindows ? ';' : ':');
    }

    Qt::KeyboardModifier controlModifier() const {
        return m_osType == OsTypeMac ? Qt::MetaModifier : Qt::ControlModifier;
    }

private:
    const OsType m_osType;
};

} // namespace Utils
