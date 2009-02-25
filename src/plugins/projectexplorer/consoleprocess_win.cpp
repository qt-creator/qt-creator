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

#include <QtCore/QDir>
#include <QtCore/private/qwineventnotifier_p.h>
#include <QtCore/QAbstractEventDispatcher>

#include <Tlhelp32.h>

#include "consoleprocess.h"

using namespace ProjectExplorer::Internal;

ConsoleProcess::ConsoleProcess(QObject *parent)
    : QObject(parent)
{
    m_isRunning = false;
    m_pid = 0;
}

ConsoleProcess::~ConsoleProcess()
{
    stop();
}

void ConsoleProcess::stop()
{
    if (m_pid)
        TerminateProcess(m_pid->hProcess, -1);
    m_isRunning = false;
}

bool ConsoleProcess::start(const QString &program, const QStringList &args)
{
    if (m_isRunning)
        return false;

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (m_pid) {
        CloseHandle(m_pid->hThread);
        CloseHandle(m_pid->hProcess);
        delete m_pid;
        m_pid = 0;
    }
    m_pid = new PROCESS_INFORMATION;
    ZeroMemory(m_pid, sizeof(PROCESS_INFORMATION));

    QString cmdLine = QLatin1String("cmd /k ")
                      + createCommandline(program, args)
                      + QLatin1String(" & pause & exit");

    bool success = CreateProcessW(0, (WCHAR*)cmdLine.utf16(),
                                  0, 0, TRUE, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
                                  environment().isEmpty() ? 0
                                  : createEnvironment(environment()).data(),
                                  workingDirectory().isEmpty() ? 0
                                  : (WCHAR*)QDir::convertSeparators(workingDirectory()).utf16(),
                                  &si, m_pid);

    if (!success) {
        emit processError(tr("The process could not be started!"));
        return false;
    }

    if (QAbstractEventDispatcher::instance(thread())) {
        processFinishedNotifier = new QWinEventNotifier(m_pid->hProcess, this);
        QObject::connect(processFinishedNotifier, SIGNAL(activated(HANDLE)), this, SLOT(processDied()));
        processFinishedNotifier->setEnabled(true);
    }
    m_isRunning = true;
    emit processStarted();
    return success;
}

bool ConsoleProcess::isRunning() const
{
    return m_isRunning;
}

void ConsoleProcess::processDied()
{
    if (processFinishedNotifier) {
        processFinishedNotifier->setEnabled(false);
        delete processFinishedNotifier;
        processFinishedNotifier = 0;
    }
    delete m_pid;
    m_pid = 0;
    m_isRunning = false;
    emit processStopped();
}

qint64 ConsoleProcess::applicationPID() const
{
    if (m_pid) {
        HANDLE hProcList = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(PROCESSENTRY32);
        DWORD procId = 0;
        BOOL moreProc = Process32First(hProcList, &procEntry);
        while (moreProc) {
            if (procEntry.th32ParentProcessID == m_pid->dwProcessId) {
                procId = procEntry.th32ProcessID;
                break;
            }
            moreProc = Process32Next(hProcList, &procEntry);
        }

        CloseHandle(hProcList);
        return procId;
    }
    return 0;
}

int ConsoleProcess::exitCode() const
{
    DWORD exitCode;
    if (GetExitCodeProcess(m_pid->hProcess, &exitCode))
        return exitCode;
    return -1;
}

QByteArray ConsoleProcess::createEnvironment(const QStringList &env)
{
    QByteArray envlist;
    if (!env.isEmpty()) {
        QStringList envStrings = env;
        int pos = 0;
        // add PATH if necessary (for DLL loading)
        if (envStrings.filter(QRegExp("^PATH=",Qt::CaseInsensitive)).isEmpty()) {
            QByteArray path = qgetenv("PATH");
            if (!path.isEmpty())
                envStrings.prepend(QString(QLatin1String("PATH=%1")).arg(QString::fromLocal8Bit(path)));
        }
        // add systemroot if needed
        if (envStrings.filter(QRegExp("^SystemRoot=",Qt::CaseInsensitive)).isEmpty()) {
            QByteArray systemRoot = qgetenv("SystemRoot");
            if (!systemRoot.isEmpty())
                envStrings.prepend(QString(QLatin1String("SystemRoot=%1")).arg(QString::fromLocal8Bit(systemRoot)));
        }
#ifdef UNICODE
        if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)) {
            for (QStringList::ConstIterator it = envStrings.constBegin(); it != envStrings.constEnd(); it++ ) {
                QString tmp = *it;
                uint tmpSize = sizeof(TCHAR) * (tmp.length()+1);
                envlist.resize(envlist.size() + tmpSize);
                memcpy(envlist.data()+pos, tmp.utf16(), tmpSize);
                pos += tmpSize;
            }
            // add the 2 terminating 0 (actually 4, just to be on the safe side)
            envlist.resize(envlist.size() + 4);
            envlist[pos++] = 0;
            envlist[pos++] = 0;
            envlist[pos++] = 0;
            envlist[pos++] = 0;
        } else
#endif // UNICODE
        {
            for (QStringList::ConstIterator it = envStrings.constBegin(); it != envStrings.constEnd(); it++) {
                QByteArray tmp = (*it).toLocal8Bit();
                uint tmpSize = tmp.length() + 1;
                envlist.resize(envlist.size() + tmpSize);
                memcpy(envlist.data()+pos, tmp.data(), tmpSize);
                pos += tmpSize;
            }
            // add the terminating 0 (actually 2, just to be on the safe side)
            envlist.resize(envlist.size() + 2);
            envlist[pos++] = 0;
            envlist[pos++] = 0;
        }
    }
    return envlist;
}

QString ConsoleProcess::createCommandline(const QString &program, const QStringList &args)
{
    QString programName = program;
    if (!programName.startsWith(QLatin1Char('\"')) && !programName.endsWith(QLatin1Char('\"')) && programName.contains(" "))
        programName = "\"" + programName + "\"";
    programName.replace("/", "\\");

    QString cmdLine;
    // add the prgram as the first arrg ... it works better
    cmdLine = programName + " ";
    for (int i = 0; i < args.size(); ++i) {
        QString tmp = args.at(i);
        // in the case of \" already being in the string the \ must also be escaped
        tmp.replace( "\\\"", "\\\\\"" );
        // escape a single " because the arguments will be parsed
        tmp.replace( "\"", "\\\"" );
        if (tmp.isEmpty() || tmp.contains(' ') || tmp.contains('\t')) {
            // The argument must not end with a \ since this would be interpreted
            // as escaping the quote -- rather put the \ behind the quote: e.g.
            // rather use "foo"\ than "foo\"
            QString endQuote("\"");
            int i = tmp.length();
            while (i > 0 && tmp.at(i - 1) == '\\') {
                --i;
                endQuote += "\\";
            }
            cmdLine += QString(" \"") + tmp.left(i) + endQuote;
        } else {
            cmdLine += ' ' + tmp;
        }
    }
    return cmdLine;
}
