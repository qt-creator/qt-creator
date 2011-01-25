#include "virtualserialdevice.h"
#include <windows.h>
#include <QtCore/private/qwineventnotifier_p.h>
#include <QtCore/QThread>

namespace SymbianUtils {

class VirtualSerialDevicePrivate
{
public:
    HANDLE portHandle;
    OVERLAPPED writeOverlapped;
    OVERLAPPED commEventOverlapped;
    DWORD commEventMask;
    QWinEventNotifier *writeCompleteNotifier;
    QWinEventNotifier *commEventNotifier;
    int bytesWrittenSignalsAlreadyEmitted;
};

void VirtualSerialDevice::platInit()
{
    d = new VirtualSerialDevicePrivate;
    d->portHandle = INVALID_HANDLE_VALUE;
    d->writeCompleteNotifier = NULL;
    memset(&d->writeOverlapped, 0, sizeof(OVERLAPPED));
    d->commEventNotifier = NULL;
    memset(&d->commEventOverlapped, 0, sizeof(OVERLAPPED));
    d->bytesWrittenSignalsAlreadyEmitted = 0;
}

bool VirtualSerialDevice::open(OpenMode mode)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (isOpen()) return true;

    d->portHandle = CreateFileA(portName.toAscii(), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (d->portHandle == INVALID_HANDLE_VALUE) {
        setErrorString(QString("Windows error %1 opening %2").arg(GetLastError()).arg(portName));
        return false;
    }

    DCB commState;
    memset(&commState, 0, sizeof(DCB));
    commState.DCBlength = sizeof(DCB);
    bool ok = GetCommState(d->portHandle, &commState);
    if (ok) {
        commState.BaudRate = CBR_115200;
        commState.fBinary = TRUE;
        commState.fParity = FALSE;
        commState.fOutxCtsFlow = FALSE;
        commState.fOutxDsrFlow = FALSE;
        commState.fInX = FALSE;
        commState.fOutX = FALSE;
        commState.fNull = FALSE;
        commState.fAbortOnError = FALSE;
        commState.fDsrSensitivity = FALSE;
        commState.fDtrControl = DTR_CONTROL_DISABLE;
        commState.ByteSize = 8;
        commState.Parity = NOPARITY;
        commState.StopBits = ONESTOPBIT;
        ok = SetCommState(d->portHandle, &commState);
        }
    if (!ok) {
        qWarning("Windows error %d setting comm state", (int)GetLastError());
        }

    // http://msdn.microsoft.com/en-us/library/aa363190(v=vs.85).aspx says this means
    // "the read operation is to return immediately with the bytes that have already been received, even if no bytes have been received"
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(d->portHandle, &timeouts);

    d->writeOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    d->writeCompleteNotifier = new QWinEventNotifier(d->writeOverlapped.hEvent, this);
    connect(d->writeCompleteNotifier, SIGNAL(activated(HANDLE)), this, SLOT(writeCompleted()));

    // This is how we implement readyRead notifications
    d->commEventOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    d->commEventNotifier = new QWinEventNotifier(d->commEventOverlapped.hEvent, this);
    connect(d->commEventNotifier, SIGNAL(activated(HANDLE)), this, SLOT(commEventOccurred()));

    if (!SetCommMask(d->portHandle, EV_RXCHAR)) {
        // What to do?
        qWarning("Couldn't set comm mask, err=%d", (int)GetLastError());
    }
    bool result = WaitCommEvent(d->portHandle, &d->commEventMask, &d->commEventOverlapped);
    Q_ASSERT(result == false); // Can't see how it would make sense to be anything else...
    (void)result; // For release build
    if (GetLastError() != ERROR_IO_PENDING) {
        setErrorString(QString("Windows error %1 waiting for read notifications from %2").arg(GetLastError()).arg(portName));
        close();
        return false;
    }

    ok = QIODevice::open(mode);
    if (!ok) close();
    return ok;
}

void VirtualSerialDevice::platClose()
{
    delete d->writeCompleteNotifier;
    d->writeCompleteNotifier = NULL;
    CloseHandle(d->writeOverlapped.hEvent);
    d->writeOverlapped.hEvent = INVALID_HANDLE_VALUE;

    delete d->commEventNotifier;
    d->commEventNotifier = NULL;
    d->commEventOverlapped.hEvent = INVALID_HANDLE_VALUE;

    CloseHandle(d->portHandle);
    d->portHandle = INVALID_HANDLE_VALUE;
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

    qint64 avail = 0;
    COMSTAT Status;
    if (ClearCommError(d->portHandle, NULL, &Status)) {
        avail = Status.cbInQue;
    }
    return avail + QIODevice::bytesAvailable();
}

void VirtualSerialDevice::commEventOccurred()
{
    DWORD event = d->commEventMask;
    if (event & EV_RXCHAR) {
        emit readyRead();
    }
    ResetEvent(d->commEventOverlapped.hEvent);
    WaitCommEvent(d->portHandle, &d->commEventMask, &d->commEventOverlapped);
}

qint64 VirtualSerialDevice::readData(char *data, qint64 maxSize)
{
    QMutexLocker locker(&lock);
    // We do our reads synchronously
    OVERLAPPED readOverlapped;
    memset(&readOverlapped, 0, sizeof(OVERLAPPED));
    DWORD bytesRead;
    BOOL done = ReadFile(d->portHandle, data, maxSize, &bytesRead, &readOverlapped);
    if (done) return (qint64)bytesRead;

    if (GetLastError() == ERROR_IO_PENDING) {
        // Note the TRUE to wait for the read to complete
        done = GetOverlappedResult(d->portHandle, &readOverlapped, &bytesRead, TRUE);
        if (done) return (qint64)bytesRead;
    }

    // If we reach here an error has occurred
    setErrorString(QString("Windows error %1 reading from %2").arg(GetLastError()).arg(portName));
    return -1;
}


qint64 VirtualSerialDevice::writeData(const char *data, qint64 maxSize)
{
    QMutexLocker locker(&lock);

    pendingWrites.append(QByteArray(data, maxSize)); // Can't see a way of doing async io safely without having to copy here...
    if (pendingWrites.count() == 1) {
        return writeNextBuffer(locker);
    } else {
        return maxSize;
    }
}

qint64 VirtualSerialDevice::writeNextBuffer(QMutexLocker& locker)
{
    Q_UNUSED(locker)
    // Must be locked on entry
    qint64 bufLen = pendingWrites[0].length();
    BOOL ok = WriteFile(d->portHandle, pendingWrites[0].constData(), bufLen, NULL, &d->writeOverlapped);
    if (ok || GetLastError() == ERROR_IO_PENDING) {
        // Apparently it can return true for a small asynchronous write...
        // Hopefully it still gets signalled in the same way!

        // Wait for signal via writeCompleted
        return bufLen;
    }
    else {
        setErrorString(QString("Windows error %1 writing to %2").arg(GetLastError()).arg(portName));
        pendingWrites.removeFirst();
        return -1;
    }
}

void VirtualSerialDevice::writeCompleted()
{
    ResetEvent(d->writeOverlapped.hEvent);

    QMutexLocker locker(&lock);

    if (d->bytesWrittenSignalsAlreadyEmitted > 0) {
        // The signal has already been emitted due to waitForBytesWritten being called, so don't do it here
        d->bytesWrittenSignalsAlreadyEmitted--;
        return;
    }

    if (pendingWrites.count() == 0) {
        qWarning("writeCompleted called when there are no pending writes!");
        return;
    }
    //Q_ASSERT(pendingWrites.count());
    qint64 len = pendingWrites.first().length();
    pendingWrites.removeFirst();

    if (pendingWrites.count() > 0) {
        // Get the next write started before notifying in case client calls waitForBytesWritten in their slot
        writeNextBuffer(locker);
    }

    emitBytesWrittenIfNeeded(locker, len);
}

bool VirtualSerialDevice::waitForBytesWritten(int msecs)
{
    QMutexLocker locker(&lock);
    if (pendingWrites.count() == 0) return false;

    DWORD waitTime = msecs;
    if (msecs == -1) waitTime = INFINITE; // Ok these are probably bitwise the same, but just to prove I've thought about it...
    DWORD result = WaitForSingleObject(d->writeOverlapped.hEvent, waitTime); // Do I need WaitForSingleObjectEx and worry about alertable states?
    if (result == WAIT_TIMEOUT) {
        return false;
    }
    else if (result == WAIT_OBJECT_0) {
        DWORD bytesWritten;
        BOOL ok = GetOverlappedResult(d->portHandle, &d->writeOverlapped, &bytesWritten, TRUE);
        if (!ok) {
            setErrorString(QString("Windows error %1 syncing on waitForBytesWritten for %2").arg(GetLastError()).arg(portName));
            return false;
        }
        Q_ASSERT(bytesWritten == (DWORD)pendingWrites.first().length());

        //TODO should we call writeNextBuffer() here? (Probably)

        /*if (thread() == QThread::currentThread()) {
            ResetEvent(writeHandle); // Make sure we don't cause a writeCompleted via the writeCompleteNotifier

        }
        else*/ {
            // We're waiting in another thread - make sure the main thread doesn't also emit bytesWritten
            d->bytesWrittenSignalsAlreadyEmitted++;
            qint64 len = pendingWrites.first().length();
            pendingWrites.removeFirst();
            emitBytesWrittenIfNeeded(locker, len);
            return true;
        }
    }
    else {
        setErrorString(QString("Windows error %1 in waitForBytesWritten for %2").arg(GetLastError()).arg(portName));
        return false;
    }
}

void VirtualSerialDevice::flush()
{
    while (waitForBytesWritten(-1)) { /* loop */ }
}

bool VirtualSerialDevice::waitForReadyRead(int msecs)
{
    return QIODevice::waitForReadyRead(msecs); //TODO
}

} // namespace SymbianUtils
