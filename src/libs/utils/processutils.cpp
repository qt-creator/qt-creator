// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processutils.h"

#include "commandline.h"
#include "qtcprocess.h"
#include "utilstr.h"

#include <QThread>

#ifdef QT_GUI_LIB
#include <QGuiApplication>
#include <QMessageBox>
#endif

using namespace std::chrono;

namespace Utils {

static bool isGuiEnabled()
{
#ifdef QT_GUI_LIB
    static bool isGuiApp = qobject_cast<QGuiApplication *>(qApp);
    return isGuiApp && QThread::isMainThread();
#else
    return false;
#endif
}

static bool askToKill(const CommandLine &command)
{
#ifdef QT_GUI_LIB
    if (!isGuiEnabled())
        return true;
    const QString title = Tr::tr("Process Not Responding");
    QString msg = command.isEmpty() ? Tr::tr("The process is not responding.")
                                    : Tr::tr("The process \"%1\" is not responding.")
                                          .arg(command.executable().toUserOutput());
    msg += ' ';
    msg += Tr::tr("Terminate the process?");
    // Restore the cursor that is set to wait while running.
    const bool hasOverrideCursor = QGuiApplication::overrideCursor() != nullptr;
    if (hasOverrideCursor)
        QGuiApplication::restoreOverrideCursor();
    QMessageBox::StandardButton answer = QMessageBox::question(nullptr, title, msg, QMessageBox::Yes|QMessageBox::No);
    if (hasOverrideCursor)
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    return answer == QMessageBox::Yes;
#else
    Q_UNUSED(command)
    return true;
#endif
}

// Helper for running a process synchronously in the foreground with timeout
// detection (taking effect after no more output
// occurs on stderr/stdout as opposed to waitForFinished()). Returns false if a timeout
// occurs. Checking of the process' exit state/code still has to be done.

bool readDataFromProcess(Process &process, QByteArray *stdOut, QByteArray *stdErr, int timeoutS)
{
    enum { syncDebug = 0 };
    if (syncDebug)
        qDebug() << ">readDataFromProcess" << timeoutS;
    if (process.state() != ProcessState::Running) {
        qWarning("readDataFromProcess: Process in non-running state passed in.");
        return false;
    }

    // Keep the process running until it has no longer has data
    bool finished = false;
    bool hasData = false;
    do {
        finished = process.waitForFinished(timeoutS > 0 ? seconds(timeoutS) : seconds(-1))
        || process.state() == ProcessState::NotRunning;
        // First check 'stdout'
        const QByteArray newStdOut = process.readAllRawStandardOutput();
        if (!newStdOut.isEmpty()) {
            hasData = true;
            if (stdOut)
                stdOut->append(newStdOut);
        }
        // Check 'stderr' separately. This is a special handling
        // for 'git pull' and the like which prints its progress on stderr.
        const QByteArray newStdErr = process.readAllRawStandardError();
        if (!newStdErr.isEmpty()) {
            hasData = true;
            if (stdErr)
                stdErr->append(newStdErr);
        }
        // Prompt user, pretend we have data if says 'No'.
        const bool hang = !hasData && !finished;
        hasData = hang && !askToKill(process.commandLine());
    } while (hasData && !finished);
    if (syncDebug)
        qDebug() << "<readDataFromProcess" << finished;
    return finished;
}
} // Utils
