// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "registerpostmortemaction.h"

#include <registryaccess.h>

#include <QCoreApplication>
#include <QDir>
#include <QString>

#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <windows.h>
#include <objbase.h>
#include <shellapi.h>

using namespace RegistryAccess;

namespace Debugger {
namespace Internal {

void RegisterPostMortemAction::registerNow(bool value)
{
    const QString debuggerExe = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + '/'
                                + QLatin1String(debuggerApplicationFileC) + ".exe");
    const ushort *debuggerWString = debuggerExe.utf16();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    SHELLEXECUTEINFO shExecInfo;
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    shExecInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.hwnd   = NULL;
    shExecInfo.lpVerb = L"runas";
    shExecInfo.lpFile = reinterpret_cast<LPCWSTR>(debuggerWString);
    shExecInfo.lpParameters = value ? L"-register" : L"-unregister";
    shExecInfo.lpDirectory  = NULL;
    shExecInfo.nShow        = SW_SHOWNORMAL;
    shExecInfo.hProcess     = NULL;
    if (ShellExecuteEx(&shExecInfo) && shExecInfo.hProcess)
        WaitForSingleObject(shExecInfo.hProcess, INFINITE);
    CoUninitialize();
    readSettings();
}

RegisterPostMortemAction::RegisterPostMortemAction()
{
    connect(this, &BaseAspect::changed, this, [this] { registerNow(value()); });
}

void RegisterPostMortemAction::readSettings()
{
    Q_UNUSED(debuggerRegistryValueNameC) // avoid warning from MinGW

    bool registered = false;
    HKEY handle = NULL;
    QString errorMessage;
    if (openRegistryKey(HKEY_LOCAL_MACHINE, debuggerRegistryKeyC, false, &handle, &errorMessage))
        registered = isRegistered(handle, debuggerCall(), &errorMessage);
    if (handle)
        RegCloseKey(handle);
    setValue(registered, BaseAspect::BeQuiet);
}

} // namespace Internal
} // namespace Debugger
