/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "trkolddevice.h"
#include "trkutils.h"

#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtCore/QQueue>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <QtCore/QFile>

#  include <stdio.h>
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <errno.h>
#  include <string.h>
#  include <unistd.h>
#endif

enum { TimerInterval = 100 };

#ifdef Q_OS_WIN

// Format windows error from GetLastError() value: TODO: Use the one provided by the utisl lib.
static QString winErrorMessage(unsigned long error)
{
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
    return rc;
}

// Non-blocking replacement for win-api ReadFile function
static BOOL WINAPI TryReadFile(HANDLE          hFile,
                        LPVOID          lpBuffer,
                        DWORD           nNumberOfBytesToRead,
                        LPDWORD         lpNumberOfBytesRead,
                        LPOVERLAPPED    lpOverlapped)
{
    COMSTAT comStat;
    if (!ClearCommError(hFile, NULL, &comStat)){
        qDebug()<<("ClearCommError() failed");
        return FALSE;
    }
    if (comStat.cbInQue == 0) {
        *lpNumberOfBytesRead = 0;
        return FALSE;
    }
    return ReadFile(hFile,
                    lpBuffer,
                    qMin(comStat.cbInQue, nNumberOfBytesToRead),
                    lpNumberOfBytesRead,
                    lpOverlapped);
}
#endif

namespace trkold {

struct TrkDevicePrivate {
    TrkDevicePrivate();
#ifdef Q_OS_WIN
    HANDLE hdevice;
#else
    QFile file;
#endif

    QByteArray trkReadBuffer;
    bool trkWriteBusy;
    int timerId;
    bool serialFrame;
    bool verbose;
    QString errorString;
};

TrkDevicePrivate::TrkDevicePrivate() :
#ifdef Q_OS_WIN
    hdevice(INVALID_HANDLE_VALUE),
#endif
    trkWriteBusy(false),
    timerId(-1),
    serialFrame(true),
    verbose(false)
{

}

TrkDevice::TrkDevice(QObject *parent) :
    QObject(parent),
    d(new TrkDevicePrivate)
{
}

bool TrkDevice::open(const QString &port, QString *errorMessage)
{
    close();
#ifdef Q_OS_WIN
    d->hdevice = CreateFile(port.toStdWString().c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (INVALID_HANDLE_VALUE == d->hdevice) {
        *errorMessage = QString::fromLatin1("Could not open device '%1': %2").arg(port, winErrorMessage(GetLastError()));
        return false;
    }
    d->timerId = startTimer(TimerInterval);
    return true;
#else
    d->file.setFileName(port);
    if (!d->file.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(port, d->file.errorString());
        return false;
    }

    struct termios termInfo;
    if (tcgetattr(d->file.handle(), &termInfo) < 0) {
        *errorMessage = QString::fromLatin1("Unable to retrieve terminal settings: %1 %2").arg(errno).arg(QString::fromAscii(strerror(errno)));
        return false;
    }
    // Turn off terminal echo as not get messages back, among other things
    termInfo.c_cflag|=CREAD|CLOCAL;
    termInfo.c_lflag&=(~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ISIG));
    termInfo.c_iflag&=(~(INPCK|IGNPAR|PARMRK|ISTRIP|ICRNL|IXANY));
    termInfo.c_oflag&=(~OPOST);
    termInfo.c_cc[VMIN]=0;
    termInfo.c_cc[VINTR] = _POSIX_VDISABLE;
    termInfo.c_cc[VQUIT] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTART] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTOP] = _POSIX_VDISABLE;
    termInfo.c_cc[VSUSP] = _POSIX_VDISABLE;
    if (tcsetattr(d->file.handle(), TCSAFLUSH, &termInfo) < 0) {
        *errorMessage = QString::fromLatin1("Unable to apply terminal settings: %1 %2").arg(errno).arg(QString::fromAscii(strerror(errno)));
        return false;
    }
    d->timerId = startTimer(TimerInterval);
    return true;
