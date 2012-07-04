/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "crashhandler.h"
#include "crashhandlerdialog.h"
#include "backtracecollector.h"
#include "utils.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QUrl>

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>

static const char FileDistroInformation[] = "/etc/lsb-release";
static const char FileKernelVersion[] = "/proc/version";

static QString collectLinuxDistributionInfo()
{
    return QString::fromLatin1(fileContents(QLatin1String(FileDistroInformation)));
}

static QString collectKernelVersionInfo()
{
    return QString::fromLatin1(fileContents(QLatin1String(FileKernelVersion)));
}

class CrashHandlerPrivate
{
public:
    CrashHandlerPrivate(pid_t pid, CrashHandler *crashHandler)
        : pid(pid), dialog(crashHandler), argv(0), envp(0) {}

    ~CrashHandlerPrivate()
    {
        if (argv) {
            for (int i = 0; argv[i]; ++i)
                delete[] argv[i];
        }
        if (envp) {
            for (int i = 0; envp[i]; ++i)
                delete[] envp[i];
        }
        free(argv);
        free(envp);
    }

    pid_t pid;
    BacktraceCollector backtraceCollector;
    CrashHandlerDialog dialog;

    // For restarting the process.
    char **argv;
    char **envp;
};

CrashHandler::CrashHandler(pid_t pid, QObject *parent)
    : QObject(parent), d(new CrashHandlerPrivate(pid, this))
{
    connect(&d->backtraceCollector, SIGNAL(error(QString)), SLOT(onError(QString)));
    connect(&d->backtraceCollector, SIGNAL(backtraceChunk(QString)), SLOT(onBacktraceChunk(QString)));
    connect(&d->backtraceCollector, SIGNAL(backtrace(QString)), SLOT(onBacktraceFinished(QString)));

    d->dialog.appendDebugInfo(collectKernelVersionInfo());
    d->dialog.appendDebugInfo(collectLinuxDistributionInfo());
    d->dialog.show();

    if (!collectRestartAppData()) // If we can't restart the app properly, ...
        d->dialog.disableRestartAppButton();
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
    const QString text = QLatin1String("There occured a problem providing the backtrace. "
        "Please make sure to have the debugger \"gdb\" installed.\n");
    d->dialog.appendDebugInfo(text);
    d->dialog.appendDebugInfo(errorMessage);
}

void CrashHandler::onBacktraceChunk(const QString &chunk)
{
    d->dialog.appendDebugInfo(chunk);
    QTextStream out(stdout);
    out << chunk;
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
    QRegExp rx("\\[Current thread is (\\d+)");
    const int pos = rx.indexIn(backtrace);
    if (pos == -1)
        return;
    const QString threadNumber = rx.cap(1);
    const QString textToSelect = QString::fromLatin1("Thread %1").arg(threadNumber);
    d->dialog.selectLineWithContents(textToSelect);
}

void CrashHandler::openBugTracker()
{
    QDesktopServices::openUrl(QUrl(URL_BUGTRACKER));
}

bool CrashHandler::collectRestartAppData()
{
    const QString procDir = QString::fromLatin1("/proc/%1").arg(d->pid);

    // Construct d->argv.
    // man 5 proc: /proc/[pid]/cmdline
    // The command-line arguments appear in this file as a set of strings separated by
    // null bytes ('\0'), with a further null byte after the last string.
    const QString procCmdFileName = procDir + QLatin1String("/cmdline");
    QList<QByteArray> cmdEntries = fileContents(procCmdFileName).split('\0');
    if (cmdEntries.size() < 2) {
        qWarning("%s: Unexpected format in file '%s'.\n", Q_FUNC_INFO, qPrintable(procCmdFileName));
        return false;
    }
    cmdEntries.removeLast();
    char * const executable = qstrdup(qPrintable(cmdEntries.takeFirst()));
    d->argv = (char **) malloc(sizeof(char*) * (cmdEntries.size() + 2));
    if (d->argv == 0)
        qFatal("%s: malloc() failed.\n", Q_FUNC_INFO);
    d->argv[0] = executable;
    int i;
    for (i = 1; i <= cmdEntries.size(); ++i)
        d->argv[i] = qstrdup(cmdEntries.at(i-1));
    d->argv[i] = 0;

    // Construct d->envp.
    // man 5 proc: /proc/[pid]/environ
    // The entries are separated by null bytes ('\0'), and there may be a null  byte at the end.
    const QString procEnvFileName = procDir + QLatin1String("/environ");
    QList<QByteArray> envEntries = fileContents(procEnvFileName).split('\0');
    if (envEntries.isEmpty()) {
        qWarning("%s: Unexpected format in file '%s'.\n", Q_FUNC_INFO, qPrintable(procEnvFileName));
        return false;
    }
    if (envEntries.last().isEmpty())
        envEntries.removeLast();
    d->envp = (char **) malloc(sizeof(char*) * (envEntries.size() + 1));
    if (d->envp == 0)
        qFatal("%s: malloc() failed.\n", Q_FUNC_INFO);
    for (i = 0; i < envEntries.size(); ++i)
        d->envp[i] = qstrdup(envEntries.at(i));
    d->envp[i] = 0;

    return true;
}

void CrashHandler::restartApplication()
{
    // TODO: If QTBUG-2284 is resolved, use QProcess::startDetached() here.
    // Close the crash handler and start the process again with same environment and
    // command line arguments.
    //
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
    case 0: // child
        qDebug("Restarting Qt Creator with\n");
        for (int i = 0; d->argv[i]; ++i)
            qDebug("   %s", d->argv[i]);
        qDebug("\nand environment\n");
        for (int i = 0; d->envp[i]; ++i)
            qDebug("   %s", d->envp[i]);

        // The standards pipes must be open, otherwise the restarted Qt Creator will
        // receive a SIGPIPE as soon as these are used.
        if (freopen("/dev/null", "r", stdin) == 0)
            qFatal("%s: freopen() failed for stdin: %s.\n", Q_FUNC_INFO, strerror(errno));
        if (freopen("/dev/null", "w", stdout) == 0)
            qFatal("%s: freopen() failed for stdout: %s.\n", Q_FUNC_INFO, strerror(errno));
        if (freopen("/dev/null", "w", stderr) == 0)
            qFatal("%s: freopen() failed for stderr: %s.\n.", Q_FUNC_INFO, strerror(errno));

        execve(d->argv[0], d->argv, d->envp);
        _exit(EXIT_FAILURE);
    default: // parent
        qApp->quit();
        break;
    }
}
