// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "osspecificaspects.h"

#include <optional>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

#ifdef Q_OS_WIN
#define QTC_HOST_EXE_SUFFIX QTC_WIN_EXE_SUFFIX
#else
#define QTC_HOST_EXE_SUFFIX ""
#endif // Q_OS_WIN

namespace Utils {

class FilePath;

class QTCREATOR_UTILS_EXPORT HostOsInfo
{
public:
    static constexpr OsType hostOs()
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
    static OsArch hostArchitecture();

    //! Returns the architecture the running binary was compiled for.
    static constexpr OsArch binaryArchitecture()
    {
// MSVC:
#if defined(_M_X64)
        return OsArchAMD64;
#elif defined(_M_IX86)
        return OsArchX86;
#elif defined(_M_ARM64)
        return OsArchArm64;
#elif defined(_M_ARM)
        return OsArchArm;
#elif defined(_M_IA64)
        return OsArchItanium;
// GCC / Clang:
#elif defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
        return OsArchAMD64;
#elif defined(__i386__) || defined(__i386) || defined(__i486__) || defined(__i586__) \
    || defined(__i686__)
        return OsArchX86;
#elif defined(__aarch64__)
        return OsArchArm64;
#elif defined(__arm__)
        return OsArchArm;
#elif defined(__ia64__) || defined(__itanium__)
        return OsArchItanium;
#else
        static_assert(false, "Unknown architecture, please add detection.");
        return OsArchUnknown;
#endif
    }

    static constexpr bool isArm64Binary() { return binaryArchitecture() == OsArchArm64; }
    static constexpr bool isAmd64Binary() { return binaryArchitecture() == OsArchAMD64; }

    static constexpr bool isWindowsHost() { return hostOs() == OsTypeWindows; }
    static constexpr bool isLinuxHost() { return hostOs() == OsTypeLinux; }
    static constexpr bool isMacHost() { return hostOs() == OsTypeMac; }
    static constexpr bool isAnyUnixHost()
    {
#ifdef Q_OS_UNIX
        return true;
#else
        return false;
#endif
    }

    static QString withExecutableSuffix(const QString &executable)
    {
        return OsSpecificAspects::withExecutableSuffix(hostOs(), executable);
    }

    static void setOverrideFileNameCaseSensitivity(Qt::CaseSensitivity sensitivity);
    static void unsetOverrideFileNameCaseSensitivity();

    static Qt::CaseSensitivity fileNameCaseSensitivity()
    {
        return m_useOverrideFileNameCaseSensitivity
                ? m_overrideFileNameCaseSensitivity
                : OsSpecificAspects::fileNameCaseSensitivity(hostOs());
    }

    static constexpr QChar pathListSeparator()
    {
        return OsSpecificAspects::pathListSeparator(hostOs());
    }

    static constexpr Qt::KeyboardModifier controlModifier()
    {
        return OsSpecificAspects::controlModifier(hostOs());
    }

    static std::optional<quint64> totalMemoryInstalledInBytes();

    static const FilePath &root();

private:
    static Qt::CaseSensitivity m_overrideFileNameCaseSensitivity;
    static bool m_useOverrideFileNameCaseSensitivity;
};

} // namespace Utils
