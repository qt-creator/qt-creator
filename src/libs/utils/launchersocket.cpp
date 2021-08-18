/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "launchersocket.h"
#include "launcherinterface.h"

#include "qtcassert.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qelapsedtimer.h>
#include <QtNetwork/qlocalsocket.h>

namespace Utils {
namespace Internal {

class CallerHandle : public QObject
{
    Q_OBJECT
public:
    // Called from caller's thread exclusively, lives in caller's thread.
    CallerHandle() : QObject() {}

    // Called from caller's thread exclusively. Returns the list of flushed signals.
    QList<LauncherHandle::SignalType> flush()
    {
        QTC_ASSERT(isCalledFromCallersThread(), return {});
        QList<LauncherHandle::SignalType> oldSignals;
        {
            QMutexLocker locker(&m_mutex);
            oldSignals = m_signals;
            m_signals = {};
        }
        for (LauncherHandle::SignalType signalType : qAsConst(oldSignals)) {
            switch (signalType) {
            case LauncherHandle::SignalType::NoSignal:
                break;
            case LauncherHandle::SignalType::Error:
                emit errorOccurred();
                break;
            case LauncherHandle::SignalType::Started:
                emit started();
                break;
            case LauncherHandle::SignalType::ReadyRead:
                emit readyRead();
                break;
            case LauncherHandle::SignalType::Finished:
                emit finished();
                break;
            }
        }
        return oldSignals;
    }
    // Called from caller's thread exclusively.
    bool shouldFlushFor(LauncherHandle::SignalType signalType)
    {
        QTC_ASSERT(isCalledFromCallersThread(), return false);
        // TODO: Should we always flush when the list isn't empty?
        QMutexLocker locker(&m_mutex);
        if (m_signals.contains(signalType))
            return true;
        if (m_signals.contains(LauncherHandle::SignalType::Error))
            return true;
        if (m_signals.contains(LauncherHandle::SignalType::Finished))
            return true;
        return false;
    }
    // Called from launcher's thread exclusively.
    void appendSignal(LauncherHandle::SignalType signalType)
    {
        QTC_ASSERT(!isCalledFromCallersThread(), return);
        if (signalType == LauncherHandle::SignalType::NoSignal)
            return;

        QMutexLocker locker(&m_mutex);
        if (m_signals.contains(signalType))
            return;

        m_signals.append(signalType);
    }
signals:
    // Emitted from caller's thread exclusively.
    void errorOccurred();
    void started();
    void readyRead();
    void finished();
private:
    // Called from caller's or launcher's thread.
    bool isCalledFromCallersThread() const
    {
        return QThread::currentThread() == thread();
    }

    QMutex m_mutex;
    QList<LauncherHandle::SignalType> m_signals;
};

void LauncherHandle::handlePacket(LauncherPacketType type, const QByteArray &payload)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    switch (type) {
    case LauncherPacketType::ProcessError:
        handleErrorPacket(payload);
        break;
    case LauncherPacketType::ProcessStarted:
        handleStartedPacket(payload);
        break;
    case LauncherPacketType::ReadyReadStandardOutput:
        handleReadyReadStandardOutput(payload);
        break;
    case LauncherPacketType::ReadyReadStandardError:
        handleReadyReadStandardError(payload);
        break;
    case LauncherPacketType::ProcessFinished:
        handleFinishedPacket(payload);
        break;
    default:
        QTC_ASSERT(false, break);
    }
}

void LauncherHandle::handleErrorPacket(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(SignalType::Error);

    const auto packet = LauncherPacket::extractPacket<ProcessErrorPacket>(m_token, packetData);
    m_error = packet.error;
    m_errorString = packet.errorString;
    if (!m_callerHandle)
        return;

    m_callerHandle->appendSignal(SignalType::Error);
    flushCaller();
}

// call me with mutex locked
void LauncherHandle::wakeUpIfWaitingFor(SignalType newSignal)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    // TODO: should we always wake up in case m_waitingFor != NoSignal?
    // The matching signal came
    const bool signalMatched = (m_waitingFor == newSignal);
    // E.g. if we are waiting for ReadyRead and we got Finished or Error signal instead -> wake it, too.
    const bool finishedOrErrorWhileWaiting =
            (m_waitingFor != SignalType::NoSignal)
            && ((newSignal == SignalType::Finished) || (newSignal == SignalType::Error));
    // Wake up, flush and continue waiting.
    // E.g. when being in waitingForFinished() state and Started or ReadyRead signal came.
    const bool continueWaitingAfterFlushing =
            ((m_waitingFor == SignalType::Finished) && (newSignal != SignalType::Finished))
            || ((m_waitingFor == SignalType::ReadyRead) && (newSignal == SignalType::Started));
    const bool shouldWake = signalMatched
                         || finishedOrErrorWhileWaiting
                         || continueWaitingAfterFlushing;

