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

#include "winutils.h"
#include "qtcassert.h"

// Enable WinAPI Windows XP and later
#ifdef Q_OS_WIN
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#include <windows.h>
#endif

#include <QString>
#include <QVector>
#include <QDebug>
#include <QLibrary>
#include <QTextStream>
#include <QDir>

namespace Utils {

QTCREATOR_UTILS_EXPORT QString winErrorMessage(unsigned long error)
{
    QString rc = QString::fromLatin1("#%1: ").arg(error);
#ifdef Q_OS_WIN
    ushort *lpMsgBuf;

    const int len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (len) {
        rc = QString::fromUtf16(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
    } else {
        rc += QString::fromLatin1("<unknown error>");
    }
#endif
    return rc;
}


#ifdef Q_OS_WIN
static inline QString msgCannotLoad(const char *lib, const QString &why)
{
    return QString::fromLatin1("Unable load %1: %2").arg(QLatin1String(lib), why);
}

static inline QString msgCannotResolve(const char *lib)
{
    return QString::fromLatin1("Unable to resolve all required symbols in  %1").arg(QLatin1String(lib));
}
#endif

QTCREATOR_UTILS_EXPORT QString winGetDLLVersion(WinDLLVersionType t,
                                                const QString &name,
                                                QString *errorMessage)
{
#ifdef Q_OS_WIN
    // Resolve required symbols from the version.dll
    typedef DWORD (APIENTRY *GetFileVersionInfoSizeProtoType)(LPCTSTR, LPDWORD);
    typedef BOOL (APIENTRY *GetFileVersionInfoWProtoType)(LPCWSTR, DWORD, DWORD, LPVOID);
    typedef BOOL (APIENTRY *VerQueryValueWProtoType)(const LPVOID, LPWSTR lpSubBlock, LPVOID, PUINT);

    const char *versionDLLC = "version.dll";
    QLibrary versionLib(QLatin1String(versionDLLC), 0);
    if (!versionLib.load()) {
        *errorMessage = msgCannotLoad(versionDLLC, versionLib.errorString());
        return QString();
    }
    // MinGW requires old-style casts
    GetFileVersionInfoSizeProtoType getFileVersionInfoSizeW = (GetFileVersionInfoSizeProtoType)(versionLib.resolve("GetFileVersionInfoSizeW"));
    GetFileVersionInfoWProtoType getFileVersionInfoW = (GetFileVersionInfoWProtoType)(versionLib.resolve("GetFileVersionInfoW"));
    VerQueryValueWProtoType verQueryValueW = (VerQueryValueWProtoType)(versionLib.resolve("VerQueryValueW"));
    if (!getFileVersionInfoSizeW || !getFileVersionInfoW || !verQueryValueW) {
        *errorMessage = msgCannotResolve(versionDLLC);
        return QString();
    }

    // Now go ahead, read version info resource
    DWORD dummy = 0;
    const LPCTSTR fileName = reinterpret_cast<LPCTSTR>(name.utf16()); // MinGWsy
    const DWORD infoSize = (*getFileVersionInfoSizeW)(fileName, &dummy);
    if (infoSize == 0) {
        *errorMessage = QString::fromLatin1("Unable to determine the size of the version information of %1: %2").arg(name, winErrorMessage(GetLastError()));
        return QString();
    }
    QByteArray dataV(infoSize + 1, '\0');
    char *data = dataV.data();
    if (!(*getFileVersionInfoW)(fileName, dummy, infoSize, data)) {
        *errorMessage = QString::fromLatin1("Unable to determine the version information of %1: %2").arg(name, winErrorMessage(GetLastError()));
        return QString();
    }
    VS_FIXEDFILEINFO  *versionInfo;
    const LPCWSTR backslash = TEXT("\\");
    UINT len = 0;
    if (!(*verQueryValueW)(data, const_cast<LPWSTR>(backslash), &versionInfo, &len)) {
        *errorMessage = QString::fromLatin1("Unable to determine version string of %1: %2").arg(name, winErrorMessage(GetLastError()));
        return QString();
    }
    QString rc;
    switch (t) {
    case WinDLLFileVersion:
        QTextStream(&rc) << HIWORD(versionInfo->dwFileVersionMS) << '.' << LOWORD(versionInfo->dwFileVersionMS);
        break;
    case WinDLLProductVersion:
        QTextStream(&rc) << HIWORD(versionInfo->dwProductVersionMS) << '.' << LOWORD(versionInfo->dwProductVersionMS);
        break;
    }
    return rc;
#endif
    Q_UNUSED(t);
    Q_UNUSED(name);
    Q_UNUSED(errorMessage);
    return QString();
}

QTCREATOR_UTILS_EXPORT bool is64BitWindowsSystem()
{
#ifdef Q_OS_WIN
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo(&systemInfo);
    return systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64
            || systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64;
#else
    return false;
#endif
}

QTCREATOR_UTILS_EXPORT bool is64BitWindowsBinary(const QString &binaryIn)
{
       QTC_ASSERT(!binaryIn.isEmpty(), return false);
#ifdef Q_OS_WIN32
#  ifdef __GNUC__   // MinGW lacking some definitions/winbase.h
#    define SCS_64BIT_BINARY 6
#  endif
        bool isAmd64 = false;
        DWORD binaryType = 0;
        const QString binary = QDir::toNativeSeparators(binaryIn);
        bool success = GetBinaryTypeW(reinterpret_cast<const TCHAR*>(binary.utf16()), &binaryType) != 0;
        if (success && binaryType == SCS_64BIT_BINARY)
            isAmd64=true;
        return isAmd64;
#else
        return false;
#endif
}

WindowsCrashDialogBlocker::WindowsCrashDialogBlocker()
#ifdef Q_OS_WIN
    : silenceErrorMode(SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS),
    originalErrorMode(SetErrorMode(silenceErrorMode))
#endif
{
}

WindowsCrashDialogBlocker::~WindowsCrashDialogBlocker()
{
#ifdef Q_OS_WIN
    unsigned int errorMode = SetErrorMode(originalErrorMode);
    // someone else messed with the error mode in between? Better not touch ...
    QTC_ASSERT(errorMode == silenceErrorMode, SetErrorMode(errorMode));
#endif
}


} // namespace Utils
