/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "winutils.h"
#include <windows.h>

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QDebug>
#include <QtCore/QLibrary>
#include <QtCore/QTextStream>

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
        *errorMessage = QString::fromLatin1("Unable load %1: %2").arg(QLatin1String(versionDLLC), versionLib.errorString());
        return QString();
    }
    // MinGW requires old-style casts
    GetFileVersionInfoSizeProtoType getFileVersionInfoSizeW = (GetFileVersionInfoSizeProtoType)(versionLib.resolve("GetFileVersionInfoSizeW"));
    GetFileVersionInfoWProtoType getFileVersionInfoW = (GetFileVersionInfoWProtoType)(versionLib.resolve("GetFileVersionInfoW"));
    VerQueryValueWProtoType verQueryValueW = (VerQueryValueWProtoType)(versionLib.resolve("VerQueryValueW"));
    if (!getFileVersionInfoSizeW || !getFileVersionInfoW || !verQueryValueW) {
        *errorMessage = QString::fromLatin1("Unable to resolve all required symbols in  %1").arg(QLatin1String(versionDLLC));
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
    UINT len = 0;
    if (!(*verQueryValueW)(data, TEXT("\\"), &versionInfo, &len)) {
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

} // namespace Utils