    if (shouldWake)
        m_waitCondition.wakeOne();
}

void LauncherHandle::sendPacket(const Internal::LauncherPacket &packet)
{
    LauncherInterface::socket()->sendData(packet.serialize());
}

bool LauncherHandle::isCalledFromLaunchersThread() const
{
    return QThread::currentThread() == thread();
}

// call me with mutex locked
bool LauncherHandle::isCalledFromCallersThread() const
{
    if (!m_callerHandle)
        return false;
    return QThread::currentThread() == m_callerHandle->thread();
}

// call me with mutex locked
void LauncherHandle::flushCaller()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    if (!m_callerHandle)
        return;

    // call in callers thread
    QMetaObject::invokeMethod(m_callerHandle, &CallerHandle::flush);
}

void LauncherHandle::handleStartedPacket(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(SignalType::Started);
    m_processState = QProcess::Running;
    const auto packet = LauncherPacket::extractPacket<ProcessStartedPacket>(m_token, packetData);
    m_processId = packet.processId;
    if (!m_callerHandle)
        return;

    m_callerHandle->appendSignal(SignalType::Started);
    flushCaller();
}

void LauncherHandle::handleReadyReadStandardOutput(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(SignalType::ReadyRead);
    const auto packet = LauncherPacket::extractPacket<ReadyReadStandardOutputPacket>(m_token, packetData);
    if (packet.standardChannel.isEmpty())
        return;

    m_stdout += packet.standardChannel;
    if (!m_callerHandle)
        return;

    m_callerHandle->appendSignal(SignalType::ReadyRead);
    flushCaller();
}

void LauncherHandle::handleReadyReadStandardError(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(SignalType::ReadyRead);
    const auto packet = LauncherPacket::extractPacket<ReadyReadStandardErrorPacket>(m_token, packetData);
    if (packet.standardChannel.isEmpty())
        return;

    m_stderr += packet.standardChannel;
    if (!m_callerHandle)
        return;

    m_callerHandle->appendSignal(SignalType::ReadyRead);
    flushCaller();
}

void LauncherHandle::handleFinishedPacket(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(SignalType::Finished);
    m_processState = QProcess::NotRunning;
    const auto packet = LauncherPacket::extractPacket<ProcessFinishedPacket>(m_token, packetData);
    m_exitCode = packet.exitCode;
    m_stdout += packet.stdOut;
    m_stderr += packet.stdErr;
    m_errorString = packet.errorString;
    m_exitStatus = packet.exitStatus;
    if (!m_callerHandle)
        return;

    if (!m_stdout.isEmpty() || !m_stderr.isEmpty())
        m_callerHandle->appendSignal(SignalType::ReadyRead);
    m_callerHandle->appendSignal(SignalType::Finished);
    flushCaller();
}

void LauncherHandle::handleSocketReady()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    m_socketError = false;
    if (m_processState == QProcess::Starting)
        doStart();
}

void LauncherHandle::handleSocketError(const QString &message)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    m_socketError = true;
    m_errorString = QCoreApplication::translate("Utils::QtcProcess",
                                                "Internal socket error: %1").arg(message);
    if (m_processState != QProcess::NotRunning) {
        m_error = QProcess::FailedToStart;
        emit errorOccurred(m_error);
    }
}

bool LauncherHandle::waitForSignal(int msecs, SignalType newSignal)
{
    QElapsedTimer timer;
    timer.start();
    while (true) {
        const int remainingMsecs = msecs - timer.elapsed();
        if (remainingMsecs <= 0)
            break;
        const bool timedOut = !doWaitForSignal(qMax(remainingMsecs, 0), newSignal);
        if (timedOut)
            break;
        m_awaitingShouldContinue = true; // TODO: make it recursive?
        const QList<SignalType> flushedSignals = m_callerHandle->flush();
        const bool wasCanceled = !m_awaitingShouldContinue;
        m_awaitingShouldContinue = false;
        const bool errorOccurred = flushedSignals.contains(SignalType::Error);
        if (errorOccurred)
            return false; // apparently QProcess behaves like this in case of error
        const bool newSignalFlushed = flushedSignals.contains(newSignal);
        if (newSignalFlushed) // so we don't continue waiting
            return true;
        if (wasCanceled)
            return true; // or false? is false only in case of timeout?
        if (timer.hasExpired(msecs))
            break;
    }
    return false;
}

