/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "terminal.h"

#include <QDebug>
#include <QIODevice>
#include <QSocketNotifier>

#include <utils/qtcassert.h>

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

namespace Debugger {
namespace Internal {

static QString currentError()
{
    int err = errno;
    return QString::fromLatin1(strerror(err));
}

Terminal::Terminal(QObject *parent)
   : QObject(parent), m_isUsable(false), m_masterFd(-1), m_masterReader(0)
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
    Q_UNUSED(msg);
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
    Q_UNUSED(fd);
#endif
}

} // namespace Internal
} // namespace Debugger