#endif
}


TrkDevice::~TrkDevice()
{
    close();
    delete d;
}

void TrkDevice::close()
{
    if (!isOpen())
        return;
    if (d->timerId != -1) {
        killTimer(d->timerId);
        d->timerId = -1;
    }
#ifdef Q_OS_WIN
    CloseHandle(d->hdevice);
    d->hdevice = INVALID_HANDLE_VALUE;
#else
    d->file.close();
#endif
    if (verbose())
        logMessage("Close");
}

bool TrkDevice::isOpen() const
{
#ifdef Q_OS_WIN
    return d->hdevice != INVALID_HANDLE_VALUE;
#else
    return d->file.isOpen();
#endif
}

QString TrkDevice::errorString() const
{
    return d->errorString;
}

bool TrkDevice::serialFrame() const
{
    return d->serialFrame;
}

void TrkDevice::setSerialFrame(bool f)
{
    d->serialFrame = f;
}

bool TrkDevice::verbose() const
{
    return true || d->verbose;
}

void TrkDevice::setVerbose(bool b)
{
    d->verbose = b;
}

bool TrkDevice::write(const QByteArray &data, QString *errorMessage)
{
    if (verbose())
        logMessage("XWRITE " + data.toHex());
#ifdef Q_OS_WIN
    DWORD charsWritten;
    if (!WriteFile(d->hdevice, data.data(), data.size(), &charsWritten, NULL)) {
        *errorMessage = QString::fromLatin1("Error writing data: %1").arg(winErrorMessage(GetLastError()));
        return false;
    }
    FlushFileBuffers(d->hdevice);
    return true;
#else
    if (d->file.write(data) == -1 || !d->file.flush()) {
        *errorMessage = QString::fromLatin1("Cannot write: %1").arg(d->file.errorString());
        return false;
    }
    return  true;
#endif
}

#ifndef Q_OS_WIN
static inline int bytesAvailable(int fileNo)
{
    int numBytes;
    const int rc = ioctl(fileNo, FIONREAD, &numBytes);
    if (rc < 0)
        numBytes=0;
    return numBytes;
}
#endif

void TrkDevice::tryTrkRead()
{
#ifdef Q_OS_WIN
    const DWORD BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    DWORD charsRead;
    DWORD totalCharsRead = 0;

    while (TryReadFile(d->hdevice, buffer, BUFFERSIZE, &charsRead, NULL)) {
        totalCharsRead += charsRead;
        d->trkReadBuffer.append(buffer, charsRead);
        if (isValidTrkResult(d->trkReadBuffer, d->serialFrame))
            break;
    }
    if (verbose() && totalCharsRead)
        logMessage("Read" + d->trkReadBuffer.toHex());
    if (!totalCharsRead)
        return;
    const ushort len = trk::isValidTrkResult(d->trkReadBuffer, d->serialFrame);
    if (!len) {
        const QString msg = QString::fromLatin1("Partial message: %1").arg(stringFromArray(d->trkReadBuffer));
        emitError(msg);
        return;
    }
#else
    const int size = bytesAvailable(d->file.handle());
    if (!size)
        return;
    const QByteArray data = d->file.read(size);
    if (verbose())
        logMessage("READ " + data.toHex());
    d->trkReadBuffer.append(data);
    const ushort len = trk::isValidTrkResult(d->trkReadBuffer, d->serialFrame);
    if (!len) {
        if (d->trkReadBuffer.size() > 10) {
            const QString msg = QString::fromLatin1("Unable to extract message from '%1' '%2'").
                             arg(QLatin1String(d->trkReadBuffer.toHex())).arg(QString::fromAscii(d->trkReadBuffer));
            emitError(msg);
        }
        return;
    }
#endif // Q_OS_WIN
    trk::TrkResult r;
    QByteArray rawData;
    while (extractResult(&d->trkReadBuffer, d->serialFrame, &r, &rawData)) {
        if (verbose())
            logMessage("Read TrkResult " + r.data.toHex());
        emit messageReceived(r);
        if (!rawData.isEmpty())
            emit rawDataReceived(rawData);
    }
}

