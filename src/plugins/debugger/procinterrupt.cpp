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

#include "procinterrupt.h"

using namespace Debugger::Internal;

#if defined(Q_OS_WIN)

#define _WIN32_WINNT 0x0501 /* WinXP, needed for DebugBreakProcess() */

#include <windows.h>

bool Debugger::Internal::interruptProcess(int pID)
{
    if (pID <= 0)
        return false;

    HANDLE hproc = OpenProcess(PROCESS_ALL_ACCESS, false, pID);
    if (hproc == NULL)
        return false;

    bool ok = DebugBreakProcess(hproc) != 0;

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
