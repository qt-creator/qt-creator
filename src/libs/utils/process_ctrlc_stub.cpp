// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>
#include <wchar.h>
#include <cstdlib>
#include <cstdio>

#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif

/// Class that ensures that when the parent process cancels, the child
/// process will also be cancelled by the operating system.
///
/// This allows handling of GUI applications that do not react to Ctrl+C
/// or console applications that ignore Ctrl+C.
class JobKillOnClose
{
    HANDLE m_job = nullptr;
public:
    JobKillOnClose()
    {
        m_job = CreateJobObject(nullptr, nullptr);
        if (!m_job) {
            fwprintf(stderr, L"qtcreator_ctrlc_stub: CreateJobObject failed: 0x%x.\n", GetLastError());
            return;
        }
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(m_job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) {
            fwprintf(stderr, L"qtcreator_ctrlc_stub: SetInformationJobObject failed: 0x%x.\n", GetLastError());
        }
    }

    BOOL AssignProcessToJob(HANDLE process) const
    {
        return AssignProcessToJobObject(m_job, process);
    }

    ~JobKillOnClose()
    {
        CloseHandle(m_job);
    }
};

const wchar_t szTitle[] = L"qtcctrlcstub";
const wchar_t szWindowClass[] = L"wcqtcctrlcstub";
const wchar_t szNice[] = L"-nice ";
UINT uiShutDownWindowMessage;
UINT uiInterruptMessage;
HWND hwndMain = nullptr;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL WINAPI shutdownHandler(DWORD dwCtrlType);
BOOL WINAPI interruptHandler(DWORD dwCtrlType);
bool isSpaceOrTab(const wchar_t c);
bool startProcess(wchar_t pCommandLine[], bool lowerPriority, const JobKillOnClose& job);

int main(int argc, char **)
{
    if (argc < 2) {
        fprintf(stderr, "This is an internal helper of Qt Creator. Do not run it manually.\n");
        return 1;
    }

    uiShutDownWindowMessage = RegisterWindowMessage(L"qtcctrlcstub_shutdown");
    uiInterruptMessage = RegisterWindowMessage(L"qtcctrlcstub_interrupt");

    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = GetModuleHandle(nullptr);
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

    const size_t niceLen = wcslen(szNice);
    bool lowerPriority = !wcsncmp(strCommandLine + pos, szNice, niceLen);
    if (lowerPriority) {
        pos += niceLen - 1; // reach the space, then the following line skips all spaces.
        while (isSpaceOrTab(strCommandLine[++pos]))
            ;
    }

    JobKillOnClose job;
    bool bSuccess = startProcess(strCommandLine + pos, lowerPriority, job);
    free(strCommandLine);

    if (!bSuccess)
        return -1;

    MSG msg;
    DWORD dwExitCode = -1;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_DESTROY)
            dwExitCode = static_cast<DWORD>(msg.wParam);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)dwExitCode;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == uiShutDownWindowMessage) {
        SetConsoleCtrlHandler(interruptHandler, FALSE);
        PostQuitMessage(0);
        return 0;
    } else if (message == uiInterruptMessage) {
        SetConsoleCtrlHandler(interruptHandler, TRUE);
        GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
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

BOOL WINAPI interruptHandler(DWORD /*dwCtrlType*/)
{
    return TRUE;
}

DWORD WINAPI processWatcherThread(LPVOID lpParameter)
{
    auto hProcess = reinterpret_cast<HANDLE>(lpParameter);
    WaitForSingleObject(hProcess, INFINITE);
    DWORD dwExitCode;
    if (!GetExitCodeProcess(hProcess, &dwExitCode))
        dwExitCode = -1;
    CloseHandle(hProcess);
    PostMessage(hwndMain, WM_DESTROY, dwExitCode, 0);
    return 0;
}

bool startProcess(wchar_t *pCommandLine, bool lowerPriority, const JobKillOnClose& job)
{
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    STARTUPINFO si = {};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi = {};
    DWORD dwCreationFlags = lowerPriority ? BELOW_NORMAL_PRIORITY_CLASS : 0;
    BOOL bSuccess = CreateProcess(NULL, pCommandLine, &sa, &sa, TRUE, dwCreationFlags, NULL, NULL, &si, &pi);
    if (!bSuccess) {
        fwprintf(stderr, L"qtcreator_ctrlc_stub: Command line failed: %s\n", pCommandLine);
        return false;
    }
    CloseHandle(pi.hThread);

    if (!job.AssignProcessToJob(pi.hProcess)) {
        fwprintf(stderr, L"qtcreator_ctrlc_stub: AssignProcessToJobObject failed: 0x%x.\n", GetLastError());
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
