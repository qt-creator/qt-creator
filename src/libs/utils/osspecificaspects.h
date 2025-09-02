// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "result.h"

#include <QDebug>
#include <QString>

#include <algorithm>

#define QTC_WIN_EXE_SUFFIX ".exe"

namespace Utils {

// Add more as needed.
enum OsType { OsTypeWindows, OsTypeLinux, OsTypeMac, OsTypeOtherUnix, OsTypeOther };

enum OsArch { OsArchUnknown, OsArchX86, OsArchAMD64, OsArchItanium, OsArchArm, OsArchArm64 };

inline QString osTypeToString(OsType osType)
{
    switch (osType) {
    case OsTypeWindows:
        return "Windows";
    case OsTypeLinux:
        return "Linux";
    case OsTypeMac:
        return "Mac";
    case OsTypeOtherUnix:
        return "Other Unix";
    case OsTypeOther:
    default:
        return "Other";
    }
}

inline Utils::Result<OsType> osTypeFromString(const QString &string)
{
    if (string.compare("windows", Qt::CaseInsensitive) == 0)
        return OsTypeWindows;
    if (string.compare("linux", Qt::CaseInsensitive) == 0)
        return OsTypeLinux;
    if (string.compare("mac", Qt::CaseInsensitive) == 0
        || string.compare("darwin", Qt::CaseInsensitive) == 0
        || string.compare("macos", Qt::CaseInsensitive) == 0)
        return OsTypeMac;
    if (string.compare("other unix", Qt::CaseInsensitive) == 0)
        return OsTypeOtherUnix;

    return Utils::ResultError(QString::fromLatin1("Unknown os type: %1").arg(string));
}

inline Utils::Result<OsArch> osArchFromString(const QString &architecture)
{
    if (architecture == QLatin1String("x86_64") || architecture == QLatin1String("amd64"))
        return OsArchAMD64;
    if (architecture == QLatin1String("x86"))
        return OsArchX86;
    if (architecture == QLatin1String("ia64"))
        return OsArchItanium;
    if (architecture == QLatin1String("arm"))
        return OsArchArm;
    if (architecture == QLatin1String("arm64") || architecture == QLatin1String("aarch64"))
        return OsArchArm64;

    return Utils::ResultError(QString::fromLatin1("Unknown architecture: %1").arg(architecture));
}

namespace OsSpecificAspects {

inline QString withExecutableSuffix(OsType osType, const QString &executable)
{
    QString finalName = executable;
    if (osType == OsTypeWindows && !finalName.endsWith(QTC_WIN_EXE_SUFFIX))
        finalName += QLatin1String(QTC_WIN_EXE_SUFFIX);
    return finalName;
}

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

inline QString pathWithNativeSeparators(OsType osType, const QString &pathName)
{
    if (osType == OsTypeWindows) {
        const int pos = pathName.indexOf('/');
        if (pos >= 0) {
            QString n = pathName;
            std::replace(std::begin(n) + pos, std::end(n), '/', '\\');
            return n;
        }
    } else {
        const int pos = pathName.indexOf('\\');
        if (pos >= 0) {
            QString n = pathName;
            std::replace(std::begin(n) + pos, std::end(n), '\\', '/');
            return n;
        }
    }
    return pathName;
}

} // namespace OsSpecificAspects
} // namespace Utils