void TrkDevice::timerEvent(QTimerEvent *)
{
    tryTrkRead();
}

void TrkDevice::emitError(const QString &s)
{
    d->errorString = s;
    qWarning("%s\n", qPrintable(s));
    emit error(s);
}

/* A message to be send to TRK, triggering a callback on receipt
 * of the answer. */
typedef trk::Callback<const trk::TrkResult &> TrkCallback;
struct TrkMessage {
    explicit TrkMessage(unsigned char code = 0u,
                        unsigned char token = 0u,
                        TrkCallback callback = TrkCallback());

    unsigned char code;
    unsigned char token;
    QByteArray data;
    QVariant cookie;
    TrkCallback callback;
    bool invokeOnNAK;
};

TrkMessage::TrkMessage(unsigned char c,
                        unsigned char t,
                        TrkCallback cb) :
    code(c),
    token(t),
    callback(cb),
    invokeOnNAK(false)
{
}

// ------- TrkWriteQueueDevice
typedef QSharedPointer<TrkMessage> SharedPointerTrkMessage;

/* Mixin class that manages a write queue of Trk messages. */

class TrkWriteQueue {
public:
    TrkWriteQueue();

    // Enqueue messages.
    void queueTrkMessage(unsigned char code, TrkCallback callback,
                        const QByteArray &data, const QVariant &cookie,
                        bool invokeOnNAK);
    void queueTrkInitialPing();

    // Call this from the device read notification with the results.
    void slotHandleResult(const trk::TrkResult &result);

    // This can be called periodically in a timer to retrieve
    // the pending messages to be sent.
    bool pendingMessage(SharedPointerTrkMessage *message = 0);
    // Notify the queue about the success of the write operation
    // after taking the pendingMessage off.
    void notifyWriteResult(bool ok);

    // Factory function for ack message
    static SharedPointerTrkMessage trkAck(unsigned char token);

private:
    typedef QMap<unsigned char, SharedPointerTrkMessage> TokenMessageMap;

    unsigned char nextTrkWriteToken();

    unsigned char trkWriteToken;
    QQueue<SharedPointerTrkMessage> trkWriteQueue;
    TokenMessageMap writtenTrkMessages;
    bool trkWriteBusy;
};

TrkWriteQueue::TrkWriteQueue() :
    trkWriteToken(0),
    trkWriteBusy(false)
{
}

unsigned char TrkWriteQueue::nextTrkWriteToken()
{
    ++trkWriteToken;
    if (trkWriteToken == 0)
        ++trkWriteToken;
    return trkWriteToken;
}

void TrkWriteQueue::queueTrkMessage(unsigned char code, TrkCallback callback,
                                         const QByteArray &data, const QVariant &cookie,
                                         bool invokeOnNAK)
{
    const unsigned char token = code == TRK_WRITE_QUEUE_NOOP_CODE ?
                                (unsigned char)(0) : nextTrkWriteToken();
    SharedPointerTrkMessage msg(new TrkMessage(code, token, callback));
    msg->data = data;
    msg->cookie = cookie;
    msg->invokeOnNAK = invokeOnNAK;
    trkWriteQueue.append(msg);
}

bool TrkWriteQueue::pendingMessage(SharedPointerTrkMessage *message)
{
    // Invoked from timer, try to flush out message queue
    if (trkWriteBusy || trkWriteQueue.isEmpty())
        return false;
    // Handle the noop message, just invoke CB
    if (trkWriteQueue.front()->code == TRK_WRITE_QUEUE_NOOP_CODE) {
        const SharedPointerTrkMessage noopMessage = trkWriteQueue.dequeue();
        if (noopMessage->callback) {
            trk::TrkResult result;
            result.code = noopMessage->code;
            result.token = noopMessage->token;
            result.data = noopMessage->data;
            result.cookie = noopMessage->cookie;
            noopMessage->callback(result);
        }
    }
    // Check again for real messages
    if (trkWriteQueue.isEmpty())
        return false;
    if (message)
        *message = trkWriteQueue.front();
    return true;
}

