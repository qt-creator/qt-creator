// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Some functions used by qtcreator.exe and qtcdebugger.exe to check if
// qtcdebugger is currently registered for post-mortem debugging.
// This is only needed on Windows.

#pragma once

#include <QString>
#include <QLatin1String>

#include <windows.h>

namespace RegistryAccess {

enum AccessMode {
    DefaultAccessMode,
    Registry32Mode = 0x2, // Corresponds to QSettings::Registry32Format (5.7)
    Registry64Mode = 0x4  // Corresponds to QSettings::Registry64Format (5.7)
};

constexpr const char debuggerApplicationFileC[] = "qtcdebugger";
constexpr const WCHAR debuggerRegistryKeyC[]
    = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";
constexpr const WCHAR debuggerRegistryValueNameC[] = L"Debugger";
constexpr const WCHAR autoRegistryValueNameC[] = L"Auto";

static inline QString wCharToQString(const WCHAR *w)
{
    return QString::fromUtf16(reinterpret_cast<const char16_t *>(w));
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
                     AccessMode mode,
                     QString *errorMessage);

inline bool openRegistryKey(HKEY category, const WCHAR *key, bool readWrite, HKEY *keyHandle, QString *errorMessage)
{ return openRegistryKey(category, key, readWrite, keyHandle, DefaultAccessMode, errorMessage); }

QString debuggerCall(const QString &additionalOption = QString());

bool isRegistered(HKEY handle, const QString &call, QString *errorMessage, QString *oldDebugger = nullptr);

} // namespace RegistryAccess
