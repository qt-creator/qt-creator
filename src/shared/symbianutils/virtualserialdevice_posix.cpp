#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <QtCore/QSocketNotifier>
#include <QtCore/QTimer>
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
        setErrorString(QString("Posix error %1 opening %2").arg(errno).arg(portName));
        return false;
        }

    // Not totally sure this stuff is needed -tomsci
    /*
    struct termios termInfo;
    if (tcgetattr(d->portHandle, &termInfo) < 0) {
        setErrorString(QString::fromLatin1("Unable to retrieve terminal settings: %1 %2").arg(errno).arg(QString::fromAscii(strerror(errno))));
        close();
        return false;
    }
    cfmakeraw(&termInfo); //TOMSCI TESTING
    // Turn off terminal echo as not get messages back, among other things
    termInfo.c_cflag |= CREAD|CLOCAL;

    // BEGIN TESTING
    //termInfo.c_cflag&=(~CBAUD);
    //termInfo.c_cflag|=B115200;
    //termInfo.c_cc[VTIME] = 5;
    //END TESTING

    termInfo.c_lflag &= (~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ISIG));
    termInfo.c_iflag &= (~(INPCK|IGNPAR|PARMRK|ISTRIP|ICRNL|IXANY));
    termInfo.c_oflag &= (~OPOST);
    termInfo.c_cc[VMIN]  = 0;
    termInfo.c_cc[VINTR] = _POSIX_VDISABLE;
    termInfo.c_cc[VQUIT] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTART] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTOP] = _POSIX_VDISABLE;
    termInfo.c_cc[VSUSP] = _POSIX_VDISABLE;
    if (tcsetattr(d->portHandle, TCSAFLUSH, &termInfo) < 0) {
        setErrorString(QString::fromLatin1("Unable to apply terminal settings: %1 %2").arg(errno).arg(QString::fromAscii(strerror(errno))));
        close();
        return false;
    }
    //fcntl(d->portHandle, F_SETFL, O_SYNC); // TOMSCI TESTING
    */

    d->readNotifier = new QSocketNotifier(d->portHandle, QSocketNotifier::Read);
    connect(d->readNotifier, SIGNAL(activated(int)), this, SIGNAL(readyRead()));

    d->writeUnblockedNotifier = new QSocketNotifier(d->portHandle, QSocketNotifier::Write);
    d->writeUnblockedNotifier->setEnabled(false);
    connect(d->writeUnblockedNotifier, SIGNAL(activated(int)), this, SLOT(writeHasUnblocked(int)));

    bool ok = QIODevice::open(mode);
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
    if (ioctl(d->portHandle, FIONREAD, &avail) == -1) {
        return 0;
    }
    return (qint64)avail + QIODevice::bytesAvailable();
}

qint64 VirtualSerialDevice::readData(char *data, qint64 maxSize)
{
    QMutexLocker locker(&lock);
    int result = ::read(d->portHandle, data, maxSize);
    if (result == -1 && errno == EAGAIN) result = 0; // To Qt, 0 here means nothing ready right now, and -1 is reserved for permanent errors
    return result;
}

qint64 VirtualSerialDevice::writeData(const char *data, qint64 maxSize)
{
    QMutexLocker locker(&lock);
    qint64 bytesWritten;
    bool needToWait = tryFlushPendingBuffers(locker, EmitBytesWrittenAsync);
    if (!needToWait) {
        needToWait = tryWrite(data, maxSize, bytesWritten);
    }

    if (needToWait) {
        pendingWrites.append(QByteArray(data, maxSize));
        d->writeUnblockedNotifier->setEnabled(true);
        // Now wait for the writeUnblocked signal or for a call to waitForBytesWritten
        return maxSize;
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
    int result = ::write(d->portHandle, data, maxSize);
    if (result == -1) {
        if (errno == EAGAIN) {
            // Need to wait
            return true;
        } else {
            setErrorString(QString("Posix error %1 from write to %2").arg(errno).arg(portName));
            bytesWritten = -1;
            return false;
        }
    } else {
        Q_ASSERT(result == maxSize); // Otherwise I cry
        bytesWritten = maxSize;
        return false;
    }
}

/* Returns true if EAGAIN encountered. Emits (or queues) bytesWritten for any buffers written.
 * If stopAfterWritingOneBuffer is true, return immediately if a single buffer is written, rather than
 * attempting to drain the whole queue.
 * Doesn't modify notifier.
 */
bool VirtualSerialDevice::tryFlushPendingBuffers(QMutexLocker& locker, FlushPendingOptions flags) //bool stopAfterWritingOneBuffer)
{
    while (pendingWrites.count() > 0) {
        // Try writing everything we've got, until we hit EAGAIN
        const QByteArray& data = pendingWrites[0];
        qint64 bytesWritten;
        bool needToWait = tryWrite(data.constData(), data.size(), bytesWritten);
        if (needToWait) {
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
    //TODO this won't handle multithreading... need to disable writeCompleteNotifier in main thread context (or use bytesWrittenSignalsAlreadyEmitted)

    QMutexLocker locker(&lock);
    if (pendingWrites.count() == 0) return false;

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
            setErrorString(QString("Posix error %1 returned from select in waitForBytesWritten").arg(errno));
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
