// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#if _WIN32_WINNT < 0x0501
#error Must target Windows NT 5.0.1 or later for DebugBreakProcess
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

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