void TrkWriteQueue::notifyWriteResult(bool ok)
{
    // On success, dequeue message and await result
    if (ok) {
        const SharedPointerTrkMessage firstMsg = trkWriteQueue.dequeue();
        writtenTrkMessages.insert(firstMsg->token, firstMsg);
        trkWriteBusy = true;
    }
}

void TrkWriteQueue::slotHandleResult(const trk::TrkResult &result)
{
    trkWriteBusy = false;
    if (result.code != trk::TrkNotifyAck && result.code != trk::TrkNotifyNak)
        return;
    // Find which request the message belongs to and invoke callback
    // if ACK or on NAK if desired.
    const TokenMessageMap::iterator it = writtenTrkMessages.find(result.token);
    if (it == writtenTrkMessages.end())
        return;
    const bool invokeCB = it.value()->callback
                          && (result.code == trk::TrkNotifyAck || it.value()->invokeOnNAK);

    if (invokeCB) {
        trk::TrkResult result1 = result;
        result1.cookie = it.value()->cookie;
        it.value()->callback(result1);
    }
    writtenTrkMessages.erase(it);
}

SharedPointerTrkMessage TrkWriteQueue::trkAck(unsigned char token)
{
    SharedPointerTrkMessage msg(new TrkMessage(0x80, token));
    msg->token = token;
    msg->data.append('\0');
    return msg;
}

void TrkWriteQueue::queueTrkInitialPing()
{
    const SharedPointerTrkMessage msg(new TrkMessage(0, 0)); // Ping, reset sequence count
    trkWriteQueue.append(msg);
}

// -----------------------
TrkWriteQueueDevice::TrkWriteQueueDevice(QObject *parent) :
    TrkDevice(parent),
    qd(new TrkWriteQueue)
{
    connect(this, SIGNAL(messageReceived(trk::TrkResult)), this, SLOT(slotHandleResult(trk::TrkResult)));
}

TrkWriteQueueDevice::~TrkWriteQueueDevice()
{
    delete qd;
}

void TrkWriteQueueDevice::sendTrkMessage(unsigned char code, TrkCallback callback,
                                         const QByteArray &data, const QVariant &cookie,
                                         bool invokeOnNAK)
{
    qd->queueTrkMessage(code, callback, data, cookie, invokeOnNAK);
}

void TrkWriteQueueDevice::sendTrkInitialPing()
{
    qd->queueTrkInitialPing();
}

bool TrkWriteQueueDevice::sendTrkAck(unsigned char token)
{
    // The acknowledgement must not be queued!
    const SharedPointerTrkMessage ack = TrkWriteQueue::trkAck(token);
    return trkWriteRawMessage(*ack);
    // 01 90 00 07 7e 80 01 00 7d 5e 7e
}

void TrkWriteQueueDevice::tryTrkWrite()
{
    if (!qd->pendingMessage())
        return;
    SharedPointerTrkMessage message;
    qd->pendingMessage(&message);
    const bool success = trkWriteRawMessage(*message);
    qd->notifyWriteResult(success);
}

bool TrkWriteQueueDevice::trkWriteRawMessage(const TrkMessage &msg)
{
    const QByteArray ba = trk::frameMessage(msg.code, msg.token, msg.data, serialFrame());
    if (verbose())
        logMessage("WRITE: " + trk::stringFromArray(ba));
    QString errorMessage;
    const bool rc = write(ba, &errorMessage);
    if (!rc)
        emitError(errorMessage);
    return rc;
}

void TrkWriteQueueDevice::timerEvent(QTimerEvent *ev)
{
    tryTrkWrite();
    TrkDevice::timerEvent(ev);
}