static void warnAboutWrongSignal(QProcess::ProcessState state, LauncherHandle::SignalType newSignal)
{
    qWarning() << "LauncherHandle::doWaitForSignal: Can't wait for" << newSignal <<
                  "while being in" << state << "state.";
}

bool LauncherHandle::doWaitForSignal(int msecs, SignalType newSignal)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return false);
    QTC_ASSERT(m_waitingFor == SignalType::NoSignal, return false);
    // It may happen, that after calling start() and before calling waitForStarted() we might have
    // reached the Running (or even Finished) state already. In this case we should have
    // collected Started (or even Finished) signal to be flushed - so we return true
    // and we are going to flush pending signals synchronously.
    // It could also happen, that some new readyRead data has appeared, so before we wait for
    // more we flush it, too.
    if (m_callerHandle->shouldFlushFor(newSignal))
        return true;

    if (canWaitFor(newSignal)) { // e.g. can't wait for started if we are in not running state.
        m_waitingFor = newSignal;
        const bool ret = m_waitCondition.wait(&m_mutex, msecs)/* && !m_failed*/;
        m_waitingFor = SignalType::NoSignal;
        return ret;
    }
    // Can't wait for passed signal, should never happen.
    QTC_ASSERT(false, warnAboutWrongSignal(m_processState, newSignal));
    return false;
}

// call me with mutex locked
bool LauncherHandle::canWaitFor(SignalType newSignal) const
{
    QTC_ASSERT(isCalledFromCallersThread(), return false);
    switch (newSignal) {
    case SignalType::Started:
        return m_processState == QProcess::Starting;
    case SignalType::ReadyRead:
    case SignalType::Finished:
        return m_processState != QProcess::NotRunning;
    default:
        break;
    }
    return false;
}

QProcess::ProcessState LauncherHandle::state() const
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return QProcess::NotRunning);
    return m_processState;
}

void LauncherHandle::cancel()
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);

    switch (m_processState) {
    case QProcess::NotRunning:
        break;
    case QProcess::Starting:
        m_errorString = QCoreApplication::translate("Utils::LauncherHandle",
                                                    "Process canceled before it was started.");
        m_error = QProcess::FailedToStart;
        if (LauncherInterface::socket()->isReady())
            sendPacket(StopProcessPacket(m_token));
        else
            emit errorOccurred(m_error);
        break;
    case QProcess::Running:
        sendPacket(StopProcessPacket(m_token));
        break;
    }

    m_processState = QProcess::NotRunning;
    m_awaitingShouldContinue = false;
}

QByteArray LauncherHandle::readAllStandardOutput()
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    return readAndClear(m_stdout);
}

QByteArray LauncherHandle::readAllStandardError()
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    return readAndClear(m_stderr);
}

qint64 LauncherHandle::processId() const
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return 0);
    return m_processId;
}

QString LauncherHandle::errorString() const
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    return m_errorString;
}

void LauncherHandle::setErrorString(const QString &str)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_errorString = str;
}

void LauncherHandle::start(const QString &program, const QStringList &arguments, const QByteArray &writeData)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);

    if (m_socketError) {
        m_error = QProcess::FailedToStart;
        emit errorOccurred(m_error);
        return;
    }
    m_command = program;
    m_arguments = arguments;
    // TODO: check if state is not running
    // TODO: check if m_canceled is not true
    m_processState = QProcess::Starting;
    m_writeData = writeData;
    if (LauncherInterface::socket()->isReady())
        doStart();
}

qint64 LauncherHandle::write(const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return -1);

    if (m_processState != QProcess::Running)
        return -1;

    WritePacket p(m_token);
    p.inputData = data;
    sendPacket(p);
    return data.size();
}

QProcess::ProcessError LauncherHandle::error() const
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return QProcess::UnknownError);
    return m_error;
}

QString LauncherHandle::program() const
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    return m_command;
}

void LauncherHandle::setStandardInputFile(const QString &fileName)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_standardInputFile = fileName;
}

void LauncherHandle::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    if (mode != QProcess::SeparateChannels && mode != QProcess::MergedChannels) {
        qWarning("setProcessChannelMode: The only supported modes are SeparateChannels and MergedChannels.");
        return;
    }
    m_channelMode = mode;
}

void LauncherHandle::setProcessEnvironment(const QProcessEnvironment &environment)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_environment = environment;
}

