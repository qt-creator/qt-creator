/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

// Some functions used by qtcreator.exe and qtcdebugger.exe to check if
// qtcdebugger is currently registered for post-mortem debugging.
// This is only needed on Windows.

#ifndef REGISTRYACCESS_H
#define REGISTRYACCESS_H

#include <QString>
#include <QLatin1String>

#include <windows.h>

namespace RegistryAccess {

static const char *debuggerApplicationFileC = "qtcdebugger";
static const WCHAR *debuggerRegistryKeyC = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";
static const WCHAR *debuggerRegistryValueNameC = L"Debugger";

static inline QString wCharToQString(const WCHAR *w)
{
    return QString::fromUtf16(reinterpret_cast<const ushort *>(w));
}

QString msgFunctionFailed(const char *f, unsigned long error);

static inline QString msgRegistryOperationFailed(const char *op, const WCHAR *valueName, const QString &why)
{
    QString rc = QLatin1String("Registry ");
    rc += QLatin1String(op);
    rc += QLatin1String(" of ");
    rc += wCharToQString(valueName);
    rc += QLatin1String(" failed: ");
    rc += why;
    return rc;
}

bool registryReadStringKey(HKEY handle, // HKEY_LOCAL_MACHINE, etc.
                           const WCHAR *valueName,
                           QString *s,
                           QString *errorMessage);

bool openRegistryKey(HKEY category, // HKEY_LOCAL_MACHINE, etc.
                     const WCHAR *key,
                     bool readWrite,
                     HKEY *keyHandle,
                     QString *errorMessage);

QString debuggerCall(const QString &additionalOption = QString());

bool isRegistered(HKEY handle, const QString &call, QString *errorMessage, QString *oldDebugger = 0);

} // namespace RegistryAccess

#endif // REGISTRYACCESS_H
