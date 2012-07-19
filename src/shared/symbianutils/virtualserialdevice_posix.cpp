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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <QSocketNotifier>
#include <QTimer>
#include <QThread>
#include <QWaitCondition>
#include "virtualserialdevice.h"

namespace SymbianUtils {

class VirtualSerialDevicePrivate
{
public:
    int portHandle;
    QSocketNotifier* readNotifier;
    QSocketNotifier* writeUnblockedNotifier;
};

void VirtualSerialDevice::platInit()
{
    d = new VirtualSerialDevicePrivate;
    d->portHandle = -1;
    d->readNotifier = NULL;
    d->writeUnblockedNotifier = NULL;
    connect(this, SIGNAL(AsyncCall_emitBytesWrittenIfNeeded(qint64)), this, SIGNAL(bytesWritten(qint64)), Qt::QueuedConnection);
}

bool VirtualSerialDevice::open(OpenMode mode)
{
    if (isOpen()) return true;

    d->portHandle = ::open(portName.toAscii().constData(), O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (d->portHandle == -1) {
        setErrorString(tr("The port %1 could not be opened: %2 (POSIX error %3)").
                       arg(portName, QString::fromLocal8Bit(strerror(errno))).arg(errno));
        return false;
        }

    struct termios termInfo;
    if (tcgetattr(d->portHandle, &termInfo) < 0) {
        setErrorString(tr("Unable to retrieve terminal settings of port %1: %2 (POSIX error %3)").
                       arg(portName, QString::fromLocal8Bit(strerror(errno))).arg(errno));
        close();
        return false;
    }
    cfmakeraw(&termInfo);
    // Turn off terminal echo as not get messages back, among other things
    termInfo.c_cflag |= CREAD|CLOCAL;
    termInfo.c_cc[VTIME] = 0;
    termInfo.c_lflag &= (~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ISIG));
    termInfo.c_iflag &= (~(INPCK|IGNPAR|PARMRK|ISTRIP|ICRNL|IXANY|IXON|IXOFF));
    termInfo.c_oflag &= (~OPOST);
    termInfo.c_cc[VMIN]  = 0;
    termInfo.c_cc[VINTR] = _POSIX_VDISABLE;
    termInfo.c_cc[VQUIT] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTART] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTOP] = _POSIX_VDISABLE;
    termInfo.c_cc[VSUSP] = _POSIX_VDISABLE;

    if (tcsetattr(d->portHandle, TCSAFLUSH, &termInfo) < 0) {
        setErrorString(tr("Unable to apply terminal settings to port %1: %2 (POSIX error %3)").
                       arg(portName, QString::fromLocal8Bit(strerror(errno))).arg(errno));
        close();
        return false;
    }

    d->readNotifier = new QSocketNotifier(d->portHandle, QSocketNotifier::Read);
    connect(d->readNotifier, SIGNAL(activated(int)), this, SIGNAL(readyRead()));

    d->writeUnblockedNotifier = new QSocketNotifier(d->portHandle, QSocketNotifier::Write);
    d->writeUnblockedNotifier->setEnabled(false);
    connect(d->writeUnblockedNotifier, SIGNAL(activated(int)), this, SLOT(writeHasUnblocked(int)));

    bool ok = QIODevice::open(mode | QIODevice::Unbuffered);
    if (!ok) close();
    return ok;
}

void VirtualSerialDevice::platClose()
{
    delete d->readNotifier;
    d->readNotifier = NULL;

    delete d->writeUnblockedNotifier;
    d->writeUnblockedNotifier = NULL;

    ::close(d->portHandle);
    d->portHandle = -1;
}

VirtualSerialDevice::~VirtualSerialDevice()
{
    close();
    delete d;
}

qint64 VirtualSerialDevice::bytesAvailable() const
{
    QMutexLocker locker(&lock);
    if (!isOpen()) return 0;

    int avail = 0;
    d->readNotifier->setEnabled(false);
    int res = ioctl(d->portHandle, FIONREAD, &avail);
    d->readNotifier->setEnabled(true);
    if (res == -1) {
        return 0;
    }
    return (qint64)avail + QIODevice::bytesAvailable();
}

qint64 VirtualSerialDevice::readData(char *data, qint64 maxSize)
{
    QMutexLocker locker(&lock);
    d->readNotifier->setEnabled(false);
    int result = ::read(d->portHandle, data, maxSize);
    d->readNotifier->setEnabled(true);
    if (result == -1 && errno == EAGAIN)
        result = 0; // To Qt, 0 here means nothing ready right now, and -1 is reserved for permanent errors
    return result;
}

qint64 VirtualSerialDevice::writeData(const char *data, qint64 maxSize)
{
    QMutexLocker locker(&lock);
    qint64 bytesWritten;
    bool needToWait = tryFlushPendingBuffers(locker, EmitBytesWrittenAsync);
    if (!needToWait) {
        needToWait = tryWrite(data, maxSize, bytesWritten);
        if (needToWait && bytesWritten > 0) {
            // Wrote some of the buffer, adjust pointers to point to the remainder that needs queueing
            data += bytesWritten;
            maxSize -= bytesWritten;
        }
    }

    if (needToWait) {
        pendingWrites.append(QByteArray(data, maxSize));
        d->writeUnblockedNotifier->setEnabled(true);
        // Now wait for the writeUnblocked signal or for a call to waitForBytesWritten
        return bytesWritten + maxSize;
    } else {
        //emitBytesWrittenIfNeeded(locker, bytesWritten);
        // Can't emit bytesWritten directly from writeData - means clients end up recursing
        emit AsyncCall_emitBytesWrittenIfNeeded(bytesWritten);
        return bytesWritten;
    }
}