void LauncherHandle::setWorkingDirectory(const QString &dir)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_workingDirectory = dir;
}

QProcess::ExitStatus LauncherHandle::exitStatus() const
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return QProcess::CrashExit);
    return m_exitStatus;
}

void LauncherHandle::setBelowNormalPriority()
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_belowNormalPriority = true;
}

void LauncherHandle::setNativeArguments(const QString &arguments)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_nativeArguments = arguments;
}

void LauncherHandle::setLowPriority()
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_lowPriority = true;
}

void LauncherHandle::setUnixTerminalDisabled()
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_unixTerminalDisabled = true;
}

// Ensure it's called from caller's thread, after moving LauncherHandle into the launcher's thread
void LauncherHandle::createCallerHandle()
{
    QMutexLocker locker(&m_mutex); // may be not needed, as we call it just after Launcher's c'tor
    QTC_ASSERT(!isCalledFromLaunchersThread(), return);
    QTC_ASSERT(m_callerHandle == nullptr, return);
    m_callerHandle = new CallerHandle();
    connect(m_callerHandle, &CallerHandle::errorOccurred, this, &LauncherHandle::slotErrorOccurred, Qt::DirectConnection);
    connect(m_callerHandle, &CallerHandle::started, this, &LauncherHandle::slotStarted, Qt::DirectConnection);
    connect(m_callerHandle, &CallerHandle::readyRead, this, &LauncherHandle::slotReadyRead, Qt::DirectConnection);
    connect(m_callerHandle, &CallerHandle::finished, this, &LauncherHandle::slotFinished, Qt::DirectConnection);
}

void LauncherHandle::destroyCallerHandle()
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return);
    QTC_ASSERT(m_callerHandle, return);
    m_callerHandle->deleteLater();
    m_callerHandle = nullptr;
}

void LauncherHandle::slotErrorOccurred()
{
    QProcess::ProcessError error = QProcess::UnknownError;
    {
        QMutexLocker locker(&m_mutex);
        QTC_ASSERT(isCalledFromCallersThread(), return);
        error = m_error;
    }
    emit errorOccurred(error);
}

void LauncherHandle::slotStarted()
{
    {
        QMutexLocker locker(&m_mutex);
        QTC_ASSERT(isCalledFromCallersThread(), return);
    }
    emit started();
}

void LauncherHandle::slotReadyRead()
{
    bool hasOutput = false;
    bool hasError = false;
    {
        QMutexLocker locker(&m_mutex);
        QTC_ASSERT(isCalledFromCallersThread(), return);
        hasOutput = !m_stdout.isEmpty();
        hasError = !m_stderr.isEmpty();
    }
    if (hasOutput)
        emit readyReadStandardOutput();
    if (hasError)
        emit readyReadStandardError();
}

void LauncherHandle::slotFinished()
{
    int exitCode = 0;
    QProcess::ExitStatus exitStatus = QProcess::NormalExit;
    {
        QMutexLocker locker(&m_mutex);
        QTC_ASSERT(isCalledFromCallersThread(), return);
        exitCode = m_exitCode;
        exitStatus = m_exitStatus;
    }
    emit finished(exitCode, exitStatus);
}

// call me with mutex locked
void LauncherHandle::doStart()
{
    StartProcessPacket p(m_token);
    p.command = m_command;
    p.arguments = m_arguments;
    p.env = m_environment.toStringList();
    p.workingDir = m_workingDirectory;
    p.processMode = m_processMode;
    p.writeData = m_writeData;
    p.channelMode = m_channelMode;
    p.standardInputFile = m_standardInputFile;
    p.belowNormalPriority = m_belowNormalPriority;
    p.nativeArguments = m_nativeArguments;
    sendPacket(p);
}

LauncherSocket::LauncherSocket(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<Utils::Internal::LauncherPacketType>();
    qRegisterMetaType<quintptr>("quintptr");
}

void LauncherSocket::sendData(const QByteArray &data)
{
    if (!isReady())
        return;

    auto storeRequest = [this](const QByteArray &data)
    {
        QMutexLocker locker(&m_mutex);
        m_requests.push_back(data);
        return m_requests.size() == 1; // Returns true if requests handling should be triggered.
    };

    if (storeRequest(data)) // Call handleRequests() in launcher's thread.
        QMetaObject::invokeMethod(this, &LauncherSocket::handleRequests);
}