void TrkWriteQueueDevice::slotHandleResult(const trk::TrkResult &result)
{
    qd->slotHandleResult(result);
}

// ----------- TrkWriteQueueDevice

struct TrkWriteQueueIODevicePrivate {
    TrkWriteQueueIODevicePrivate(const QSharedPointer<QIODevice> &d);

    const QSharedPointer<QIODevice> device;
    TrkWriteQueue queue;
    QByteArray readBuffer;
    bool serialFrame;
    bool verbose;
};

TrkWriteQueueIODevicePrivate::TrkWriteQueueIODevicePrivate(const QSharedPointer<QIODevice> &d) :
    device(d),
    serialFrame(true),
    verbose(false)
{
}

TrkWriteQueueIODevice::TrkWriteQueueIODevice(const QSharedPointer<QIODevice> &device,
                                             QObject *parent) :
    QObject(parent),
    d(new TrkWriteQueueIODevicePrivate(device))
{
    startTimer(TimerInterval);
}

TrkWriteQueueIODevice::~TrkWriteQueueIODevice()
{
    delete d;
}

bool TrkWriteQueueIODevice::serialFrame() const
{
    return d->serialFrame;
}

void TrkWriteQueueIODevice::setSerialFrame(bool f)
{
    d->serialFrame = f;
}

bool TrkWriteQueueIODevice::verbose() const
{
    return true || d->verbose;
}

void TrkWriteQueueIODevice::setVerbose(bool b)
{
    d->verbose = b;
}

void TrkWriteQueueIODevice::sendTrkMessage(unsigned char code, TrkCallback callback,
                                         const QByteArray &data, const QVariant &cookie,
                                         bool invokeOnNAK)
{
    d->queue.queueTrkMessage(code, callback, data, cookie, invokeOnNAK);
}

void TrkWriteQueueIODevice::sendTrkInitialPing()
{
    d->queue.queueTrkInitialPing();
}

bool TrkWriteQueueIODevice::sendTrkAck(unsigned char token)
{
    // The acknowledgement must not be queued!
    const SharedPointerTrkMessage ack = TrkWriteQueue::trkAck(token);
    return trkWriteRawMessage(*ack);
    // 01 90 00 07 7e 80 01 00 7d 5e 7e
}


void TrkWriteQueueIODevice::timerEvent(QTimerEvent *)
{
    tryTrkWrite();
    tryTrkRead();
}

void TrkWriteQueueIODevice::tryTrkWrite()
{
    if (!d->queue.pendingMessage())
        return;
    SharedPointerTrkMessage message;
    d->queue.pendingMessage(&message);
    const bool success = trkWriteRawMessage(*message);
    d->queue.notifyWriteResult(success);
}

bool TrkWriteQueueIODevice::trkWriteRawMessage(const TrkMessage &msg)
{
    const QByteArray ba = trk::frameMessage(msg.code, msg.token, msg.data, serialFrame());
    if (verbose())
        logMessage("WRITE: " + trk::stringFromArray(ba));
    const bool ok = d->device->write(ba) != -1;
    if (!ok) {
        const QString msg = QString::fromLatin1("Unable to write %1 bytes: %2:").arg(ba.size()).arg(d->device->errorString());
        qWarning("%s\n", qPrintable(msg));
    }
    return ok;
}

void TrkWriteQueueIODevice::tryTrkRead()
{
    const  quint64 bytesAvailable = d->device->bytesAvailable();
    if (!bytesAvailable)
        return;
    const QByteArray newData = d->device->read(bytesAvailable);
    //if (verbose())
        logMessage("READ " + newData.toHex());
    d->readBuffer.append(newData);
    trk::TrkResult r;
    QByteArray rawData;
    while (trk::extractResult(&(d->readBuffer), d->serialFrame, &r, &rawData)) {
        d->queue.slotHandleResult(r);
        emit messageReceived(r);
        if (!rawData.isEmpty())
            emit rawDataReceived(rawData);
    }
}

} // namespace trkold
