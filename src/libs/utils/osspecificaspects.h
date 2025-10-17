// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "result.h"
#include "utils_global.h"

#include <QString>

#define QTC_WIN_EXE_SUFFIX ".exe"

namespace Utils {

// Add more as needed.
enum OsType { OsTypeWindows, OsTypeLinux, OsTypeMac, OsTypeOtherUnix, OsTypeOther };

enum OsArch { OsArchUnknown, OsArchX86, OsArchAMD64, OsArchItanium, OsArchArm, OsArchArm64 };

QTCREATOR_UTILS_EXPORT QString osTypeToString(OsType osType);
QTCREATOR_UTILS_EXPORT Utils::Result<OsType> osTypeFromString(const QString &string);
QTCREATOR_UTILS_EXPORT Utils::Result<OsArch> osArchFromString(const QString &architecture);

namespace OsSpecificAspects {

QTCREATOR_UTILS_EXPORT QString withExecutableSuffix(OsType osType, const QString &executable);

constexpr Qt::CaseSensitivity fileNameCaseSensitivity(OsType osType)
{
    return osType == OsTypeWindows || osType == OsTypeMac ? Qt::CaseInsensitive : Qt::CaseSensitive;
}

constexpr Qt::CaseSensitivity envVarCaseSensitivity(OsType osType)
{
    return fileNameCaseSensitivity(osType);
}

constexpr QChar pathListSeparator(OsType osType)
{
    return QLatin1Char(osType == OsTypeWindows ? ';' : ':');
}

constexpr Qt::KeyboardModifier controlModifier(OsType osType)
{
    return osType == OsTypeMac ? Qt::MetaModifier : Qt::ControlModifier;
}

QTCREATOR_UTILS_EXPORT QString pathWithNativeSeparators(OsType osType, const QString &pathName);

} // namespace OsSpecificAspects
} // namespace Utils
