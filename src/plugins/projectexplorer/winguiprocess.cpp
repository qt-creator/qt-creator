/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "winguiprocess.h"
#include "windebuginterface.h"
#include "consoleprocess.h"

#include <utils/qtcprocess.h>
#include <utils/winutils.h>

#include <QtCore/QDir>

using namespace ProjectExplorer::Internal;

/*!
    \class ProjectExplorer::Internal::WinGuiProcess
    \brief Captures the debug output of a Windows GUI application.

    The output of a Windows GUI application would otherwise not be
    visible. Uses the debug interface and emits via a signal.

    \sa ProjectExplorer::Internal::WinDebugInterface
*/

WinGuiProcess::WinGuiProcess(QObject *parent) :
    QThread(parent),
    m_pid(0),
    m_exitCode(0)
{
    connect(this, SIGNAL(processFinished(int)), this, SLOT(done()));
}

WinGuiProcess::~WinGuiProcess()
{
    stop();
}

bool WinGuiProcess::isRunning() const
{
    return QThread::isRunning();
}

bool WinGuiProcess::start(const QString &program, const QString &args)
{
    if (isRunning())
        return false;

    connect(WinDebugInterface::instance(), SIGNAL(debugOutput(qint64,QString)),
            this, SLOT(checkDebugOutput(qint64,QString)));

    m_program = program;
    m_args = args;

    QThread::start();
    return true;
}

void WinGuiProcess::stop()
{
    if (m_pid)
        TerminateProcess(m_pid->hProcess, 1);
    wait();
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

    if (!WinDebugInterface::instance()->isRunning())
        WinDebugInterface::instance()->start(); // Try to start listener again...

    do {
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
            emit processMessage(tr("The process could not be started: %1").
                                arg(Utils::winErrorMessage(GetLastError())), true);
            emit processFinished(0);
            break;
        }

        if (!WinDebugInterface::instance()->isRunning())
            emit processMessage(msgWinCannotRetrieveDebuggingOutput(), false);

        WaitForSingleObject(m_pid->hProcess, INFINITE);
    } while (false);

    if (started) {
        GetExitCodeProcess(m_pid->hProcess, &m_exitCode);
        emit processFinished(static_cast<int>(m_exitCode));
    }

    if (m_pid->hProcess != NULL)
        CloseHandle(m_pid->hProcess);
    if  (m_pid->hThread != NULL)
        CloseHandle(m_pid->hThread);
    delete m_pid;
    m_pid = 0;
}

void WinGuiProcess::checkDebugOutput(qint64 pid, const QString &message)
{
    if (applicationPID() == pid)
        emit receivedDebugOutput(message);
}

void WinGuiProcess::done()
{
    disconnect(WinDebugInterface::instance(), SIGNAL(debugOutput(qint64,QString)),
               this, SLOT(checkDebugOutput(qint64,QString)));
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
