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

#include "terminal.h"

#include <coreplugin/icore.h>

#include <projectexplorer/runconfiguration.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

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

namespace Debugger {
namespace Internal {

static QString currentError()
{
    int err = errno;
    return QString::fromLatin1(strerror(err));
}

Terminal::Terminal(QObject *parent)
   : QObject(parent)
{
}

void Terminal::setup()
{
#ifdef DEBUGGER_USE_TERMINAL
    if (!qEnvironmentVariableIsSet("QTC_USE_PTY"))
        return;

    m_masterFd = ::open("/dev/ptmx", O_RDWR);
    if (m_masterFd < 0) {
        error(tr("Terminal: Cannot open /dev/ptmx: %1").arg(currentError()));
        return;
    }

    const char *sName = ptsname(m_masterFd);
    if (!sName) {
        error(tr("Terminal: ptsname failed: %1").arg(currentError()));
        return;
    }
    m_slaveName = sName;

    struct stat s;
    int r = ::stat(m_slaveName.constData(), &s);
    if (r != 0) {
        error(tr("Terminal: Error: %1").arg(currentError()));
        return;
    }
    if (!S_ISCHR(s.st_mode)) {
        error(tr("Terminal: Slave is no character device."));
        return;
    }

    m_masterReader = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    connect(m_masterReader, &QSocketNotifier::activated,
            this, &Terminal::onSlaveReaderActivated);

    r = grantpt(m_masterFd);
    if (r != 0) {
        error(tr("Terminal: grantpt failed: %1").arg(currentError()));
        return;
    }

    r = unlockpt(m_masterFd);
    if (r != 0) {
        error(tr("Terminal: unlock failed: %1").arg(currentError()));
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
        error(tr("Terminal: Read failed: %1").arg(QString::fromLatin1(strerror(err))));
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
                               const std::function<Runnable()> &stubRunnable)
    : RunWorker(runControl), m_stubRunnable(stubRunnable)
{
    setId("TerminalRunner");
}

void TerminalRunner::kickoffProcess()
{
    if (m_stubProc)
        m_stubProc->kickoffProcess();
}

void TerminalRunner::interruptProcess()
{
    if (m_stubProc)
        m_stubProc->interruptProcess();
}

void TerminalRunner::start()
{
    QTC_ASSERT(m_stubRunnable, reportFailure({}); return);
    QTC_ASSERT(!m_stubProc, reportFailure({}); return);
    Runnable stub = m_stubRunnable();

    m_stubProc = new QtcProcess(this);
    m_stubProc->setTerminalMode(HostOsInfo::isWindowsHost()
            ? TerminalMode::Suspend : TerminalMode::Debug);

    connect(m_stubProc, &QtcProcess::errorOccurred,
            this, &TerminalRunner::stubError);
    connect(m_stubProc, &QtcProcess::started,
            this, &TerminalRunner::stubStarted);
    connect(m_stubProc, &QtcProcess::finished,
            this, &TerminalRunner::reportDone);

    m_stubProc->setEnvironment(stub.environment);
    m_stubProc->setWorkingDirectory(stub.workingDirectory);

    // Error message for user is delivered via a signal.
    m_stubProc->setCommand(stub.command);
    m_stubProc->start();
}

void TerminalRunner::stop()
{
    if (m_stubProc)
        m_stubProc->stopProcess();
    reportStopped();
}

void TerminalRunner::stubStarted()
{
    m_applicationPid = m_stubProc->processId();
    m_applicationMainThreadId = m_stubProc->applicationMainThreadID();
    reportStarted();
}

void TerminalRunner::stubError()
{
    reportFailure(m_stubProc->errorString());
}

} // namespace Internal
} // namespace Debugger

