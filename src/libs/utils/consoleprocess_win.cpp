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

#include "consoleprocess_p.h"
#include "environment.h"
#include "qtcprocess.h"
#include "winutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QAbstractEventDispatcher>

#include <stdlib.h>

namespace Utils {

ConsoleProcessPrivate::ConsoleProcessPrivate() :
    m_mode(ConsoleProcess::Run),
    m_appPid(0),
    m_stubSocket(0),
    m_tempFile(0),
    m_appMainThreadId(0),
    m_pid(0),
    m_hInferior(NULL),
    inferiorFinishedNotifier(0),
    processFinishedNotifier(0)
{
}

ConsoleProcess::ConsoleProcess(QObject *parent) :
    QObject(parent), d(new ConsoleProcessPrivate)
{
    connect(&d->m_stubServer, SIGNAL(newConnection()), SLOT(stubConnectionAvailable()));
}

qint64 ConsoleProcess::applicationMainThreadID() const
{
    return d->m_appMainThreadId;
}

bool ConsoleProcess::start(const QString &program, const QString &args)
{
    if (isRunning())
        return false;

    QString pcmd;
    QString pargs;
    if (d->m_mode != Run) { // The debugger engines already pre-process the arguments.
        pcmd = program;
        pargs = args;
    } else {
        QtcProcess::prepareCommand(program, args, &pcmd, &pargs, &d->m_environment, &d->m_workingDir);
    }

    const QString err = stubServerListen();
    if (!err.isEmpty()) {
        emit processError(msgCommChannelFailed(err));
        return false;
    }

    QStringList env = d->m_environment.toStringList();
    if (!env.isEmpty()) {
        d->m_tempFile = new QTemporaryFile();
        if (!d->m_tempFile->open()) {
            stubServerShutdown();
            emit processError(msgCannotCreateTempFile(d->m_tempFile->errorString()));
            delete d->m_tempFile;
            d->m_tempFile = 0;
            return false;
        }
        QTextStream out(d->m_tempFile);
        out.setCodec("UTF-16LE");
        out.setGenerateByteOrderMark(false);
        foreach (const QString &var, fixWinEnvironment(env))
            out << var << QChar(0);
        out << QChar(0);
        out.flush();
        if (out.status() != QTextStream::Ok) {
            stubServerShutdown();
            emit processError(msgCannotWriteTempFile());
            delete d->m_tempFile;
            d->m_tempFile = 0;
            return false;
        }
    }

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    d->m_pid = new PROCESS_INFORMATION;
    ZeroMemory(d->m_pid, sizeof(PROCESS_INFORMATION));

    QString workDir = QDir::toNativeSeparators(workingDirectory());
    if (!workDir.isEmpty() && !workDir.endsWith(QLatin1Char('\\')))
        workDir.append(QLatin1Char('\\'));

    QStringList stubArgs;
    stubArgs << modeOption(d->m_mode)
             << d->m_stubServer.fullServerName()
             << workDir
             << (d->m_tempFile ? d->m_tempFile->fileName() : QString())
             << createWinCommandline(pcmd, pargs)
             << msgPromptToClose();

    const QString cmdLine = createWinCommandline(
            QCoreApplication::applicationDirPath() + QLatin1String("/qtcreator_process_stub.exe"), stubArgs);

    bool success = CreateProcessW(0, (WCHAR*)cmdLine.utf16(),
                                  0, 0, FALSE, CREATE_NEW_CONSOLE,
                                  0, 0,
                                  &si, d->m_pid);

    if (!success) {
        delete d->m_pid;
        d->m_pid = 0;
        delete d->m_tempFile;
        d->m_tempFile = 0;
        stubServerShutdown();
        emit processError(tr("The process '%1' could not be started: %2").arg(cmdLine, winErrorMessage(GetLastError())));
        return false;
    }

    d->processFinishedNotifier = new QWinEventNotifier(d->m_pid->hProcess, this);
    connect(d->processFinishedNotifier, SIGNAL(activated(HANDLE)), SLOT(stubExited()));
    return true;
}


void ConsoleProcess::killProcess()
{
    if (d->m_hInferior != NULL) {
        TerminateProcess(d->m_hInferior, (unsigned)-1);
        cleanupInferior();
    }
}

void ConsoleProcess::killStub()
{
    if (d->m_pid) {
        TerminateProcess(d->m_pid->hProcess, (unsigned)-1);
        WaitForSingleObject(d->m_pid->hProcess, INFINITE);
        cleanupStub();
    }
}

void ConsoleProcess::stop()
{
    killProcess();
    killStub();
}

bool ConsoleProcess::isRunning() const
{
    return d->m_pid != 0;
}

QString ConsoleProcess::stubServerListen()
{
    if (d->m_stubServer.listen(QString::fromLatin1("creator-%1-%2")
                            .arg(QCoreApplication::applicationPid())
                            .arg(rand())))
        return QString();
    return d->m_stubServer.errorString();
}

void ConsoleProcess::stubServerShutdown()
{
    delete d->m_stubSocket;
    d->m_stubSocket = 0;
    if (d->m_stubServer.isListening())
        d->m_stubServer.close();
}

void ConsoleProcess::stubConnectionAvailable()
{
    emit stubStarted();
    d->m_stubSocket = d->m_stubServer.nextPendingConnection();
    connect(d->m_stubSocket, SIGNAL(readyRead()), SLOT(readStubOutput()));
}

void ConsoleProcess::readStubOutput()
{
    while (d->m_stubSocket->canReadLine()) {
        QByteArray out = d->m_stubSocket->readLine();
        out.chop(2); // \r\n
        if (out.startsWith("err:chdir ")) {
            emit processError(msgCannotChangeToWorkDir(workingDirectory(), winErrorMessage(out.mid(10).toInt())));
        } else if (out.startsWith("err:exec ")) {
            emit processError(msgCannotExecute(d->m_executable, winErrorMessage(out.mid(9).toInt())));
        } else if (out.startsWith("thread ")) { // Windows only
            d->m_appMainThreadId = out.mid(7).toLongLong();
        } else if (out.startsWith("pid ")) {
            // Will not need it any more
            delete d->m_tempFile;
            d->m_tempFile = 0;
            d->m_appPid = out.mid(4).toLongLong();

            d->m_hInferior = OpenProcess(
                    SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE,
                    FALSE, d->m_appPid);
            if (d->m_hInferior == NULL) {
                emit processError(tr("Cannot obtain a handle to the inferior: %1")
                                  .arg(winErrorMessage(GetLastError())));
                // Uhm, and now what?
                continue;
            }
            d->inferiorFinishedNotifier = new QWinEventNotifier(d->m_hInferior, this);
            connect(d->inferiorFinishedNotifier, SIGNAL(activated(HANDLE)), SLOT(inferiorExited()));
            emit processStarted();
        } else {
            emit processError(msgUnexpectedOutput(out));
            TerminateProcess(d->m_pid->hProcess, (unsigned)-1);
            break;
        }
    }
}

void ConsoleProcess::cleanupInferior()
{
    delete d->inferiorFinishedNotifier;
    d->inferiorFinishedNotifier = 0;
    CloseHandle(d->m_hInferior);
    d->m_hInferior = NULL;
    d->m_appPid = 0;
}

void ConsoleProcess::inferiorExited()
{
    DWORD chldStatus;

    if (!GetExitCodeProcess(d->m_hInferior, &chldStatus))
        emit processError(tr("Cannot obtain exit status from inferior: %1")
                          .arg(winErrorMessage(GetLastError())));
    cleanupInferior();
    d->m_appStatus = QProcess::NormalExit;
    d->m_appCode = chldStatus;
    emit processStopped();
}

void ConsoleProcess::cleanupStub()
{
    stubServerShutdown();
    delete d->processFinishedNotifier;
    d->processFinishedNotifier = 0;
    CloseHandle(d->m_pid->hThread);
    CloseHandle(d->m_pid->hProcess);
    delete d->m_pid;
    d->m_pid = 0;
    delete d->m_tempFile;
    d->m_tempFile = 0;
}

void ConsoleProcess::stubExited()
{
    // The stub exit might get noticed before we read the pid for the kill.
    if (d->m_stubSocket && d->m_stubSocket->state() == QLocalSocket::ConnectedState)
        d->m_stubSocket->waitForDisconnected();
    cleanupStub();
    if (d->m_hInferior != NULL) {
        TerminateProcess(d->m_hInferior, (unsigned)-1);
        cleanupInferior();
        d->m_appStatus = QProcess::CrashExit;
        d->m_appCode = -1;
        emit processStopped();
    }
    emit stubStopped();
}

QStringList ConsoleProcess::fixWinEnvironment(const QStringList &env)
{
    QStringList envStrings = env;
    // add PATH if necessary (for DLL loading)
    if (envStrings.filter(QRegExp(QLatin1String("^PATH="),Qt::CaseInsensitive)).isEmpty()) {
        QByteArray path = qgetenv("PATH");
        if (!path.isEmpty())
            envStrings.prepend(QString(QLatin1String("PATH=%1")).arg(QString::fromLocal8Bit(path)));
    }
    // add systemroot if needed
    if (envStrings.filter(QRegExp(QLatin1String("^SystemRoot="),Qt::CaseInsensitive)).isEmpty()) {
        QByteArray systemRoot = qgetenv("SystemRoot");
        if (!systemRoot.isEmpty())
            envStrings.prepend(QString(QLatin1String("SystemRoot=%1")).arg(QString::fromLocal8Bit(systemRoot)));
    }
    return envStrings;
}

static QString quoteWinCommand(const QString &program)
{
    const QChar doubleQuote = QLatin1Char('"');

    // add the program as the first arg ... it works better
    QString programName = program;
    programName.replace(QLatin1Char('/'), QLatin1Char('\\'));
    if (!programName.startsWith(doubleQuote) && !programName.endsWith(doubleQuote)
            && programName.contains(QLatin1Char(' '))) {
        programName.prepend(doubleQuote);
        programName.append(doubleQuote);
    }
    return programName;
}

static QString quoteWinArgument(const QString &arg)
{
    if (!arg.length())
        return QString::fromLatin1("\"\"");

    QString ret(arg);
    // Quotes are escaped and their preceding backslashes are doubled.
    ret.replace(QRegExp(QLatin1String("(\\\\*)\"")), QLatin1String("\\1\\1\\\""));
    if (ret.contains(QRegExp(QLatin1String("\\s")))) {
        // The argument must not end with a \ since this would be interpreted
        // as escaping the quote -- rather put the \ behind the quote: e.g.
        // rather use "foo"\ than "foo\"
        int i = ret.length();
        while (i > 0 && ret.at(i - 1) == QLatin1Char('\\'))
            --i;
        ret.insert(i, QLatin1Char('"'));
        ret.prepend(QLatin1Char('"'));
    }
    return ret;
}

QString ConsoleProcess::createWinCommandline(const QString &program, const QStringList &args)
{
    QString programName = quoteWinCommand(program);
    foreach (const QString &arg, args) {
        programName += QLatin1Char(' ');
        programName += quoteWinArgument(arg);
    }
    return programName;
}

QString ConsoleProcess::createWinCommandline(const QString &program, const QString &args)
{
    QString programName = quoteWinCommand(program);
    if (!args.isEmpty()) {
        programName += QLatin1Char(' ');
        programName += args;
    }
    return programName;
}

QString ConsoleProcess::defaultTerminalEmulator()
{
    return QString::fromLocal8Bit(qgetenv("COMSPEC"));
}

QStringList ConsoleProcess::availableTerminalEmulators()
{
    return QStringList(ConsoleProcess::defaultTerminalEmulator());
}

} // namespace Utils
