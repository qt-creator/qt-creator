/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "registerpostmortemaction.h"

#include "registryaccess.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QString>

#include <Windows.h>
#include <Objbase.h>
#include <Shellapi.h>

using namespace RegistryAccess;

namespace Debugger {
namespace Internal {

void RegisterPostMortemAction::registerNow(const QVariant &value)
{
    const bool boolValue = value.toBool();
    const QString debuggerExe = QCoreApplication::applicationDirPath() + QLatin1Char('/')
                                + debuggerApplicationFileC + QLatin1String(".exe");
    const std::wstring debuggerWString = QDir::toNativeSeparators(debuggerExe).toStdWString();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    SHELLEXECUTEINFO shExecInfo;
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    shExecInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.hwnd   = NULL;
    shExecInfo.lpVerb = L"runas";
    shExecInfo.lpFile = debuggerWString.data();
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