LauncherHandle *LauncherSocket::registerHandle(quintptr token, ProcessMode mode)
{
    QTC_ASSERT(!isCalledFromLaunchersThread(), return nullptr);
    QMutexLocker locker(&m_mutex);
    if (m_handles.contains(token))
        return nullptr; // TODO: issue a warning

    LauncherHandle *handle = new LauncherHandle(token, mode);
    handle->moveToThread(thread());
    // Call it after moving LauncherHandle to the launcher's thread.
    // Since this method is invoked from caller's thread, CallerHandle will live in caller's thread.
    handle->createCallerHandle();
    m_handles.insert(token, handle);
    connect(this, &LauncherSocket::ready,
            handle, &LauncherHandle::handleSocketReady);
    connect(this, &LauncherSocket::errorOccurred,
            handle, &LauncherHandle::handleSocketError);

    return handle;
}

void LauncherSocket::unregisterHandle(quintptr token)
{
    QTC_ASSERT(!isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    auto it = m_handles.find(token);
    if (it == m_handles.end())
        return; // TODO: issue a warning

    LauncherHandle *handle = it.value();
    handle->destroyCallerHandle();
    handle->deleteLater();
    m_handles.erase(it);
}

LauncherHandle *LauncherSocket::handleForToken(quintptr token) const
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return nullptr);
    QMutexLocker locker(&m_mutex);
    return m_handles.value(token);
}

void LauncherSocket::setSocket(QLocalSocket *socket)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QTC_ASSERT(!m_socket, return);
    m_socket.store(socket);
    m_packetParser.setDevice(m_socket);
    connect(m_socket,
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
            static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),
#else
            &QLocalSocket::errorOccurred,
#endif
            this, &LauncherSocket::handleSocketError);
    connect(m_socket, &QLocalSocket::readyRead,
            this, &LauncherSocket::handleSocketDataAvailable);
    connect(m_socket, &QLocalSocket::disconnected,
            this, &LauncherSocket::handleSocketDisconnected);
    emit ready();
}

void LauncherSocket::shutdown()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    const auto socket = m_socket.exchange(nullptr);
    if (!socket)
        return;
    socket->disconnect();
    socket->write(ShutdownPacket().serialize());
    socket->waitForBytesWritten(1000);
    socket->deleteLater(); // or schedule a queued call to delete later?
}

void LauncherSocket::handleSocketError()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    auto socket = m_socket.load();
    if (socket->error() != QLocalSocket::PeerClosedError)
        handleError(QCoreApplication::translate("Utils::LauncherSocket",
                    "Socket error: %1").arg(socket->errorString()));
}

void LauncherSocket::handleSocketDataAvailable()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    try {
        if (!m_packetParser.parse())
            return;
    } catch (const PacketParser::InvalidPacketSizeException &e) {
        handleError(QCoreApplication::translate("Utils::LauncherSocket",
                    "Internal protocol error: invalid packet size %1.").arg(e.size));
        return;
    }
    LauncherHandle *handle = handleForToken(m_packetParser.token());
    if (handle) {
        switch (m_packetParser.type()) {
        case LauncherPacketType::ProcessError:
        case LauncherPacketType::ProcessStarted:
        case LauncherPacketType::ReadyReadStandardOutput:
        case LauncherPacketType::ReadyReadStandardError:
        case LauncherPacketType::ProcessFinished:
            handle->handlePacket(m_packetParser.type(), m_packetParser.packetData());
            break;
        default:
            handleError(QCoreApplication::translate("Utils::LauncherSocket",
                                                    "Internal protocol error: invalid packet type %1.")
                        .arg(static_cast<int>(m_packetParser.type())));
            return;
        }
    } else {
//        qDebug() << "No handler for token" << m_packetParser.token() << m_handles;
        // in this case the QtcProcess was canceled and deleted
    }
    handleSocketDataAvailable();
}

void LauncherSocket::handleSocketDisconnected()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    handleError(QCoreApplication::translate("Utils::LauncherSocket",
                "Launcher socket closed unexpectedly"));
}

void LauncherSocket::handleError(const QString &error)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    const auto socket = m_socket.exchange(nullptr);
    socket->disconnect();
    socket->deleteLater();
    emit errorOccurred(error);
}

void LauncherSocket::handleRequests()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    const auto socket = m_socket.load();
    QTC_ASSERT(socket, return);
    QMutexLocker locker(&m_mutex);
    for (const QByteArray &request : qAsConst(m_requests))
        socket->write(request);
    m_requests.clear();
}

bool LauncherSocket::isCalledFromLaunchersThread() const
{
    return QThread::currentThread() == thread();
}

} // namespace Internal
} // namespace Utils

#include "launchersocket.moc"
