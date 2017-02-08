/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "crashhandler.h"
#include "crashhandlerdialog.h"
#include "backtracecollector.h"
#include "utils.h"

#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QUrl>
#include <QVector>

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

static const char FileDistroInformation[] = "/etc/lsb-release";
static const char FileKernelVersion[] = "/proc/version";
static const char QtCreatorExecutable[] = "qtcreator";

static QString collectLinuxDistributionInfo()
{
    return QString::fromLatin1(fileContents(QLatin1String(FileDistroInformation)));
}

static QString collectKernelVersionInfo()
{
    return QString::fromLatin1(fileContents(QLatin1String(FileKernelVersion)));
}

// Convience class for interacting with exec() family of functions.
class CExecList : public QVector<char *>
{
public:
    CExecList(const QStringList &list)
    {
        foreach (const QString &item, list)
            append(qstrdup(item.toLatin1().data()));
        append(0);
    }

    ~CExecList()
    {
        for (int i = 0; i < size(); ++i)
            delete[] value(i);
    }
};

class CrashHandlerPrivate
{
public:
    CrashHandlerPrivate(pid_t pid,
                        const QString &signalName,
                        const QString &appName,
                        CrashHandler *crashHandler)
        : pid(pid),
          creatorInPath(Utils::Environment::systemEnvironment().searchInPath(QLatin1String(QtCreatorExecutable))),
          dialog(crashHandler, signalName, appName) {}

    const pid_t pid;
    const Utils::FileName creatorInPath; // Backup debugger.

    BacktraceCollector backtraceCollector;
    CrashHandlerDialog dialog;

    QStringList restartAppCommandLine;
    QStringList restartAppEnvironment;
};

CrashHandler::CrashHandler(pid_t pid,
                           const QString &signalName,
                           const QString &appName,
                           RestartCapability restartCap,
                           QObject *parent)
    : QObject(parent), d(new CrashHandlerPrivate(pid, signalName, appName, this))
{
    connect(&d->backtraceCollector, &BacktraceCollector::error, this, &CrashHandler::onError);
    connect(&d->backtraceCollector, &BacktraceCollector::backtraceChunk,
            this, &CrashHandler::onBacktraceChunk);
    connect(&d->backtraceCollector, &BacktraceCollector::backtrace,
            this, &CrashHandler::onBacktraceFinished);

    d->dialog.appendDebugInfo(collectKernelVersionInfo());
    d->dialog.appendDebugInfo(collectLinuxDistributionInfo());

    if (restartCap == DisableRestart || !collectRestartAppData()) {
        d->dialog.disableRestartAppCheckBox();
        if (d->creatorInPath.isEmpty())
            d->dialog.disableDebugAppButton();
    }

    d->dialog.show();
}

CrashHandler::~CrashHandler()
{
    delete d;
}

void CrashHandler::run()
{
    d->backtraceCollector.run(d->pid);
}

void CrashHandler::onError(const QString &errorMessage)
{
    d->dialog.setToFinalState();

    QTextStream(stderr) << errorMessage;
    const QString text = QLatin1String("A problem occurred providing the backtrace. "
        "Please make sure to have the debugger \"gdb\" installed.\n");
    d->dialog.appendDebugInfo(text);
    d->dialog.appendDebugInfo(errorMessage);
}

void CrashHandler::onBacktraceChunk(const QString &chunk)
{
    d->dialog.appendDebugInfo(chunk);
}

void CrashHandler::onBacktraceFinished(const QString &backtrace)
{
    d->dialog.setToFinalState();

    // Select first line of relevant thread.

    // Example debugger output:
    // ...
    // [Current thread is 1 (Thread 0x7f1c33c79780 (LWP 975))]
    // ...
    // Thread 1 (Thread 0x7f1c33c79780 (LWP 975)):
    // ...
    QRegExp rx(QLatin1String("\\[Current thread is (\\d+)"));
    const int pos = rx.indexIn(backtrace);
    if (pos == -1)
        return;
    const QString threadNumber = rx.cap(1);
    const QString textToSelect = QString::fromLatin1("Thread %1").arg(threadNumber);
    d->dialog.selectLineWithContents(textToSelect);
}

void CrashHandler::openBugTracker()
{
    QDesktopServices::openUrl(QUrl(QLatin1String(URL_BUGTRACKER)));
}

