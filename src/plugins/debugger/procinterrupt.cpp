/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "procinterrupt.h"

#include <QtCore/QProcess> // makes kill visible on Windows.

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
