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

#include "consoleprocess.h"
#include "winutils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/private/qwineventnotifier_p.h>

#include <QtNetwork/QLocalSocket>

#include <stdlib.h>

using namespace Core::Utils;

ConsoleProcess::ConsoleProcess(QObject *parent)
    : QObject(parent)
{
    m_debug = false;
    m_appPid = 0;
    m_pid = 0;
    m_hInferior = NULL;
    m_tempFile = 0;
    m_stubSocket = 0;
    processFinishedNotifier = 0;
    inferiorFinishedNotifier = 0;

    connect(&m_stubServer, SIGNAL(newConnection()), SLOT(stubConnectionAvailable()));
}

ConsoleProcess::~ConsoleProcess()
{
    stop();
}

bool ConsoleProcess::start(const QString &program, const QStringList &args)
{
    if (isRunning())
        return false;

    QString err = stubServerListen();
    if (!err.isEmpty()) {
        emit processError(tr("Cannot set up comm channel: %1").arg(err));
        return false;
    }

    if (!environment().isEmpty()) {
        m_tempFile = new QTemporaryFile();
        if (!m_tempFile->open()) {
            stubServerShutdown();
            emit processError(tr("Cannot create temp file: %1").arg(m_tempFile->errorString()));
            delete m_tempFile;
            m_tempFile = 0;
            return false;
        }
        QTextStream out(m_tempFile);
        out.setCodec("UTF-16LE");
        out.setGenerateByteOrderMark(false);
        foreach (const QString &var, fixEnvironment(environment()))
            out << var << QChar(0);
        out << QChar(0);
    }

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    m_pid = new PROCESS_INFORMATION;
    ZeroMemory(m_pid, sizeof(PROCESS_INFORMATION));

    QString workDir = QDir::toNativeSeparators(workingDirectory());
    if (!workDir.isEmpty() && !workDir.endsWith('\\'))
        workDir.append('\\');

    QStringList stubArgs;
    stubArgs << (m_debug ? "debug" : "exec")
             << m_stubServer.fullServerName()
             << workDir
             << (m_tempFile ? m_tempFile->fileName() : 0)
             << createCommandline(program, args)
             << tr("Press <RETURN> to close this window...");

    QString cmdLine = createCommandline(
            QCoreApplication::applicationDirPath() + "/qtcreator_process_stub.exe", stubArgs);

    bool success = CreateProcessW(0, (WCHAR*)cmdLine.utf16(),
                                  0, 0, FALSE, CREATE_NEW_CONSOLE,
                                  0, 0,
                                  &si, m_pid);

    if (!success) {
        delete m_pid;
        m_pid = 0;
        delete m_tempFile;
        m_tempFile = 0;
        stubServerShutdown();
        emit processError(tr("The process '%1' could not be started: %2").arg(cmdLine, winErrorMessage(GetLastError())));
        return false;
    }

    processFinishedNotifier = new QWinEventNotifier(m_pid->hProcess, this);
    connect(processFinishedNotifier, SIGNAL(activated(HANDLE)), SLOT(stubExited()));
    emit wrapperStarted();
    return true;
}

void ConsoleProcess::stop()
{
    if (m_hInferior != NULL) {
        TerminateProcess(m_hInferior, (unsigned)-1);
        cleanupInferior();
    }
    if (m_pid) {
        TerminateProcess(m_pid->hProcess, (unsigned)-1);
        WaitForSingleObject(m_pid->hProcess, INFINITE);
        cleanupStub();
    }
}

bool ConsoleProcess::isRunning() const
{
    return m_pid != 0;
}

QString ConsoleProcess::stubServerListen()
{
    if (m_stubServer.listen(QString::fromLatin1("creator-%1-%2")
                            .arg(QCoreApplication::applicationPid())
                            .arg(rand())))
        return QString();
    return m_stubServer.errorString();
}

void ConsoleProcess::stubServerShutdown()
{
    delete m_stubSocket;
    m_stubSocket = 0;
    if (m_stubServer.isListening())
        m_stubServer.close();
}

void ConsoleProcess::stubConnectionAvailable()
{
    m_stubSocket = m_stubServer.nextPendingConnection();
    connect(m_stubSocket, SIGNAL(readyRead()), SLOT(readStubOutput()));
}

void ConsoleProcess::readStubOutput()
{
    while (m_stubSocket->canReadLine()) {
        QByteArray out = m_stubSocket->readLine();
        out.chop(2); // \r\n
        if (out.startsWith("err:chdir ")) {
            emit processError(tr("Cannot change to working directory %1: %2")
                              .arg(workingDirectory(), winErrorMessage(out.mid(10).toInt())));
        } else if (out.startsWith("err:exec ")) {
            emit processError(tr("Cannot execute %1: %2")
                              .arg(m_executable, winErrorMessage(out.mid(9).toInt())));
        } else if (out.startsWith("pid ")) {
            // Will not need it any more
            delete m_tempFile;
            m_tempFile = 0;

            m_appPid = out.mid(4).toInt();
            m_hInferior = OpenProcess(
                    SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE,
                    FALSE, m_appPid);
            if (m_hInferior == NULL) {
                emit processError(tr("Cannot obtain a handle to the inferior: %1")
                                  .arg(winErrorMessage(GetLastError())));
                // Uhm, and now what?
                continue;
            }
            inferiorFinishedNotifier = new QWinEventNotifier(m_hInferior, this);
            connect(inferiorFinishedNotifier, SIGNAL(activated(HANDLE)), SLOT(inferiorExited()));
            emit processStarted();
        } else {
            emit processError(tr("Unexpected output from helper program."));
            TerminateProcess(m_pid->hProcess, (unsigned)-1);
            break;
        }
    }
}

void ConsoleProcess::cleanupInferior()
{
    delete inferiorFinishedNotifier;
    inferiorFinishedNotifier = 0;
    CloseHandle(m_hInferior);
    m_hInferior = NULL;
    m_appPid = 0;
}

void ConsoleProcess::inferiorExited()
{
    DWORD chldStatus;

    if (!GetExitCodeProcess(m_hInferior, &chldStatus))
        emit processError(tr("Cannot obtain exit status from inferior: %1")
                          .arg(winErrorMessage(GetLastError())));
    cleanupInferior();
    m_appStatus = QProcess::NormalExit;
    m_appCode = chldStatus;
    emit processStopped();
}

void ConsoleProcess::cleanupStub()
{
    stubServerShutdown();
    delete processFinishedNotifier;
    processFinishedNotifier = 0;
    CloseHandle(m_pid->hThread);
    CloseHandle(m_pid->hProcess);
    delete m_pid;
    m_pid = 0;
    delete m_tempFile;
    m_tempFile = 0;
}

void ConsoleProcess::stubExited()
{
    // The stub exit might get noticed before we read the pid for the kill.
    if (m_stubSocket && m_stubSocket->state() == QLocalSocket::ConnectedState)
        m_stubSocket->waitForDisconnected();
    cleanupStub();
    if (m_hInferior != NULL) {
        TerminateProcess(m_hInferior, (unsigned)-1);
        cleanupInferior();
        m_appStatus = QProcess::CrashExit;
        m_appCode = -1;
        emit processStopped();
    }
    emit wrapperStopped();
}

QStringList ConsoleProcess::fixEnvironment(const QStringList &env)
{
    QStringList envStrings = env;
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
    return envStrings;
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
