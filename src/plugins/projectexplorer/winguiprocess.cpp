/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "winguiprocess.h"
#include "consoleprocess.h"

#include <QtCore/QDir>

using namespace ProjectExplorer::Internal;

WinGuiProcess::WinGuiProcess(QObject *parent)
    : QThread(parent)
{
    m_pid = 0;
    m_exitCode = 0;
}

WinGuiProcess::~WinGuiProcess()
{
    stop();
}

bool WinGuiProcess::start(const QString &program, const QStringList &args)
{
    m_program = program;
    m_args = args;

    if (!isRunning()) {
        QThread::start(QThread::NormalPriority);
        return true;
    }
    return false;
}

void WinGuiProcess::stop()
{
    if (m_pid)
        TerminateProcess(m_pid->hProcess, 1);
    wait();
}

bool WinGuiProcess::isRunning() const
{
    return QThread::isRunning();
}

bool WinGuiProcess::setupDebugInterface(HANDLE &bufferReadyEvent, HANDLE &dataReadyEvent, HANDLE &sharedFile, LPVOID &sharedMem)
{

    bufferReadyEvent = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_BUFFER_READY");
    if (!bufferReadyEvent || GetLastError() == ERROR_ALREADY_EXISTS)
        return false;
    dataReadyEvent = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_DATA_READY");
    if (!dataReadyEvent || GetLastError() == ERROR_ALREADY_EXISTS)
        return false;
    sharedFile = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, 4096, L"DBWIN_BUFFER");
    if (!sharedFile || GetLastError() == ERROR_ALREADY_EXISTS)
        return false;
    sharedMem = MapViewOfFile(sharedFile, FILE_MAP_READ, 0, 0,  512);
    if (!sharedMem)
        return false;
    return true;
}

void WinGuiProcess::run()
{
    if (m_pid)
        return;

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    m_pid = new PROCESS_INFORMATION;
    ZeroMemory(m_pid, sizeof(PROCESS_INFORMATION));

    m_exitCode = 0;

    HANDLE bufferReadyEvent = NULL;
    HANDLE dataReadyEvent = NULL;
    HANDLE sharedFile = NULL;
    LPVOID sharedMem = NULL;

    bool dbgInterface = setupDebugInterface(bufferReadyEvent, dataReadyEvent, sharedFile, sharedMem);

    QString cmdLine = ConsoleProcess::createCommandline(m_program, m_args);
    bool success = CreateProcessW(0, (WCHAR*)cmdLine.utf16(),
                                  0, 0, TRUE, CREATE_UNICODE_ENVIRONMENT,
                                  environment().isEmpty() ? 0
                                  : ConsoleProcess::createEnvironment(environment()).data(),
                                  workingDirectory().isEmpty() ? 0
                                  : (WCHAR*)QDir::convertSeparators(workingDirectory()).utf16(),
                                  &si, m_pid);

    if (!success) {
        emit processError(tr("The process could not be started!"));
        delete m_pid;
        m_pid = 0;
        return;
    }

    if (!dbgInterface) {
        emit receivedDebugOutput(tr("Cannot retrieve debugging output!"));
        WaitForSingleObject(m_pid->hProcess, INFINITE);
    } else {
        LPSTR  message;
        LPDWORD processId;
        HANDLE toWaitFor[2];

        message = reinterpret_cast<LPSTR>(sharedMem) + sizeof(DWORD);
        processId = reinterpret_cast<LPDWORD>(sharedMem);

        SetEvent(bufferReadyEvent);

        toWaitFor[0] = dataReadyEvent;
        toWaitFor[1] = m_pid->hProcess;

        for (bool stop = false; !stop;) {
            DWORD ret = WaitForMultipleObjects(2, toWaitFor, FALSE, INFINITE);

            switch (ret) {
            case WAIT_OBJECT_0 + 0:
                if (*processId == m_pid->dwProcessId)
                    emit receivedDebugOutput(QString::fromAscii(message));
                SetEvent(bufferReadyEvent);
                break;
            case WAIT_OBJECT_0 + 1:
                stop = true;
                break;
            }
        }
    }

    GetExitCodeProcess(m_pid->hProcess, &m_exitCode);
    emit processFinished(static_cast<int>(m_exitCode));

    UnmapViewOfFile(sharedMem);
    CloseHandle(sharedFile);
    CloseHandle(bufferReadyEvent);
    CloseHandle(dataReadyEvent);
    CloseHandle(m_pid->hProcess);
    CloseHandle(m_pid->hThread);

    delete m_pid;
    m_pid = 0;
}

qint64 WinGuiProcess::applicationPID() const
{
    if (m_pid)
        return m_pid->dwProcessId;
    return 0;
}

int WinGuiProcess::exitCode() const
{
    return static_cast<int>(m_exitCode);
}
