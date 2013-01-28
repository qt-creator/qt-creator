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

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include "registerpostmortemaction.h"

#include "registryaccess.h"

#include <QCoreApplication>
#include <QDir>
#include <QString>

#include <windows.h>
#include <objbase.h>
#include <shellapi.h>

using namespace RegistryAccess;

namespace Debugger {
namespace Internal {

void RegisterPostMortemAction::registerNow(const QVariant &value)
{
    const bool boolValue = value.toBool();
    const QString debuggerExe = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QLatin1Char('/')
                                + QLatin1String(debuggerApplicationFileC) + QLatin1String(".exe"));
    const ushort *debuggerWString = debuggerExe.utf16();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    SHELLEXECUTEINFO shExecInfo;
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    shExecInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.hwnd   = NULL;
    shExecInfo.lpVerb = L"runas";
    shExecInfo.lpFile = reinterpret_cast<LPCWSTR>(debuggerWString);
    shExecInfo.lpParameters = boolValue ? L"-register" : L"-unregister";
    shExecInfo.lpDirectory  = NULL;
    shExecInfo.nShow        = SW_SHOWNORMAL;
    shExecInfo.hProcess     = NULL;
    if (ShellExecuteEx(&shExecInfo) && shExecInfo.hProcess)
        WaitForSingleObject(shExecInfo.hProcess, INFINITE);
    CoUninitialize();
    readSettings();
}

RegisterPostMortemAction::RegisterPostMortemAction(QObject *parent) : Utils::SavedAction(parent)
{
    connect(this, SIGNAL(valueChanged(QVariant)), SLOT(registerNow(QVariant)));
}

void RegisterPostMortemAction::readSettings(const QSettings *)
{
    Q_UNUSED(debuggerRegistryValueNameC); // avoid warning from MinGW

    bool registered = false;
    HKEY handle = 0;
    QString errorMessage;
    if (openRegistryKey(HKEY_LOCAL_MACHINE, debuggerRegistryKeyC, false, &handle, &errorMessage))
        registered = isRegistered(handle, debuggerCall(), &errorMessage);
    if (handle)
        RegCloseKey(handle);
    setValue(registered, false);
}

} // namespace Internal
} // namespace Debugger
