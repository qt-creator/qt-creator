// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "terminal.h"

#include "debuggertr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QIODevice>
#include <QSocketNotifier>

#ifdef Q_OS_UNIX
#   define DEBUGGER_USE_TERMINAL
#endif

#ifdef DEBUGGER_USE_TERMINAL
#   include <errno.h>
#   include <fcntl.h>
#   include <stdlib.h>
#   include <string.h>
#   include <unistd.h>
#   include <sys/ioctl.h>
#   include <sys/stat.h>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger::Internal {

Terminal::Terminal(QObject *parent)
   : QObject(parent)
{
}

void Terminal::setup()
{
#ifdef DEBUGGER_USE_TERMINAL
    const auto currentError = [] {
        int err = errno;
        return QString::fromLatin1(strerror(err));
    };
    if (!qtcEnvironmentVariableIsSet("QTC_USE_PTY"))
        return;

    m_masterFd = ::open("/dev/ptmx", O_RDWR);
    if (m_masterFd < 0) {
        error(Tr::tr("Terminal: Cannot open /dev/ptmx: %1").arg(currentError()));
        return;
    }

    const char *sName = ptsname(m_masterFd);
    if (!sName) {
        error(Tr::tr("Terminal: ptsname failed: %1").arg(currentError()));
        return;
    }
    m_slaveName = sName;

    struct stat s;
    int r = ::stat(m_slaveName.constData(), &s);
    if (r != 0) {
        error(Tr::tr("Terminal: Error: %1").arg(currentError()));
        return;
    }
    if (!S_ISCHR(s.st_mode)) {
        error(Tr::tr("Terminal: Slave is no character device."));
        return;
    }

    m_masterReader = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    connect(m_masterReader, &QSocketNotifier::activated,
            this, &Terminal::onSlaveReaderActivated);

    r = grantpt(m_masterFd);
    if (r != 0) {
        error(Tr::tr("Terminal: grantpt failed: %1").arg(currentError()));
        return;
    }

    r = unlockpt(m_masterFd);
    if (r != 0) {
        error(Tr::tr("Terminal: unlock failed: %1").arg(currentError()));
        return;
    }

    m_isUsable = true;
#endif
}

bool Terminal::isUsable() const
{
    return m_isUsable;
}

int Terminal::write(const QByteArray &msg)
{
#ifdef DEBUGGER_USE_TERMINAL
    return ::write(m_masterFd, msg.constData(), msg.size());
#else
    Q_UNUSED(msg)
    return -1;
#endif
}

bool Terminal::sendInterrupt()
{
#ifdef DEBUGGER_USE_TERMINAL
    if (!m_isUsable)
        return false;
    ssize_t written = ::write(m_masterFd, "\003", 1);
    return written == 1;
#else
    return false;
#endif
}

void Terminal::onSlaveReaderActivated(int fd)
{
#ifdef DEBUGGER_USE_TERMINAL
    ssize_t available = 0;
    int ret = ::ioctl(fd, FIONREAD, (char *) &available);
    if (ret != 0)
        return;

    QByteArray buffer(available, Qt::Uninitialized);
    ssize_t got = ::read(fd, buffer.data(), available);
    int err = errno;
    if (got < 0) {
        error(Tr::tr("Terminal: Read failed: %1").arg(QString::fromLatin1(strerror(err))));
        return;
    }
    buffer.resize(got);
    if (got >= 0)
        stdOutReady(QString::fromUtf8(buffer));
#else
    Q_UNUSED(fd)
#endif
}

TerminalRunner::TerminalRunner(RunControl *runControl,
                               const std::function<ProcessRunData()> &stubRunnable)
    : RunWorker(runControl), m_stubRunnable(stubRunnable)
{
    setId("TerminalRunner");
}

void TerminalRunner::kickoffProcess()
{
    if (m_stubProc)
        m_stubProc->kickoffProcess();
}

void TerminalRunner::interrupt()
{
    if (m_stubProc)
        m_stubProc->interrupt();
}

void TerminalRunner::start()
{
    QTC_ASSERT(m_stubRunnable, reportFailure({}); return);
    QTC_ASSERT(!m_stubProc, reportFailure({}); return);
    ProcessRunData stub = m_stubRunnable();

    bool runAsRoot = false;
    if (auto runAsRootAspect = runControl()->aspect<RunAsRootAspect>())
        runAsRoot = runAsRootAspect->value;

    m_stubProc = new Process(this);
    m_stubProc->setTerminalMode(TerminalMode::Debug);

    if (runAsRoot) {
        m_stubProc->setRunAsRoot(runAsRoot);
        RunControl::provideAskPassEntry(stub.environment);
    }

    connect(m_stubProc, &Process::started,
            this, &TerminalRunner::stubStarted);
    connect(m_stubProc, &Process::done,
            this, &TerminalRunner::stubDone);

    m_stubProc->setEnvironment(stub.environment);
    m_stubProc->setWorkingDirectory(stub.workingDirectory);

    // Error message for user is delivered via a signal.
    m_stubProc->setCommand(stub.command);
    m_stubProc->start();
}

void TerminalRunner::stop()
{
    if (m_stubProc && m_stubProc->isRunning())
        m_stubProc->stop();
}

void TerminalRunner::stubStarted()
{
    m_applicationPid = m_stubProc->processId();
    m_applicationMainThreadId = m_stubProc->applicationMainThreadId();
    reportStarted();
}

void TerminalRunner::stubDone()
{
    if (m_stubProc->error() != QProcess::UnknownError)
        reportFailure(m_stubProc->errorString());
    if (m_stubProc->error() != QProcess::FailedToStart)
        reportDone();
}

} // Debugger::Internal

