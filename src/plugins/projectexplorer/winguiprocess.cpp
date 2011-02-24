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

#include "winguiprocess.h"
#include "consoleprocess.h"

#include <utils/qtcprocess.h>

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

bool WinGuiProcess::start(const QString &program, const QString &args)
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
    bool started = false;

    HANDLE bufferReadyEvent = NULL;
    HANDLE dataReadyEvent = NULL;
    HANDLE sharedFile = NULL;
    LPVOID sharedMem = 0;

    do {

        const bool dbgInterface = setupDebugInterface(bufferReadyEvent, dataReadyEvent, sharedFile, sharedMem);

        QString pcmd, pargs;
        QtcProcess::prepareCommand(m_program, m_args, &pcmd, &pargs, &m_environment, &m_workingDir);
        const QString cmdLine = createWinCommandline(pcmd, pargs);
        const QStringList env = m_environment.toStringList();
        started = CreateProcessW(0, (WCHAR*)cmdLine.utf16(),
                                      0, 0, TRUE, CREATE_UNICODE_ENVIRONMENT,
                                      env.isEmpty() ? 0
                                          : createWinEnvironment(fixWinEnvironment(env)).data(),
                                          workingDirectory().isEmpty() ? 0
                                              : (WCHAR*)QDir::convertSeparators(workingDirectory()).utf16(),
                                              &si, m_pid);

        if (!started) {
            emit processMessage(tr("The process could not be started!"), true);
            break;
        }

        if (!dbgInterface) {
            // Text is dublicated in qmlengine.cpp
            emit receivedDebugOutput(tr("Cannot retrieve debugging output!"), true);
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
                        emit receivedDebugOutput(QString::fromLocal8Bit(message), false);
                    SetEvent(bufferReadyEvent);
                    break;
                case WAIT_OBJECT_0 + 1:
                    stop = true;
                    break;
                }
            }
        }
    } while (false);

    if (started) {
        GetExitCodeProcess(m_pid->hProcess, &m_exitCode);
        emit processFinished(static_cast<int>(m_exitCode));
    }

    if (sharedMem)
        UnmapViewOfFile(sharedMem);
    if (sharedFile != NULL)
        CloseHandle(sharedFile);
    if (bufferReadyEvent != NULL)
        CloseHandle(bufferReadyEvent);
    if (dataReadyEvent != NULL)
        CloseHandle(dataReadyEvent);
    if (m_pid->hProcess != NULL)
        CloseHandle(m_pid->hProcess);
    if  (m_pid->hThread != NULL)
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
