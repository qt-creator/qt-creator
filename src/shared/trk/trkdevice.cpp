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
/* Required headers for select() according to POSIX.1-2001 */
#  include <sys/select.h>
/* Required headers for select() according to earlier standards:
       #include <sys/time.h>
       #include <sys/types.h>
       #include <unistd.h>
*/
#endif

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

#endif

enum { verboseTrk = 0 };

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

QDebug operator<<(QDebug d, const TrkMessage &msg)
{
    return d << "Message: Code: " << msg.code
        << " Token: " << msg.token << " " << msg.data.toHex();
}

} // namespace trk

Q_DECLARE_METATYPE(trk::TrkMessage)
Q_DECLARE_METATYPE(trk::TrkResult)

namespace trk {

///////////////////////////////////////////////////////////////////////
//
// TrkWriteQueue: Mixin class that manages a write queue of Trk messages.
// pendingMessage()/notifyWriteResult() should be called from a worked/timer
// that writes the messages. The class does not take precautions for multithreading.
// A no-op message is simply taken off the queue. The calling class
// can use the helper invokeNoopMessage() to trigger its callback.
//
///////////////////////////////////////////////////////////////////////

class TrkWriteQueue
{    
    Q_DISABLE_COPY(TrkWriteQueue)
public:
    explicit TrkWriteQueue();

    // Enqueue messages.
    void queueTrkMessage(byte code, TrkCallback callback,
                        const QByteArray &data, const QVariant &cookie);
    void queueTrkInitialPing();

    // Call this from the device read notification with the results.
    void slotHandleResult(const TrkResult &result, QMutex *mutex = 0);

    // pendingMessage() can be called periodically in a timer to retrieve
    // the pending messages to be sent.
    enum PendingMessageResult {
        NoMessage,               // No message in queue.
        PendingMessage,          /* There is a queued message. The calling class
                                  * can write it out and use notifyWriteResult()
                                  * to notify about the result. */
        NoopMessageDequeued      // A no-op message has been dequeued. see invokeNoopMessage().
    };

    PendingMessageResult pendingMessage(TrkMessage *message);
    // Notify the queue about the success of the write operation
    // after taking the pendingMessage off.
    enum WriteResult {
        WriteOk,
        WriteFailedDiscard, // Discard failed message
        WriteFailedKeep,    // Keep failed message
    };
    void notifyWriteResult(WriteResult ok);

    // Helper function that invokes the callback of a no-op message
    static void invokeNoopMessage(trk::TrkMessage);

private:
    typedef QMap<byte, TrkMessage> TokenMessageMap;

    byte nextTrkWriteToken();