/* Returns true if EAGAIN encountered.
 * if error occurred (other than EAGAIN) returns -1 in bytesWritten
 * lock must be held. Doesn't emit signals or set notifiers.
 */
bool VirtualSerialDevice::tryWrite(const char *data, qint64 maxSize, qint64& bytesWritten)
{
    // Must be locked
    bytesWritten = 0;
    while (maxSize > 0) {
        int result = ::write(d->portHandle, data, maxSize);
        if (result == -1) {
            if (errno == EAGAIN)
                return true; // Need to wait
            setErrorString(tr("Cannot write to port %1: %2 (POSIX error %3)").
                           arg(portName, QString::fromLocal8Bit(strerror(errno))).arg(errno));

            bytesWritten = -1;
            return false;
        } else {
            if (result == 0)
                qWarning("%s: Zero bytes written to port %s!", Q_FUNC_INFO, qPrintable(portName));
            bytesWritten += result;
            maxSize -= result;
            data += result;
        }
    }
    return false; // If we reach here we've successfully written all the data without blocking
}

/* Returns true if EAGAIN encountered. Emits (or queues) bytesWritten for any buffers written.
 * If stopAfterWritingOneBuffer is true, return immediately if a single buffer is written, rather than
 * attempting to drain the whole queue.
 * Doesn't modify notifier.
 */
bool VirtualSerialDevice::tryFlushPendingBuffers(QMutexLocker& locker, FlushPendingOptions flags)
{
    while (pendingWrites.count() > 0) {
        // Try writing everything we've got, until we hit EAGAIN
        const QByteArray& data = pendingWrites[0];
        qint64 bytesWritten;
        bool needToWait = tryWrite(data.constData(), data.size(), bytesWritten);
        if (needToWait) {
            if (bytesWritten > 0) {
                // We wrote some of the data, update the pending queue
                QByteArray remainder = data.mid(bytesWritten);
                pendingWrites.removeFirst();
                pendingWrites.insert(0, remainder);
            }
            return needToWait;
        } else {
            pendingWrites.removeFirst();
            if (flags & EmitBytesWrittenAsync) {
                emit AsyncCall_emitBytesWrittenIfNeeded(bytesWritten);
            } else {
                emitBytesWrittenIfNeeded(locker, bytesWritten);
            }
            if (flags & StopAfterWritingOneBuffer) return false;
            // Otherwise go round loop again
        }
    }
    return false; // no EAGAIN encountered
}

void VirtualSerialDevice::writeHasUnblocked(int fileHandle)
{
    Q_ASSERT(fileHandle == d->portHandle);
    (void)fileHandle; // Compiler shutter-upper
    d->writeUnblockedNotifier->setEnabled(false);

    QMutexLocker locker(&lock);
    bool needToWait = tryFlushPendingBuffers(locker);
    if (needToWait) d->writeUnblockedNotifier->setEnabled(true);
}

// Copy of qt_safe_select from /qt/src/corelib/kernel/qeventdispatcher_unix.cpp
// But without the timeout correction
int safe_select(int nfds, fd_set *fdread, fd_set *fdwrite, fd_set *fdexcept,
                   const struct timeval *orig_timeout)
{
    if (!orig_timeout) {
        // no timeout -> block forever
        register int ret;
        do {
            ret = select(nfds, fdread, fdwrite, fdexcept, 0);
        } while (ret == -1 && errno == EINTR);
        return ret;
    }

    timeval timeout = *orig_timeout;

    int ret;
    forever {
        ret = ::select(nfds, fdread, fdwrite, fdexcept, &timeout);
        if (ret != -1 || errno != EINTR)
            return ret;
    }
}

bool VirtualSerialDevice::waitForBytesWritten(int msecs)
{
    QMutexLocker locker(&lock);
    if (pendingWrites.count() == 0) return false;

    if (QThread::currentThread() != thread()) {
        // Wait for signal from main thread
        unsigned long timeout = msecs;
        if (msecs == -1) timeout = ULONG_MAX;
        if (waiterForBytesWritten == NULL)
            waiterForBytesWritten = new QWaitCondition;
        return waiterForBytesWritten->wait(&lock, timeout);
    }

    d->writeUnblockedNotifier->setEnabled(false);
    forever {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(d->portHandle, &writeSet);

        struct timeval timeout;
        if (msecs != -1) {
            timeout.tv_sec = msecs / 1000;
            timeout.tv_usec = (msecs % 1000) * 1000;
        }
        int ret = safe_select(d->portHandle+1, NULL, &writeSet, NULL, msecs == -1 ? NULL : &timeout);

        if (ret == 0) {
            // Timeout
            return false;
        } else if (ret < 0) {
            setErrorString(tr("The function select() returned an error on port %1: %2 (POSIX error %3)").
                           arg(portName, QString::fromLocal8Bit(strerror(errno))).arg(errno));
            return false;
        } else {
            bool needToWait = tryFlushPendingBuffers(locker, StopAfterWritingOneBuffer);
            if (needToWait) {
                // go round the select again
            } else {
                return true;
            }
        }
    }
}

void VirtualSerialDevice::flush()
{
    while (waitForBytesWritten(-1)) { /* loop */ }
    tcflush(d->portHandle, TCIOFLUSH);
}

bool VirtualSerialDevice::waitForReadyRead(int msecs)
{
    return QIODevice::waitForReadyRead(msecs); //TODO
}

} // namespace SymbianUtils
