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

#include "virtualserialdevice.h"
#include <windows.h>
#if QT_VERSION >= 0x050000
#  include <QWinEventNotifier>
#else
#  include <private/qwineventnotifier_p.h>
#endif
#include <QThread>
#include <QWaitCondition>

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
};

void VirtualSerialDevice::platInit()
{
    d = new VirtualSerialDevicePrivate;
    d->portHandle = INVALID_HANDLE_VALUE;
    d->writeCompleteNotifier = NULL;
    memset(&d->writeOverlapped, 0, sizeof(OVERLAPPED));
    d->commEventNotifier = NULL;
    memset(&d->commEventOverlapped, 0, sizeof(OVERLAPPED));
}

QString windowsPortName(const QString& port)
{
    // Add the \\.\ to the name if it's a COM port and doesn't already have it
    QString winPortName(port);
    if (winPortName.startsWith(QLatin1String("COM"))) {
        winPortName.prepend("\\\\.\\");
    }
    return winPortName;
}

// Copied from \creator\src\libs\utils\winutils.cpp
QString winErrorMessage(unsigned long error)
{
    // Some of the windows error messages are a bit too obscure
    switch (error)
        {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_NOT_FOUND:
        return VirtualSerialDevice::tr("Port not found");
        break;
    case ERROR_ACCESS_DENIED:
        return VirtualSerialDevice::tr("Port in use");
    case ERROR_SEM_TIMEOUT: // Bluetooth ports sometimes return this
        return VirtualSerialDevice::tr("Timed out");
    case ERROR_NETWORK_UNREACHABLE:
        return VirtualSerialDevice::tr("Port unreachable"); // I don't know what this error indicates... from observation, that the windows Bluetooth stack has got itself into a state and needs resetting
    default:
        break;
        }

    QString rc = QString::fromLatin1("#%1: ").arg(error);
    ushort *lpMsgBuf;

    const int len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (len) {
        rc = QString::fromUtf16(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
    } else {
        rc += QString::fromLatin1("<unknown error>");
    }
    return rc.trimmed();
}

bool VirtualSerialDevice::open(OpenMode mode)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (isOpen()) return true;

    d->portHandle = CreateFileA(windowsPortName(portName).toAscii(), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (d->portHandle == INVALID_HANDLE_VALUE) {
        setErrorString(tr("The port %1 could not be opened: %2").
                       arg(portName, winErrorMessage(GetLastError())));
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
        qWarning("%s setting comm state", qPrintable(winErrorMessage(GetLastError())));
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
        qWarning("%s: Could not set comm mask, err=%d", Q_FUNC_INFO, (int)GetLastError());
    }
    bool result = WaitCommEvent(d->portHandle, &d->commEventMask, &d->commEventOverlapped);
    Q_ASSERT(result == false); // Can't see how it would make sense to be anything else...
    (void)result; // For release build
    if (GetLastError() != ERROR_IO_PENDING) {
        setErrorString(tr("An error occurred while waiting for read notifications from %1: %2").
                       arg(portName, winErrorMessage(GetLastError())));
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
    setErrorString(tr("An error occurred while reading from %1: %2").
                   arg(portName, winErrorMessage(GetLastError())));
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
        setErrorString(tr("An error occurred while writing to %1: %2").
                       arg(portName, winErrorMessage(GetLastError())));
        pendingWrites.removeFirst();
        return -1;
    }
}

void VirtualSerialDevice::writeCompleted()
{
    QMutexLocker locker(&lock);
    if (pendingWrites.count() == 0) {
        qWarning("%s: writeCompleted called when there are no pending writes on %s!",
                 Q_FUNC_INFO, qPrintable(portName));
        return;
    }

    doWriteCompleted(locker);
}

void VirtualSerialDevice::doWriteCompleted(QMutexLocker &locker)
{
    // Must be locked on entry
    ResetEvent(d->writeOverlapped.hEvent);

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

    if (QThread::currentThread() != thread()) {
        // Wait for signal from main thread
        unsigned long timeout = msecs;
        if (msecs == -1) timeout = ULONG_MAX;
        if (waiterForBytesWritten == NULL)
            waiterForBytesWritten = new QWaitCondition;
        return waiterForBytesWritten->wait(&lock, timeout);
    }

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
            setErrorString(tr("An error occurred while syncing on waitForBytesWritten for %1: %2").
                           arg(portName, winErrorMessage(GetLastError())));
            return false;
        }
        Q_ASSERT(bytesWritten == (DWORD)pendingWrites.first().length());

        doWriteCompleted(locker);
        return true;
    }
    else {
        setErrorString(QString("An error occurred in waitForBytesWritten() for %1: %2").
                       arg(portName, winErrorMessage(GetLastError())));
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
