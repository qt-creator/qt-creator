/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "procinterrupt.h"

#include <QtCore/QProcess> // makes kill visible on Windows.
#include <QtCore/QFile>

using namespace Debugger::Internal;

#if defined(Q_OS_WIN)

#define _WIN32_WINNT 0x0501 /* WinXP, needed for DebugBreakProcess() */

#include <utils/winutils.h>
#include <windows.h>

static BOOL isWow64Process(HANDLE hproc)
{
    BOOL ret = false;
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
    HMODULE hModule = GetModuleHandle(L"kernel32.dll");
    if (hModule == NULL)
        return false;

    fnIsWow64Process = reinterpret_cast<LPFN_ISWOW64PROCESS>(GetProcAddress(hModule, "IsWow64Process"));
    if (fnIsWow64Process == NULL)
        return false;

    if (!fnIsWow64Process(hproc, &ret))
        return false;
    return ret;
}

bool Debugger::Internal::interruptProcess(int pID)
{
    if (pID <= 0)
        return false;

    HANDLE hproc = OpenProcess(PROCESS_ALL_ACCESS, false, pID);
    if (hproc == NULL)
        return false;

    BOOL proc64bit = false;

    if (Utils::winIs64BitSystem())
        proc64bit = !isWow64Process(hproc);

    bool ok = false;
    if (proc64bit)
        ok = !QProcess::execute(QCoreApplication::applicationDirPath() + QString::fromLatin1("/win64interrupt.exe %1").arg(pID));
    else
        ok = !DebugBreakProcess(hproc);

    CloseHandle(hproc);
    return ok;
}

#else // Q_OS_WIN

#include <sys/types.h>
#include <signal.h>

bool Debugger::Internal::interruptProcess(int pID)
{
    if (pID > 0) {
        if (kill(pID, SIGINT) == 0)
            return true;
    }
    return false;
}

#endif // !Q_OS_WIN
