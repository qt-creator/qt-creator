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

/* A stub for Windows console processes (like nmake) that is able to terminate
 * its child process via a generated Ctrl-C event.
 * The termination is triggered by sending a custom message to the HWND of
 * this process. */

#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>
#include <cstdlib>
#include <cstdio>

const wchar_t szTitle[] = L"qtcctrlcstub";
const wchar_t szWindowClass[] = L"wcqtcctrlcstub";
UINT uiShutDownWindowMessage;
HWND hwndMain = 0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL WINAPI ctrlHandler(DWORD dwCtrlType);
bool isSpaceOrTab(const wchar_t c);
bool startProcess(wchar_t pCommandLine[]);

int main(int argc, char **)
{
    if (argc < 2) {
        fprintf(stderr, "This is an internal helper of Qt Creator. Do not run it manually.\n");
        return 1;
    }

    SetConsoleCtrlHandler(ctrlHandler, TRUE);
    uiShutDownWindowMessage = RegisterWindowMessage(L"qtcctrlcstub_shutdown");

    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(wcex);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = GetModuleHandle(0);
    wcex.lpszClassName = szWindowClass;
    if (!RegisterClassEx(&wcex))
        return 1;

    hwndMain = CreateWindow(szWindowClass, szTitle, WS_DISABLED,
                            0, 0, 0, 0, NULL, NULL, wcex.hInstance, NULL);
    if (!hwndMain)
        return FALSE;

    // Get the command line and remove the call to this executable.
    wchar_t *strCommandLine = _wcsdup(GetCommandLine());
    const size_t strCommandLineLength = wcslen(strCommandLine);
    size_t pos = 0;
    bool quoted = false;
    while (pos < strCommandLineLength) {
        if (strCommandLine[pos] == L'"') {
            quoted = !quoted;
        } else if (!quoted && isSpaceOrTab(strCommandLine[pos])) {
            while (isSpaceOrTab(strCommandLine[++pos]));
            break;
        }
        ++pos;
    }

    bool bSuccess = startProcess(strCommandLine + pos);
    free(strCommandLine);

    if (!bSuccess)
        return -1;

    MSG msg;
    DWORD dwExitCode = -1;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_DESTROY)
            dwExitCode = msg.wParam;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)dwExitCode;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == uiShutDownWindowMessage) {
        GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
        PostQuitMessage(0);
        return 0;
    }

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool isSpaceOrTab(const wchar_t c)
{
    return c == L' ' || c == L'\t';
}

BOOL WINAPI ctrlHandler(DWORD /*dwCtrlType*/)
{
    PostMessage(hwndMain, WM_DESTROY, 0, 0);
    return TRUE;
}

DWORD WINAPI processWatcherThread(LPVOID lpParameter)
{
    HANDLE hProcess = reinterpret_cast<HANDLE>(lpParameter);
    WaitForSingleObject(hProcess, INFINITE);
    DWORD dwExitCode;
    if (!GetExitCodeProcess(hProcess, &dwExitCode))
        dwExitCode = -1;
    PostMessage(hwndMain, WM_DESTROY, dwExitCode, 0);
    return 0;
}

bool startProcess(wchar_t *pCommandLine)
{
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    STARTUPINFO si = {0};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    DWORD dwCreationFlags = 0;
    BOOL bSuccess = CreateProcess(NULL, pCommandLine, &sa, &sa, TRUE, dwCreationFlags, NULL, NULL, &si, &pi);
    if (!bSuccess) {
        fwprintf(stderr, L"qtcreator_ctrlc_stub: Command line failed: %s\n", pCommandLine);
        return false;
    }

    HANDLE hThread = CreateThread(NULL, 0, processWatcherThread, reinterpret_cast<void*>(pi.hProcess), 0, NULL);
    if (!hThread) {
        fwprintf(stderr, L"qtcreator_ctrlc_stub: The watch dog thread cannot be started.\n");
        return false;
    }
    CloseHandle(hThread);
    return true;
}