    byte m_trkWriteToken;
    QQueue<TrkMessage> m_trkWriteQueue;
    TokenMessageMap m_writtenTrkMessages;
    bool m_trkWriteBusy;
};

TrkWriteQueue::TrkWriteQueue() :
    m_trkWriteToken(0),
    m_trkWriteBusy(false)
{
}

byte TrkWriteQueue::nextTrkWriteToken()
{
    ++m_trkWriteToken;
    if (m_trkWriteToken == 0)
        ++m_trkWriteToken;
    if (verboseTrk)
        qDebug() << "Write token: " << m_trkWriteToken;
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

TrkWriteQueue::PendingMessageResult TrkWriteQueue::pendingMessage(TrkMessage *message)
{
    // Invoked from timer, try to flush out message queue
    if (m_trkWriteBusy || m_trkWriteQueue.isEmpty())
        return NoMessage;
    // Handle the noop message, just invoke CB in slot (ower thread)
    if (m_trkWriteQueue.front().code == TRK_WRITE_QUEUE_NOOP_CODE) {
        *message = m_trkWriteQueue.dequeue();
        return NoopMessageDequeued;
    }
    // Insert into map fir answers (as reading threads might get an
    // answer before notifyWriteResult(true)) is called.
    *message = m_trkWriteQueue.front();
    m_writtenTrkMessages.insert(message->token, *message);
    m_trkWriteBusy = true;
    return PendingMessage;
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

void TrkWriteQueue::notifyWriteResult(WriteResult wr)
{
    // On success, dequeue message and await result
    const byte token = m_trkWriteQueue.front().token;
    switch (wr) {
    case WriteOk:
        m_trkWriteQueue.dequeue();
        break;
    case WriteFailedKeep:
    case WriteFailedDiscard:
        m_writtenTrkMessages.remove(token);
        m_trkWriteBusy = false;
        if (wr == WriteFailedDiscard)
            m_trkWriteQueue.dequeue();
        break;
    }
}

void TrkWriteQueue::slotHandleResult(const TrkResult &result, QMutex *mutex)
{
    // Find which request the message belongs to and invoke callback
    // if ACK or on NAK if desired.
    if (mutex)
        mutex->lock();
    m_trkWriteBusy = false;
    const TokenMessageMap::iterator it = m_writtenTrkMessages.find(result.token);
    if (it == m_writtenTrkMessages.end()) {
        if (mutex)
            mutex->unlock();
        return;
    }
    TrkCallback callback = it.value().callback;
    const QVariant cookie = it.value().cookie;
    m_writtenTrkMessages.erase(it);
    if (mutex)
        mutex->unlock();
    // Invoke callback
    if (callback) {
        TrkResult result1 = result;
        result1.cookie = cookie;
        callback(result1);
    }
}

void TrkWriteQueue::queueTrkInitialPing()
{
    // Ping, reset sequence count
    m_trkWriteToken = 0;
    m_trkWriteQueue.append(TrkMessage(TrkPing, 0));
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
    OVERLAPPED readOverlapped;
    OVERLAPPED writeOverlapped;
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
// with exception of the handling of the  TRK_WRITE_QUEUE_NOOP_CODE
// synchronization message. The invocation of the callback is then
// done by the thread owning the TrkWriteQueue, while pendingMessage() is called
// from another thread. This happens via a Qt::BlockingQueuedConnection.

///////////////////////////////////////////////////////////////////////

class WriterThread : public QThread
{
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
    void internalNoopMessageDequeued(const trk::TrkMessage&);

public slots:
    bool trkWriteRawMessage(const TrkMessage &msg);
    void terminate();
    void tryWrite();

private slots:
    void invokeNoopMessage(const trk::TrkMessage &);

private:    
    bool write(const QByteArray &data, QString *errorMessage);
    inline int writePendingMessage();

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
    static const int trkMessageMetaId = qRegisterMetaType<trk::TrkMessage>();
    Q_UNUSED(trkMessageMetaId)
    connect(this, SIGNAL(internalNoopMessageDequeued(trk::TrkMessage)),
            this, SLOT(invokeNoopMessage(trk::TrkMessage)), Qt::BlockingQueuedConnection);
}

void WriterThread::run()
{
    while (writePendingMessage() == 0) ;
}

int WriterThread::writePendingMessage()
{
    enum { MaxAttempts = 100, RetryIntervalMS = 200 };

    // Wait. Use a timeout in case something is already queued before we
    // start up or some weird hanging exit condition
    m_waitMutex.lock();
    m_waitCondition.wait(&m_waitMutex, 100);
    m_waitMutex.unlock();
    if (m_terminate)
        return 1;

    // Send off message
    m_dataMutex.lock();
    TrkMessage message;
    const TrkWriteQueue::PendingMessageResult pr = m_queue.pendingMessage(&message);
    m_dataMutex.unlock();

    switch (pr) {
    case TrkWriteQueue::NoMessage:
        break;
    case TrkWriteQueue::PendingMessage: {
            //qDebug() << "Write pending message " << message;
            // Untested: try to re-send a few times
            bool success = false;
            for (int r = 0; !success && (r < MaxAttempts); r++) {
                success = trkWriteRawMessage(message);
                if (!success) {
                    emit error(QString::fromLatin1("Write failure, attempt %1 of %2.").arg(r).arg(int(MaxAttempts)));
                    if (m_terminate)
                        return 1;
                    QThread::msleep(RetryIntervalMS);
                }
            }
            // Notify queue. If still failed, give up.
            m_dataMutex.lock();
            m_queue.notifyWriteResult(success ? TrkWriteQueue::WriteOk : TrkWriteQueue::WriteFailedDiscard);
            m_dataMutex.unlock();
        }
        break;
    case TrkWriteQueue::NoopMessageDequeued:
        // Sync with thread that owns us via a blocking signal
        if (verboseTrk)
            qDebug() << "Noop message dequeued" << message;
        emit internalNoopMessageDequeued(message);
        break;
    } // switch
    return 0;
}

void WriterThread::invokeNoopMessage(const trk::TrkMessage &msg)
{
    TrkWriteQueue::invokeNoopMessage(msg);
}

void WriterThread::terminate()
{
    m_terminate = true;
    m_waitCondition.wakeAll();
    wait();
    m_terminate = false;
}

#ifdef Q_OS_WIN

static inline QString msgTerminated(int size)
{
    return QString::fromLatin1("Terminated with %1 bytes pending.").arg(size);
}

// Interruptible synchronous write function.
static inline bool overlappedSyncWrite(HANDLE file,
                                       const bool &terminateFlag,
                                       const char *data,
                                       DWORD size, DWORD *charsWritten,
                                       OVERLAPPED *overlapped,
                                       QString *errorMessage)
{
    if (WriteFile(file, data, size, charsWritten, overlapped))
        return true;
    const DWORD writeError = GetLastError();
    if (writeError != ERROR_IO_PENDING) {
        *errorMessage = QString::fromLatin1("WriteFile failed: %1").arg(winErrorMessage(writeError));
        return false;
    }
    // Wait for written or thread terminated
    const DWORD timeoutMS = 200;
    const unsigned maxAttempts = 20;
    DWORD wr = WaitForSingleObject(overlapped->hEvent, timeoutMS);
    for (unsigned n = 0; wr == WAIT_TIMEOUT && n < maxAttempts && !terminateFlag;
         wr = WaitForSingleObject(overlapped->hEvent, timeoutMS), n++);
    if (terminateFlag) {
        *errorMessage = msgTerminated(size);
        return false;
    }
    switch (wr) {
    case WAIT_OBJECT_0:
        break;
    case WAIT_TIMEOUT:
        *errorMessage = QString::fromLatin1("Write timed out.");
        return false;
    default:
        *errorMessage = QString::fromLatin1("Error while waiting for WriteFile results: %1").arg(winErrorMessage(GetLastError()));
        return false;
    }
    if (!GetOverlappedResult(file, overlapped, charsWritten, TRUE)) {
        *errorMessage = QString::fromLatin1("Error writing %1 bytes: %2").arg(size).arg(winErrorMessage(GetLastError()));
        return false;
    }
    return true;
}
#endif

bool WriterThread::write(const QByteArray &data, QString *errorMessage)
{
    if (verboseTrk)
        qDebug() << "Write raw data: " << data.toHex();
    QMutexLocker locker(&m_context->mutex);
#ifdef Q_OS_WIN
    DWORD charsWritten;
    if (!overlappedSyncWrite(m_context->device, m_terminate, data.data(), data.size(), &charsWritten, &m_context->writeOverlapped, errorMessage)) {
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
    if (!rc) {
        qWarning("%s\n", qPrintable(errorMessage));
        emit error(errorMessage);
    }
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
    m_queue.slotHandleResult(result, &m_dataMutex);
    tryWrite(); // Have messages been enqueued in-between?
}


///////////////////////////////////////////////////////////////////////
//
// ReaderThreadBase: Base class for a thread that reads data from
// the device, decodes the messages and emit signals for the messages.
// A Qt::BlockingQueuedConnection should be used for the message signal
// to ensure messages are processed in the correct sequence.
//
///////////////////////////////////////////////////////////////////////

class ReaderThreadBase : public QThread
{
    Q_OBJECT
    Q_DISABLE_COPY(ReaderThreadBase)
public:

signals:
    void messageReceived(const trk::TrkResult &result, const QByteArray &rawData);

protected:
    explicit ReaderThreadBase(const QSharedPointer<DeviceContext> &context);
    void processData(const QByteArray &a);
    void processData(char c);

    const QSharedPointer<DeviceContext> m_context;

private:
    void readMessages();

    QByteArray m_trkReadBuffer;
};

ReaderThreadBase::ReaderThreadBase(const QSharedPointer<DeviceContext> &context) :
    m_context(context)
{
    static const int trkResultMetaId = qRegisterMetaType<trk::TrkResult>();
    Q_UNUSED(trkResultMetaId)
}

void ReaderThreadBase::processData(const QByteArray &a)
{
    m_trkReadBuffer += a;
    readMessages();
}

void ReaderThreadBase::processData(char c)
{
    m_trkReadBuffer += c;
    if (m_trkReadBuffer.size() > 1)
        readMessages();
}

void ReaderThreadBase::readMessages()
{
    TrkResult r;
    QByteArray rawData;
    while (extractResult(&m_trkReadBuffer, m_context->serialFrame, &r, &rawData)) {
        emit messageReceived(r, rawData);
    }
}

#ifdef Q_OS_WIN
///////////////////////////////////////////////////////////////////////
//
// WinReaderThread: A thread reading from the device using Windows API.
// Waits on an overlapped I/O handle and an event that tells the thread to
// terminate.
//
///////////////////////////////////////////////////////////////////////

class WinReaderThread : public ReaderThreadBase
{
    Q_OBJECT
    Q_DISABLE_COPY(WinReaderThread)
public:
    explicit WinReaderThread(const QSharedPointer<DeviceContext> &context);
    ~WinReaderThread();

    virtual void run();

signals:
    void error(const QString &);

public slots:
    void terminate();

private:
    enum Handles { FileHandle, TerminateEventHandle, HandleCount };

    inline int tryRead();

    HANDLE m_handles[HandleCount];
};

WinReaderThread::WinReaderThread(const QSharedPointer<DeviceContext> &context) :
    ReaderThreadBase(context)
{
    m_handles[FileHandle] = NULL;
    m_handles[TerminateEventHandle] = CreateEvent(NULL, FALSE, FALSE, NULL);
}

WinReaderThread::~WinReaderThread()
{
    CloseHandle(m_handles[TerminateEventHandle]);
}

// Return 0 to continue or error code
int WinReaderThread::tryRead()
{
    enum { BufSize = 1024 };
    char buffer[BufSize];
    // Check if there are already bytes waiting. If not, wait for first byte
    COMSTAT comStat;
    if (!ClearCommError(m_context->device, NULL, &comStat)){
        emit error(QString::fromLatin1("ClearCommError failed: %1").arg(winErrorMessage(GetLastError())));
        return -7;
    }    
    const DWORD bytesToRead = qMax(DWORD(1), qMin(comStat.cbInQue, DWORD(BufSize)));
    // Trigger read
    DWORD bytesRead = 0;
    if (ReadFile(m_context->device, &buffer, bytesToRead, &bytesRead, &m_context->readOverlapped)) {
        if (bytesRead == 1) {
            processData(buffer[0]);
        } else {
            processData(QByteArray(buffer, bytesRead));
        }
        return 0;
    }
    const DWORD readError = GetLastError();
    if (readError != ERROR_IO_PENDING) {
        emit error(QString::fromLatin1("Read error: %1").arg(winErrorMessage(readError)));
        return -1;
    }    
    // Wait for either termination or data
    const DWORD wr = WaitForMultipleObjects(HandleCount, m_handles, false, INFINITE);
    if (wr == WAIT_FAILED) {
        emit error(QString::fromLatin1("Wait failed: %1").arg(winErrorMessage(GetLastError())));
        return -2;
    }
    if (wr - WAIT_OBJECT_0 == TerminateEventHandle) {
        return 1; // Terminate
    }
    // Check data
    if (!GetOverlappedResult(m_context->device, &m_context->readOverlapped, &bytesRead, true)) {
        emit error(QString::fromLatin1("GetOverlappedResult failed: %1").arg(winErrorMessage(GetLastError())));
        return -3;
    }
    if (bytesRead == 1) {
        processData(buffer[0]);
    } else {
        processData(QByteArray(buffer, bytesRead));
    }
    return 0;
}

void WinReaderThread::run()
{
    m_handles[FileHandle] = m_context->readOverlapped.hEvent;
    while ( tryRead() == 0) ;
}

void WinReaderThread::terminate()
{
    SetEvent(m_handles[TerminateEventHandle]);
    wait();
}

typedef WinReaderThread ReaderThread;

#else

///////////////////////////////////////////////////////////////////////
//
// UnixReaderThread: A thread reading from the device.
// Uses select() to wait and a special ioctl() to find out the number
// of bytes queued. For clean termination, the self-pipe trick is used.
// The class maintains a pipe, on whose read end the select waits besides
// the device file handle. To terminate, a byte is written to the pipe.
//
///////////////////////////////////////////////////////////////////////

static inline QString msgUnixCallFailedErrno(const char *func, int errorNumber)
{
    return QString::fromLatin1("Call to %1() failed: %2").arg(QLatin1String(func), QString::fromLocal8Bit(strerror(errorNumber)));
}

class UnixReaderThread : public ReaderThreadBase {
    Q_OBJECT
    Q_DISABLE_COPY(UnixReaderThread)
public:
    explicit UnixReaderThread(const QSharedPointer<DeviceContext> &context);
    ~UnixReaderThread();

    virtual void run();

signals:
    void error(const QString &);

public slots:
    void terminate();

private:
    inline int tryRead();

    int m_terminatePipeFileDescriptors[2];
};

UnixReaderThread::UnixReaderThread(const QSharedPointer<DeviceContext> &context) : 
    ReaderThreadBase(context)
{
    m_terminatePipeFileDescriptors[0] = m_terminatePipeFileDescriptors[1] = -1;
    // Set up pipes for termination. Should not fail
    if (pipe(m_terminatePipeFileDescriptors) < 0)
        qWarning("%s\n", qPrintable(msgUnixCallFailedErrno("pipe", errno)));
}

UnixReaderThread::~UnixReaderThread()
{
    close(m_terminatePipeFileDescriptors[0]);
    close(m_terminatePipeFileDescriptors[1]);
}

int UnixReaderThread::tryRead()
{
    fd_set readSet, tempReadSet, tempExceptionSet;
    struct timeval timeOut;
    const int fileDescriptor = m_context->file.handle();
    FD_ZERO(&readSet);
    FD_SET(fileDescriptor, &readSet);
    FD_SET(m_terminatePipeFileDescriptors[0], &readSet);
    const int maxFileDescriptor = qMax(m_terminatePipeFileDescriptors[0], fileDescriptor);
    int result = 0;
    do {
        memcpy(&tempReadSet, &readSet, sizeof(fd_set));
        memcpy(&tempExceptionSet, &readSet, sizeof(fd_set));
        timeOut.tv_sec = 1;
        timeOut.tv_usec = 0;
        result = select(maxFileDescriptor + 1, &tempReadSet, NULL, &tempExceptionSet, &timeOut);
    } while ( result < 0 && errno == EINTR );
    // Timeout?
    if (result == 0)
        return 0;
   // Something wrong?
    if (result < 0) {
        emit error(msgUnixCallFailedErrno("select", errno));
        return -1;
    }
    // Did the exception set trigger on the device?
    if (FD_ISSET(fileDescriptor,&tempExceptionSet)) {
        emit error(QLatin1String("An Exception occurred on the device."));
        return -2;
    }
    // Check termination pipe.
    if (FD_ISSET(m_terminatePipeFileDescriptors[0], &tempReadSet)
        || FD_ISSET(m_terminatePipeFileDescriptors[0], &tempExceptionSet))
        return 1;

    // determine number of pending bytes and read
    int numBytes;
    if (ioctl(fileDescriptor, FIONREAD, &numBytes) < 0) {
        emit error(msgUnixCallFailedErrno("ioctl", errno));
        return -1;
    }
    m_context->mutex.lock();
    const QByteArray data = m_context->file.read(numBytes);
    m_context->mutex.unlock();
    processData(data);
    return 0;
}

void UnixReaderThread::run()
{
    // Read loop
    while (tryRead() == 0)
        ;
}

void UnixReaderThread::terminate()
{
    // Trigger select() by writing to the pipe
    char c = 0;
    int written = write(m_terminatePipeFileDescriptors[1], &c, 1);
    // FIXME: Use result.
    wait();
}

typedef UnixReaderThread ReaderThread;

#endif

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
    QSharedPointer<ReaderThread> readerThread;

    QByteArray trkReadBuffer;
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
    if (d->verbose)
        qDebug() << "Opening" << port << "is open: " << isOpen() << " serialFrame=" << serialFrame();
    close();
#ifdef Q_OS_WIN
    d->deviceContext->device = CreateFile(port.toStdWString().c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING|FILE_FLAG_OVERLAPPED,
                           NULL);

    if (INVALID_HANDLE_VALUE == d->deviceContext->device) {
        *errorMessage = QString::fromLatin1("Could not open device '%1': %2").arg(port, winErrorMessage(GetLastError()));
        return false;
    }
    memset(&d->deviceContext->readOverlapped, 0, sizeof(OVERLAPPED));
    d->deviceContext->readOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    memset(&d->deviceContext->writeOverlapped, 0, sizeof(OVERLAPPED));
    d->deviceContext->writeOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (d->deviceContext->readOverlapped.hEvent == NULL || d->deviceContext->writeOverlapped.hEvent == NULL) {
        *errorMessage = QString::fromLatin1("Failed to create events: %1").arg(winErrorMessage(GetLastError()));
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
    d->readerThread = QSharedPointer<ReaderThread>(new ReaderThread(d->deviceContext));
    connect(d->readerThread.data(), SIGNAL(error(QString)), this, SLOT(emitError(QString)),
            Qt::QueuedConnection);
    connect(d->readerThread.data(), SIGNAL(messageReceived(trk::TrkResult,QByteArray)),
            this, SLOT(slotMessageReceived(trk::TrkResult,QByteArray)),
            Qt::QueuedConnection);
    d->readerThread->start();

    d->writerThread = QSharedPointer<WriterThread>(new WriterThread(d->deviceContext));
    connect(d->writerThread.data(), SIGNAL(error(QString)), this, SLOT(emitError(QString)),
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
    if (d->readerThread)
        d->readerThread->terminate();
    if (d->writerThread)
        d->writerThread->terminate();
#ifdef Q_OS_WIN
    CloseHandle(d->deviceContext->device);
    d->deviceContext->device = INVALID_HANDLE_VALUE;
    CloseHandle(d->deviceContext->readOverlapped.hEvent);
    CloseHandle(d->deviceContext->writeOverlapped.hEvent);
    d->deviceContext->readOverlapped.hEvent = d->deviceContext->writeOverlapped.hEvent = NULL;
#else
    d->deviceContext->file.close();
#endif
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

void TrkDevice::slotMessageReceived(const trk::TrkResult &result, const QByteArray &rawData)
{
    d->writerThread->slotHandleResult(result);
    if (d->verbose > 1)
        qDebug() << "Received: " << result.toString();
    emit messageReceived(result);    
    if (!rawData.isEmpty())
        emit rawDataReceived(rawData);
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
    if (!d->writerThread.isNull()) {
        if (d->verbose > 1)
            qDebug() << "Sending " << code << data.toHex();
        d->writerThread->queueTrkMessage(code, callback, data, cookie);
    }
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
    if (verboseTrk)
        qDebug() << "Write synchroneous message: " << msg;
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
