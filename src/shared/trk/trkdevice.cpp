/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "trkdevice.h"
#include "trkutils.h"

#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtCore/QQueue>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QSharedPointer>
#include <QtCore/QMetaType>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <QtCore/QFile>

#  include <stdio.h>
#  include <sys/ioctl.h>
#  include <sys/types.h>
#  include <termios.h>
#  include <errno.h>
#  include <string.h>
#  include <unistd.h>
#endif

enum { TimerInterval = 10 };

#ifdef Q_OS_WIN

// Format windows error from GetLastError() value:
// TODO: Use the one provided by the utils lib.
QString winErrorMessage(unsigned long error)
{
    QString rc = QString::fromLatin1("#%1: ").arg(error);
    ushort *lpMsgBuf;

    const int len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS,
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
BOOL WINAPI TryReadFile(HANDLE          hFile,
                        LPVOID          lpBuffer,
                        DWORD           nNumberOfBytesToRead,
                        LPDWORD         lpNumberOfBytesRead,
                        LPOVERLAPPED    lpOverlapped)
{
    COMSTAT comStat;
    if (!ClearCommError(hFile, NULL, &comStat)){
        qDebug() << "ClearCommError() failed";
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

namespace trk {

///////////////////////////////////////////////////////////////////////
//
// TrkMessage
//
///////////////////////////////////////////////////////////////////////

/* A message to be send to TRK, triggering a callback on receipt
 * of the answer. */
struct TrkMessage
{
    explicit TrkMessage(byte code = 0u, byte token = 0u,
                        TrkCallback callback = TrkCallback());

    byte code;
    byte token;
    QByteArray data;
    QVariant cookie;
    TrkCallback callback;
};

TrkMessage::TrkMessage(byte c, byte t, TrkCallback cb) :
    code(c),
    token(t),
    callback(cb)
{
}

} // namespace trk

Q_DECLARE_METATYPE(trk::TrkMessage)

namespace trk {

///////////////////////////////////////////////////////////////////////
//
// TrkWriteQueue: Mixin class that manages a write queue of Trk messages.
// pendingMessage()/notifyWriteResult() should be called from a worked/timer
// that writes the messages. The class does not take precautions for multithreading
// with exception of the handling of the  TRK_WRITE_QUEUE_NOOP_CODE
// synchronization message. The invocation of the callback is then
// done by the thread owning the TrkWriteQueue, while pendingMessage() is called
// from another thread. This happens via a Qt::BlockingQueuedConnection.
//
///////////////////////////////////////////////////////////////////////

class TrkWriteQueue : public QObject
{    
    Q_OBJECT
    Q_DISABLE_COPY(TrkWriteQueue)
public:
    explicit TrkWriteQueue(bool multithreaded = true);

    // Enqueue messages.
    void queueTrkMessage(byte code, TrkCallback callback,
                        const QByteArray &data, const QVariant &cookie);
    void queueTrkInitialPing();

    // Call this from the device read notification with the results.
    void slotHandleResult(const TrkResult &result);

    // This can be called periodically in a timer to retrieve
    // the pending messages to be sent.
    bool pendingMessage(TrkMessage *message);
    // Notify the queue about the success of the write operation
    // after taking the pendingMessage off.
    void notifyWriteResult(bool ok);

signals:
    void internalNoopMessageDequeued(const trk::TrkMessage&);

private slots:
    void invokeNoopMessage(trk::TrkMessage);

private:
    typedef QMap<byte, TrkMessage> TokenMessageMap;

    byte nextTrkWriteToken();

    byte m_trkWriteToken;
    QQueue<TrkMessage> m_trkWriteQueue;
    TokenMessageMap m_writtenTrkMessages;
    bool m_trkWriteBusy;
};

TrkWriteQueue::TrkWriteQueue(bool multithreaded) :
    m_trkWriteToken(0),
    m_trkWriteBusy(false)
{
    static const int trkMessageMetaId = qRegisterMetaType<trk::TrkMessage>();
    Q_UNUSED(trkMessageMetaId)
    connect(this, SIGNAL(internalNoopMessageDequeued(trk::TrkMessage)),
            this, SLOT(invokeNoopMessage(trk::TrkMessage)),
            multithreaded ? Qt::BlockingQueuedConnection : Qt::AutoConnection);
}

byte TrkWriteQueue::nextTrkWriteToken()
{
    ++m_trkWriteToken;
    if (m_trkWriteToken == 0)
        ++m_trkWriteToken;
    return m_trkWriteToken;
}

void TrkWriteQueue::queueTrkMessage(byte code, TrkCallback callback,
    const QByteArray &data, const QVariant &cookie)
{
    const byte token = code == TRK_WRITE_QUEUE_NOOP_CODE ?
                                byte(0) : nextTrkWriteToken();
    TrkMessage msg(code, token, callback);
    msg.data = data;
    msg.cookie = cookie;
    m_trkWriteQueue.append(msg);
}

bool TrkWriteQueue::pendingMessage(TrkMessage *message)
{
    // Invoked from timer, try to flush out message queue
    if (m_trkWriteBusy || m_trkWriteQueue.isEmpty())
        return false;
    // Handle the noop message, just invoke CB in slot (ower thread)
    if (m_trkWriteQueue.front().code == TRK_WRITE_QUEUE_NOOP_CODE) {
        TrkMessage noopMessage = m_trkWriteQueue.dequeue();
        if (noopMessage.callback)
            emit internalNoopMessageDequeued(noopMessage);
    }
    // Check again for real messages
    if (m_trkWriteQueue.isEmpty())
        return false;
    if (message)
        *message = m_trkWriteQueue.front();
    return true;
}

void TrkWriteQueue::invokeNoopMessage(trk::TrkMessage noopMessage)
{
    TrkResult result;
    result.code = noopMessage.code;
    result.token = noopMessage.token;
    result.data = noopMessage.data;
    result.cookie = noopMessage.cookie;
    noopMessage.callback(result);
}

void TrkWriteQueue::notifyWriteResult(bool ok)
{
    // On success, dequeue message and await result
    if (ok) {        
        TrkMessage firstMsg = m_trkWriteQueue.dequeue();
        m_writtenTrkMessages.insert(firstMsg.token, firstMsg);
        m_trkWriteBusy = true;
    }
}

void TrkWriteQueue::slotHandleResult(const TrkResult &result)
{
    m_trkWriteBusy = false;
    //if (result.code != TrkNotifyAck && result.code != TrkNotifyNak)
    //    return;
    // Find which request the message belongs to and invoke callback
    // if ACK or on NAK if desired.
    const TokenMessageMap::iterator it = m_writtenTrkMessages.find(result.token);
    if (it == m_writtenTrkMessages.end())
        return;
    const bool invokeCB = it.value().callback;
    if (invokeCB) {
        TrkResult result1 = result;
        result1.cookie = it.value().cookie;
        it.value().callback(result1);
    }
    m_writtenTrkMessages.erase(it);
}

void TrkWriteQueue::queueTrkInitialPing()
{
    // Ping, reset sequence count
    m_trkWriteToken = 0;
    m_trkWriteQueue.append(TrkMessage(0, 0));
}

///////////////////////////////////////////////////////////////////////
//
// DeviceContext to be shared between threads
//
///////////////////////////////////////////////////////////////////////

struct DeviceContext {
    DeviceContext();
#ifdef Q_OS_WIN
    HANDLE device;
#else
    QFile file;
#endif
    bool serialFrame;
    QMutex mutex;
};

DeviceContext::DeviceContext() :
#ifdef Q_OS_WIN
    device(INVALID_HANDLE_VALUE),
#endif
    serialFrame(true)
{
}

///////////////////////////////////////////////////////////////////////
//
// TrkWriterThread: A thread operating a TrkWriteQueue.
//
///////////////////////////////////////////////////////////////////////

class WriterThread : public QThread {
    Q_OBJECT
    Q_DISABLE_COPY(WriterThread)
public:            
    explicit WriterThread(const QSharedPointer<DeviceContext> &context);

    // Enqueue messages.
    void queueTrkMessage(byte code, TrkCallback callback,
                        const QByteArray &data, const QVariant &cookie);
    void queueTrkInitialPing();

    // Call this from the device read notification with the results.
    void slotHandleResult(const TrkResult &result);

    virtual void run();

signals:
    void error(const QString &);

public slots:
    bool trkWriteRawMessage(const TrkMessage &msg);
    void terminate();
    void tryWrite();

private:    
    bool write(const QByteArray &data, QString *errorMessage);

    const QSharedPointer<DeviceContext> m_context;
    QMutex m_dataMutex;
    QMutex m_waitMutex;
    QWaitCondition m_waitCondition;
    TrkWriteQueue m_queue;
    bool m_terminate;
};

WriterThread::WriterThread(const QSharedPointer<DeviceContext> &context) :
    m_context(context),
    m_terminate(false)
{
}

void WriterThread::run()
{
    while (true) {
        // Wait. Use a timeout in case something is already queued before we
        // start up or some weird hanging exit condition
        m_waitMutex.lock();
        m_waitCondition.wait(&m_waitMutex, 100);
        m_waitMutex.unlock();
        if (m_terminate)
            break;
        // Send off message
        m_dataMutex.lock();
        TrkMessage message;
        if (m_queue.pendingMessage(&message)) {
            const bool success = trkWriteRawMessage(message);
            m_queue.notifyWriteResult(success);
        }
        m_dataMutex.unlock();
    }
}

void WriterThread::terminate()
{
    m_terminate = true;
    m_waitCondition.wakeAll();
    wait();
}

bool WriterThread::write(const QByteArray &data, QString *errorMessage)
{
    QMutexLocker(&m_context->mutex);
#ifdef Q_OS_WIN
    DWORD charsWritten;
    if (!WriteFile(m_context->device, data.data(), data.size(), &charsWritten, NULL)) {
        *errorMessage = QString::fromLatin1("Error writing data: %1").arg(winErrorMessage(GetLastError()));
        return false;
    }
    FlushFileBuffers(m_context->device);
    return true;
#else
    if (m_context->file.write(data) == -1 || !m_context->file.flush()) {
        *errorMessage = QString::fromLatin1("Cannot write: %1").arg(m_context->file.errorString());
        return false;
    }
    return  true;
#endif
}

bool WriterThread::trkWriteRawMessage(const TrkMessage &msg)
{
    const QByteArray ba = frameMessage(msg.code, msg.token, msg.data, m_context->serialFrame);
    QString errorMessage;
    const bool rc = write(ba, &errorMessage);
    if (!rc)
        emit error(errorMessage);
    return rc;
}

void WriterThread::tryWrite()
{
    m_waitCondition.wakeAll();
}

void WriterThread::queueTrkMessage(byte code, TrkCallback callback,
                                   const QByteArray &data, const QVariant &cookie)
{
    m_dataMutex.lock();
    m_queue.queueTrkMessage(code, callback, data, cookie);
    m_dataMutex.unlock();
    tryWrite();
}

void WriterThread::queueTrkInitialPing()
{
    m_dataMutex.lock();
    m_queue.queueTrkInitialPing();
    m_dataMutex.unlock();
    tryWrite();
}

// Call this from the device read notification with the results.
void WriterThread::slotHandleResult(const TrkResult &result)
{
    m_queue.slotHandleResult(result);
    tryWrite(); // Have messages been enqueued in-between?
}

///////////////////////////////////////////////////////////////////////
//
// TrkDevicePrivate
//
///////////////////////////////////////////////////////////////////////

struct TrkDevicePrivate
{
    TrkDevicePrivate();

    QSharedPointer<DeviceContext> deviceContext;
    QSharedPointer<WriterThread> writerThread;

    QByteArray trkReadBuffer;
    int timerId;
    int verbose;
    QString errorString;
};

///////////////////////////////////////////////////////////////////////
//
// TrkDevice
//
///////////////////////////////////////////////////////////////////////

TrkDevicePrivate::TrkDevicePrivate() :
    deviceContext(new DeviceContext),
    timerId(-1),
    verbose(0)
{
}

///////////////////////////////////////////////////////////////////////
//
// TrkDevice
//
///////////////////////////////////////////////////////////////////////

TrkDevice::TrkDevice(QObject *parent) :
    QObject(parent),
    d(new TrkDevicePrivate)
{}

TrkDevice::~TrkDevice()
{
    close();
    delete d;
}

bool TrkDevice::open(const QString &port, QString *errorMessage)
{
    close();
#ifdef Q_OS_WIN
    d->deviceContext->device = CreateFile(port.toStdWString().c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (INVALID_HANDLE_VALUE == d->deviceContext->device) {
        *errorMessage = QString::fromLatin1("Could not open device '%1': %2").arg(port, winErrorMessage(GetLastError()));
        return false;
    }
#else
    d->deviceContext->file.setFileName(port);
    if (!d->deviceContext->file.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(port, d->deviceContext->file.errorString());
        return false;
    }

    struct termios termInfo;
    if (tcgetattr(d->deviceContext->file.handle(), &termInfo) < 0) {
        *errorMessage = QString::fromLatin1("Unable to retrieve terminal settings: %1 %2").arg(errno).arg(QString::fromAscii(strerror(errno)));
        return false;
    }
    // Turn off terminal echo as not get messages back, among other things
    termInfo.c_cflag |= CREAD|CLOCAL;
    termInfo.c_lflag &= (~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ISIG));
    termInfo.c_iflag &= (~(INPCK|IGNPAR|PARMRK|ISTRIP|ICRNL|IXANY));
    termInfo.c_oflag &= (~OPOST);
    termInfo.c_cc[VMIN]  = 0;
    termInfo.c_cc[VINTR] = _POSIX_VDISABLE;
    termInfo.c_cc[VQUIT] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTART] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTOP] = _POSIX_VDISABLE;
    termInfo.c_cc[VSUSP] = _POSIX_VDISABLE;
    if (tcsetattr(d->deviceContext->file.handle(), TCSAFLUSH, &termInfo) < 0) {
        *errorMessage = QString::fromLatin1("Unable to apply terminal settings: %1 %2").arg(errno).arg(QString::fromAscii(strerror(errno)));
        return false;
    }
#endif
    d->timerId = startTimer(TimerInterval);
    d->writerThread = QSharedPointer<WriterThread>(new WriterThread(d->deviceContext));
    connect(d->writerThread.data(), SIGNAL(error(QString)), this, SIGNAL(error(QString)),
            Qt::QueuedConnection);
    d->writerThread->start();
    if (d->verbose)
        qDebug() << "Opened" << port;
    return true;
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
    CloseHandle(d->deviceContext->device);
    d->deviceContext->device = INVALID_HANDLE_VALUE;
#else
    d->deviceContext->file.close();
#endif
    d->writerThread->terminate();
    if (d->verbose)
        emitLogMessage("Close");
}

bool TrkDevice::isOpen() const
{
#ifdef Q_OS_WIN
    return d->deviceContext->device != INVALID_HANDLE_VALUE;
#else
    return d->deviceContext->file.isOpen();
#endif
}

QString TrkDevice::errorString() const
{
    return d->errorString;
}

bool TrkDevice::serialFrame() const
{
    return d->deviceContext->serialFrame;
}

void TrkDevice::setSerialFrame(bool f)
{
    d->deviceContext->serialFrame = f;
}

int TrkDevice::verbose() const
{
    return d->verbose;
}

void TrkDevice::setVerbose(int b)
{
    d->verbose = b;
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
    {
        QMutexLocker(&d->deviceContext->mutex);
        while (TryReadFile(d->deviceContext->device, buffer, BUFFERSIZE, &charsRead, NULL)) {
            totalCharsRead += charsRead;
            d->trkReadBuffer.append(buffer, charsRead);
            if (isValidTrkResult(d->trkReadBuffer, d->deviceContext->serialFrame))
                break;
        }
    }
    if (d->verbose > 1 && totalCharsRead)
        emitLogMessage("Read" + d->trkReadBuffer.toHex());
    if (!totalCharsRead)
        return;
    const ushort len = isValidTrkResult(d->trkReadBuffer, d->deviceContext->serialFrame);
    if (!len) {
        const QString msg = QString::fromLatin1("Partial message: %1").arg(stringFromArray(d->trkReadBuffer));
        emitError(msg);
        return;
    }
#else
    QByteArray data;
    {
        QMutexLocker(&d->deviceContext->mutex);
        const int size = bytesAvailable(d->deviceContext->file.handle());
        if (!size)
            return;
        data = d->deviceContext->file.read(size);
    }
    if (d->verbose > 1)
        emitLogMessage("trk: <- " + stringFromArray(data));
    d->trkReadBuffer.append(data);
    const ushort len = isValidTrkResult(d->trkReadBuffer, d->deviceContext->serialFrame);
    if (!len) {
        if (d->trkReadBuffer.size() > 10) {
            const QString msg = QString::fromLatin1("Unable to extract message from '%1' '%2'").
                             arg(QLatin1String(d->trkReadBuffer.toHex())).arg(QString::fromAscii(d->trkReadBuffer));
            emitError(msg);
        }
        return;
    }
#endif // Q_OS_WIN
    TrkResult r;
    QByteArray rawData;
    while (extractResult(&d->trkReadBuffer, d->deviceContext->serialFrame, &r, &rawData)) {
        if (d->verbose > 1)
            emitLogMessage("Read TrkResult " + r.data.toHex());
        d->writerThread->slotHandleResult(r);
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

void TrkDevice::sendTrkMessage(byte code, TrkCallback callback,
     const QByteArray &data, const QVariant &cookie)
{
    if (!d->writerThread.isNull())
        d->writerThread->queueTrkMessage(code, callback, data, cookie);
}

void TrkDevice::sendTrkInitialPing()
{
    if (!d->writerThread.isNull())
        d->writerThread->queueTrkInitialPing();
}

bool TrkDevice::sendTrkAck(byte token)
{
    if (d->writerThread.isNull())
        return false;
    // The acknowledgement must not be queued!
    TrkMessage msg(0x80, token);
    msg.token = token;
    msg.data.append('\0');
    return d->writerThread->trkWriteRawMessage(msg);
    // 01 90 00 07 7e 80 01 00 7d 5e 7e
}

void TrkDevice::emitLogMessage(const QString &msg)
{
    if (d->verbose)
        qDebug("%s\n", qPrintable(msg));
    emit logMessage(msg);
}

} // namespace trk

#include "trkdevice.moc"
