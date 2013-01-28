/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "winutils.h"
#include "qtcassert.h"

// Enable WinAPI Windows XP and later
#define _WIN32_WINNT 0x0501
#include <windows.h>

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
    return rc;
}


static inline QString msgCannotLoad(const char *lib, const QString &why)
{
    return QString::fromLatin1("Unable load %1: %2").arg(QLatin1String(lib), why);
}

static inline QString msgCannotResolve(const char *lib)
{
    return QString::fromLatin1("Unable to resolve all required symbols in  %1").arg(QLatin1String(lib));
}

QTCREATOR_UTILS_EXPORT QString winGetDLLVersion(WinDLLVersionType t,
                                                const QString &name,
                                                QString *errorMessage)
{
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
}

QTCREATOR_UTILS_EXPORT QString getShortPathName(const QString &name)
{
    if (name.isEmpty())
        return name;

    // Determine length, then convert.
    const LPCTSTR nameC = reinterpret_cast<LPCTSTR>(name.utf16()); // MinGW
    const DWORD length = GetShortPathNameW(nameC, NULL, 0);
    if (length == 0)
        return name;
    QScopedArrayPointer<TCHAR> buffer(new TCHAR[length]);
    GetShortPathNameW(nameC, buffer.data(), length);
    const QString rc = QString::fromUtf16(reinterpret_cast<const ushort *>(buffer.data()), length - 1);
    return rc;
}

QTCREATOR_UTILS_EXPORT QString getLongPathName(const QString &name)
{
    if (name.isEmpty())
        return name;

    // Determine length, then convert.
    const LPCTSTR nameC = reinterpret_cast<LPCTSTR>(name.utf16()); // MinGW
    const DWORD length = GetLongPathNameW(nameC, NULL, 0);
    if (length == 0)
        return name;
    QScopedArrayPointer<TCHAR> buffer(new TCHAR[length]);
    GetLongPathNameW(nameC, buffer.data(), length);
    const QString rc = QString::fromUtf16(reinterpret_cast<const ushort *>(buffer.data()), length - 1);
    return rc;
}

// makes sure that capitalization of directories is canonical.
// This mimics the logic in QDeclarative_isFileCaseCorrect
QTCREATOR_UTILS_EXPORT QString normalizePathName(const QString &name)
{
    QString canonicalName = getShortPathName(name);
    if (canonicalName.isEmpty())
        return name;
    canonicalName = getLongPathName(canonicalName);
    if (canonicalName.isEmpty())
        return name;
    // Upper case drive letter
    if (canonicalName.size() > 2 && canonicalName.at(1) == QLatin1Char(':'))
        canonicalName[0] = canonicalName.at(0).toUpper();
    return canonicalName;
}

QTCREATOR_UTILS_EXPORT unsigned long winQPidToPid(const Q_PID qpid)
{
    const PROCESS_INFORMATION *processInfo = reinterpret_cast<const PROCESS_INFORMATION*>(qpid);
    return processInfo->dwProcessId;
}

QTCREATOR_UTILS_EXPORT bool winIs64BitSystem()
{
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo(&systemInfo);
    return systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64
            || systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64;
}

QTCREATOR_UTILS_EXPORT bool winIs64BitBinary(const QString &binaryIn)
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

} // namespace Utils
