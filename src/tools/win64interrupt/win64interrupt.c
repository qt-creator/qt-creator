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
#define _WIN32_WINNT 0x0501
#endif

#if _WIN32_WINNT < 0x0501
#error Must target Windows NT 5.0.1 or later for DebugBreakProcess
#endif

#include <windows.h>
#include <stdio.h>

/* To debug break a 64bit application under Windows, you must call
 * DebugBreakProcess() from an 64bit apllication. Therefore:
 *
 * This code must be compiled with a 64bit compiler
 * Compile with one of these lines:
 *  gcc-64bit:
 *      gcc -o ..\..\..\bin\win64interrupt.exe win64interrupt.c
 *  cl-64bit:
 *      cl -o ..\..\..\bin\win64interrupt.exe win64interrupt.c
 */

int main(int argc, char *argv[])
{
    HANDLE proc;
    DWORD proc_id;
    BOOL break_result = FALSE;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <process-id>\n", argv[0]);
        return 1;
    }

    proc_id = strtoul(argv[1], NULL, 0);

    if (proc_id == 0) {
        fprintf(stderr, "%s: Invalid argument '%s'\n", argv[0], argv[1]);
        return 2;
    }

    proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proc_id);
    if (!proc) {
        fprintf(stderr, "%s: OpenProcess() failed, error 0x%lx\n", argv[0], GetLastError());
        return 3;
    }

    break_result = DebugBreakProcess(proc);
    if (!break_result)
        fprintf(stderr, "%s: DebugBreakProcess() failed, error 0x%lx\n", argv[0], GetLastError());
    CloseHandle(proc);
    return !break_result;
}
