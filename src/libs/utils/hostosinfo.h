// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "osspecificaspects.h"

#include <optional>

#ifdef Q_OS_WIN
#define QTC_HOST_EXE_SUFFIX QTC_WIN_EXE_SUFFIX
#else
#define QTC_HOST_EXE_SUFFIX ""
#endif // Q_OS_WIN

namespace Utils { class FilePath; }

// The "Host" is the machine QtCreator is running on.

namespace Utils::HostOsInfo {

QTCREATOR_UTILS_EXPORT constexpr OsType hostOs()
{
#if defined(Q_OS_WIN)
    return OsTypeWindows;
#elif defined(Q_OS_LINUX)
    return OsTypeLinux;
#elif defined(Q_OS_MAC)
    return OsTypeMac;
#elif defined(Q_OS_UNIX)
    return OsTypeOtherUnix;
#else
    return OsTypeOther;
#endif
}

//! Returns the architecture of the host system.
QTCREATOR_UTILS_EXPORT OsArch hostArchitecture();

//! Returns the architecture the running binary was compiled for.
QTCREATOR_UTILS_EXPORT OsArch binaryArchitecture();

QTCREATOR_UTILS_EXPORT constexpr bool isWindowsHost() { return hostOs() == OsTypeWindows; }
QTCREATOR_UTILS_EXPORT constexpr bool isLinuxHost() { return hostOs() == OsTypeLinux; }
QTCREATOR_UTILS_EXPORT constexpr bool isMacHost() { return hostOs() == OsTypeMac; }
QTCREATOR_UTILS_EXPORT constexpr bool isAnyUnixHost()
{
#ifdef Q_OS_UNIX
    return true;
#else
    return false;
#endif
}

QTCREATOR_UTILS_EXPORT QString withExecutableSuffix(const QString &executable);

QTCREATOR_UTILS_EXPORT void setOverrideFileNameCaseSensitivity(Qt::CaseSensitivity sensitivity);
QTCREATOR_UTILS_EXPORT void unsetOverrideFileNameCaseSensitivity();

QTCREATOR_UTILS_EXPORT Qt::CaseSensitivity fileNameCaseSensitivity();

QTCREATOR_UTILS_EXPORT constexpr QChar pathListSeparator()
{
    return OsSpecificAspects::pathListSeparator(hostOs());
}

QTCREATOR_UTILS_EXPORT constexpr Qt::KeyboardModifier controlModifier()
{
    return OsSpecificAspects::controlModifier(hostOs());
}

QTCREATOR_UTILS_EXPORT std::optional<quint64> totalMemoryInstalledInBytes();

QTCREATOR_UTILS_EXPORT const FilePath &root(); // Do not use.

} // namespace Utils::HostOsInfo