bool CrashHandler::collectRestartAppData()
{
    const QString procDir = QString::fromLatin1("/proc/%1").arg(d->pid);

    // Get command line.
    // man 5 proc: /proc/[pid]/cmdline
    // The command-line arguments appear in this file as a set of strings separated by
    // null bytes ('\0'), with a further null byte after the last string.
    const QString procCmdFileName = procDir + QLatin1String("/cmdline");
    QList<QByteArray> commandLine = fileContents(procCmdFileName).split('\0');
    if (commandLine.size() < 2) {
        qWarning("%s: Unexpected format in file '%s'.\n", Q_FUNC_INFO, qPrintable(procCmdFileName));
        return false;
    }
    commandLine.removeLast();
    foreach (const QByteArray &item, commandLine)
        d->restartAppCommandLine.append(QString::fromLatin1(item));

    // Get environment.
    // man 5 proc: /proc/[pid]/environ
    // The entries are separated by null bytes ('\0'), and there may be a null byte at the end.
    const QString procEnvFileName = procDir + QLatin1String("/environ");
    QList<QByteArray> environment = fileContents(procEnvFileName).split('\0');
    if (environment.isEmpty()) {
        qWarning("%s: Unexpected format in file '%s'.\n", Q_FUNC_INFO, qPrintable(procEnvFileName));
        return false;
    }
    if (environment.last().isEmpty())
        environment.removeLast();
    foreach (const QByteArray &item, environment)
        d->restartAppEnvironment.append(QString::fromLatin1(item));

    return true;
}

void CrashHandler::runCommand(QStringList commandLine, QStringList environment, WaitMode waitMode)
{
    // TODO: If QTBUG-2284 is resolved, use QProcess::startDetached() here.
    // We can't use QProcess::startDetached because of bug
    //
    //      QTBUG-2284
    //      QProcess::startDetached does not support setting an environment for the new process
    //
    // therefore, we use fork-exec.

    pid_t pid = fork();
    switch (pid) {
    case -1: // error
        qFatal("%s: fork() failed.", Q_FUNC_INFO);
        break;
    case 0: { // child
        CExecList argv(commandLine);
        CExecList envp(environment);
        qDebug("Running\n");
        for (int i = 0; argv[i]; ++i)
            qDebug("   %s", argv[i]);
        if (!environment.isEmpty()) {
            qDebug("\nwith environment:\n");
            for (int i = 0; envp[i]; ++i)
                qDebug("   %s", envp[i]);
        }

        // The standards pipes must be open, otherwise the application will
        // receive a SIGPIPE as soon as these are used.
        if (freopen("/dev/null", "r", stdin) == 0)
            qFatal("%s: freopen() failed for stdin: %s.\n", Q_FUNC_INFO, strerror(errno));
        if (freopen("/dev/null", "w", stdout) == 0)
            qFatal("%s: freopen() failed for stdout: %s.\n", Q_FUNC_INFO, strerror(errno));
        if (freopen("/dev/null", "w", stderr) == 0)
            qFatal("%s: freopen() failed for stderr: %s.\n.", Q_FUNC_INFO, strerror(errno));

        if (environment.isEmpty())
            execvp(argv[0], argv.data());
        else
            execvpe(argv[0], argv.data(), envp.data());
        _exit(EXIT_FAILURE);
    } default: // parent
        if (waitMode == WaitForExit) {
            while (true) {
                int status;
                if (waitpid(pid, &status, 0) == -1) {
                    if (errno == EINTR) // Signal handler of QProcess for SIGCHLD was triggered.
                        continue;
                    perror("waitpid() failed unexpectedly");
                }
                if (WIFEXITED(status)) {
                    qDebug("Child exited with exit code %d.", WEXITSTATUS(status));
                    break;
                } else if (WIFSIGNALED(status)) {
                    qDebug("Child terminated by signal %d.", WTERMSIG(status));
                    break;
                }
            }
        }
        break;
    }
}

void CrashHandler::restartApplication()
{
    runCommand(d->restartAppCommandLine, d->restartAppEnvironment, DontWaitForExit);
}

void CrashHandler::debugApplication()
{
    // User requested to debug the app while our debugger is running.
    if (d->backtraceCollector.isRunning()) {
        if (!d->dialog.runDebuggerWhileBacktraceNotFinished())
            return;
        if (d->backtraceCollector.isRunning()) {
            d->backtraceCollector.disconnect();
            d->backtraceCollector.kill();
            d->dialog.setToFinalState();
            d->dialog.appendDebugInfo(tr("\n\nCollecting backtrace aborted by user."));
            QCoreApplication::processEvents(); // Show the last appended output immediately.
        }
    }

    // Prepare command.
    QString executable = d->creatorInPath.toString();
    if (executable.isEmpty() && !d->restartAppCommandLine.isEmpty())
        executable = d->restartAppCommandLine.at(0);
    const QStringList commandLine = QStringList({ executable, "-debug", QString::number(d->pid) });
    QStringList environment;
    if (!d->restartAppEnvironment.isEmpty())
        environment = d->restartAppEnvironment;

    // The UI is blocked/frozen anyway, so hide the dialog while debugging.
    d->dialog.hide();
    runCommand(commandLine, environment, WaitForExit);
    d->dialog.show();
}
