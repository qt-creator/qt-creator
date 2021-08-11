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
#include <QtNetwork/qlocalsocket.h>

namespace Utils {
namespace Internal {

class CallerHandle : public QObject
{
    Q_OBJECT
public:
    CallerHandle() : QObject() {}

    // always called in caller's thread
    void flush()
    {
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
    }
    void appendSignal(LauncherHandle::SignalType signalType)
    {
        if (signalType == LauncherHandle::SignalType::NoSignal)
            return;

        QMutexLocker locker(&m_mutex);
        if (m_signals.contains(signalType))
            return;

        m_signals.append(signalType);
    }
    bool shouldFlushFor(LauncherHandle::SignalType signalType)
    {
        QMutexLocker locker(&m_mutex);
        if (m_signals.contains(signalType))
            return true;
        if (m_signals.contains(LauncherHandle::SignalType::Finished))
            return true;
        return false;
    }
signals:
    void started();
    void readyRead();
    void finished();
private:
    QMutex m_mutex;
    QList<LauncherHandle::SignalType> m_signals;
};

void LauncherHandle::handlePacket(LauncherPacketType type, const QByteArray &payload)
{
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
    QMutexLocker locker(&m_mutex);
    if (m_waitingFor != SignalType::NoSignal) {
        m_waitCondition.wakeOne();
        m_waitingFor = SignalType::NoSignal;
    }
    m_failed = true;

    const auto packet = LauncherPacket::extractPacket<ProcessErrorPacket>(m_token, packetData);
    m_error = packet.error;
    m_errorString = packet.errorString;
    // TODO: pipe it through the callers handle?
    emit errorOccurred(m_error);
}

// call me with mutex locked
void LauncherHandle::wakeUpIfWaitingFor(SignalType wakeUpSignal)
{
    // e.g. if we are waiting for ReadyRead and we got Finished signal instead -> wake it, too.
    const bool shouldWake = (m_waitingFor == wakeUpSignal)
            || ((m_waitingFor != SignalType::NoSignal) && (wakeUpSignal == SignalType::Finished));
    if (shouldWake) {
        m_waitCondition.wakeOne();
        m_waitingFor = SignalType::NoSignal;
    }
}

void LauncherHandle::sendPacket(const Internal::LauncherPacket &packet)
{
    LauncherInterface::socket()->sendData(packet.serialize());
}

// call me with mutex locked
void LauncherHandle::flushCaller()
{
    if (!m_callerHandle)
        return;

    // call in callers thread
    QMetaObject::invokeMethod(m_callerHandle, &CallerHandle::flush);
}

void LauncherHandle::handleStartedPacket(const QByteArray &packetData)
{
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
    QMutexLocker locker(&m_mutex);
    m_socketError = false;
    if (m_processState == QProcess::Starting)
        doStart();
}

void LauncherHandle::handleSocketError(const QString &message)
{
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
    const bool ok = doWaitForSignal(msecs, newSignal);
    if (ok)
        m_callerHandle->flush();
    return ok;
}

bool LauncherHandle::doWaitForSignal(int msecs, SignalType newSignal)
{
    QMutexLocker locker(&m_mutex);
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
        return m_waitCondition.wait(&m_mutex, msecs) && !m_failed;
    }

    return false;
}

// call me with mutex locked
bool LauncherHandle::canWaitFor(SignalType newSignal) const
{
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

void LauncherHandle::cancel()
{
    QMutexLocker locker(&m_mutex);

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
}

void LauncherHandle::start(const QString &program, const QStringList &arguments, const QByteArray &writeData)
{
    QMutexLocker locker(&m_mutex);

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

    if (m_processState != QProcess::Running)
        return -1;

    WritePacket p(m_token);
    p.inputData = data;
    sendPacket(p);
    return data.size();
}

// Ensure it's called from caller's thread, after moving LauncherHandle into the launcher's thread
void LauncherHandle::createCallerHandle()
{
    QMutexLocker locker(&m_mutex); // may be not needed, as we call it just after Launcher's c'tor
    QTC_CHECK(m_callerHandle == nullptr);
    m_callerHandle = new CallerHandle();
    connect(m_callerHandle, &CallerHandle::started, this, &LauncherHandle::slotStarted, Qt::DirectConnection);
    connect(m_callerHandle, &CallerHandle::readyRead, this, &LauncherHandle::slotReadyRead, Qt::DirectConnection);
    connect(m_callerHandle, &CallerHandle::finished, this, &LauncherHandle::slotFinished, Qt::DirectConnection);
}

void LauncherHandle::destroyCallerHandle()
{
    QMutexLocker locker(&m_mutex);
    QTC_CHECK(m_callerHandle);
    m_callerHandle->deleteLater();
    m_callerHandle = nullptr;
}

void LauncherHandle::slotStarted()
{
    emit started();
}

void LauncherHandle::slotReadyRead()
{
    bool hasOutput = false;
    bool hasError = false;
    {
        QMutexLocker locker(&m_mutex);
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
    QMutexLocker locker(&m_mutex);
    m_requests.push_back(data);
    if (m_requests.size() == 1)
        QMetaObject::invokeMethod(this, &LauncherSocket::handleRequests);
}

LauncherHandle *LauncherSocket::registerHandle(quintptr token, ProcessMode mode)
{
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
    QMutexLocker locker(&m_mutex);
    return m_handles.value(token);
}

void LauncherSocket::shutdown()
{
    const auto socket = m_socket.exchange(nullptr);
    if (!socket)
        return;
    socket->disconnect();
    socket->write(ShutdownPacket().serialize());
    socket->waitForBytesWritten(1000);
    socket->deleteLater(); // or schedule a queued call to delete later?
}

void LauncherSocket::setSocket(QLocalSocket *socket)
{
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

void LauncherSocket::handleSocketError()
{
    auto socket = m_socket.load();
    if (socket->error() != QLocalSocket::PeerClosedError)
        handleError(QCoreApplication::translate("Utils::LauncherSocket",
                    "Socket error: %1").arg(socket->errorString()));
}

void LauncherSocket::handleSocketDataAvailable()
{
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
    handleError(QCoreApplication::translate("Utils::LauncherSocket",
                "Launcher socket closed unexpectedly"));
}

void LauncherSocket::handleError(const QString &error)
{
    const auto socket = m_socket.exchange(nullptr);
    socket->disconnect();
    socket->deleteLater();
    emit errorOccurred(error);
}

void LauncherSocket::handleRequests()
{
    const auto socket = m_socket.load();
    QTC_ASSERT(socket, return);
    QMutexLocker locker(&m_mutex);
    for (const QByteArray &request : qAsConst(m_requests))
        socket->write(request);
    m_requests.clear();
}

} // namespace Internal
} // namespace Utils

#include "launchersocket.moc"
