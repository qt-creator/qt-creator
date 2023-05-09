// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "launchersocket.h"

#include "algorithm.h"
#include "launcherinterface.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QLocalSocket>
#include <QMutexLocker>

namespace Utils {
namespace Internal {

class LauncherSignal
{
public:
    CallerHandle::SignalType signalType() const { return m_signalType; }
    virtual ~LauncherSignal() = default;
protected:
    LauncherSignal(CallerHandle::SignalType signalType) : m_signalType(signalType) {}
private:
    const CallerHandle::SignalType m_signalType;
};

class LauncherStartedSignal : public LauncherSignal
{
public:
    LauncherStartedSignal(int processId)
        : LauncherSignal(CallerHandle::SignalType::Started)
        , m_processId(processId) {}
    int processId() const { return m_processId; }
private:
    const int m_processId;
};

class LauncherReadyReadSignal : public LauncherSignal
{
public:
    LauncherReadyReadSignal(const QByteArray &stdOut, const QByteArray &stdErr)
        : LauncherSignal(CallerHandle::SignalType::ReadyRead)
        , m_stdOut(stdOut)
        , m_stdErr(stdErr) {}
    QByteArray stdOut() const { return m_stdOut; }
    QByteArray stdErr() const { return m_stdErr; }

private:
    QByteArray m_stdOut;
    QByteArray m_stdErr;
};

class LauncherDoneSignal : public LauncherSignal
{
public:
    LauncherDoneSignal(const ProcessResultData &resultData)
        : LauncherSignal(CallerHandle::SignalType::Done)
        , m_resultData(resultData) {}
    ProcessResultData resultData() const { return m_resultData; }
private:
    const ProcessResultData m_resultData;
};

CallerHandle::~CallerHandle()
{
    qDeleteAll(m_signals);
}

void CallerHandle::flush()
{
    flushFor(SignalType::NoSignal);
}

bool CallerHandle::flushFor(SignalType signalType)
{
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    QList<LauncherSignal *> oldSignals;
    {
        QMutexLocker locker(&m_mutex);
        const QList<SignalType> storedSignals =
                Utils::transform(std::as_const(m_signals), [](const LauncherSignal *launcherSignal) {
                                   return launcherSignal->signalType();
        });

        // If we are flushing for ReadyRead or Done - flush all.
        // If we are flushing for Started:
        // - if Started was buffered - flush Started only.
        // - otherwise if Done signal was buffered - flush all.
        const bool flushAll = (signalType != SignalType::Started)
                || (!storedSignals.contains(SignalType::Started)
                    && storedSignals.contains(SignalType::Done));
        if (flushAll) {
            oldSignals = m_signals;
            m_signals = {};
        } else {
            auto matchingIndex = storedSignals.lastIndexOf(signalType);
            if (matchingIndex >= 0) {
                oldSignals = m_signals.mid(0, matchingIndex + 1);
                m_signals = m_signals.mid(matchingIndex + 1);
            }
        }
    }
    bool signalMatched = false;
    for (const LauncherSignal *storedSignal : std::as_const(oldSignals)) {
        const SignalType storedSignalType = storedSignal->signalType();
        if (storedSignalType == signalType)
            signalMatched = true;
        switch (storedSignalType) {
        case SignalType::NoSignal:
            break;
        case SignalType::Started:
            handleStarted(static_cast<const LauncherStartedSignal *>(storedSignal));
            break;
        case SignalType::ReadyRead:
            handleReadyRead(static_cast<const LauncherReadyReadSignal *>(storedSignal));
            break;
        case SignalType::Done:
            signalMatched = true;
            handleDone(static_cast<const LauncherDoneSignal *>(storedSignal));
            break;
        }
        delete storedSignal;
    }
    return signalMatched;
}

// Called from caller's thread exclusively.
bool CallerHandle::shouldFlush() const
{
    QTC_ASSERT(isCalledFromCallersThread(), return false);
    QMutexLocker locker(&m_mutex);
    return !m_signals.isEmpty();
}

void CallerHandle::handleStarted(const LauncherStartedSignal *launcherSignal)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_processState = QProcess::Running;
    m_processId = launcherSignal->processId();
    emit started(m_processId);
}

void CallerHandle::handleReadyRead(const LauncherReadyReadSignal *launcherSignal)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    emit readyRead(launcherSignal->stdOut(), launcherSignal->stdErr());
}

void CallerHandle::handleDone(const LauncherDoneSignal *launcherSignal)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_processState = QProcess::NotRunning;
    emit done(launcherSignal->resultData());
    m_processId = 0;
}

// Called from launcher's thread exclusively.
void CallerHandle::appendSignal(LauncherSignal *newSignal)
{
    QTC_ASSERT(!isCalledFromCallersThread(), return);
    QTC_ASSERT(newSignal->signalType() != SignalType::NoSignal, delete newSignal; return);

    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    m_signals.append(newSignal);
}

QProcess::ProcessState CallerHandle::state() const
{
    return m_processState;
}

void CallerHandle::sendControlPacket(ControlProcessPacket::SignalType signalType)
{
    if (m_processState == QProcess::NotRunning)
        return;

    // TODO: In case m_processState == QProcess::Starting and the launcher socket isn't ready yet
    // we might want to remove posted start packet and finish the process immediately.
    // In addition, we may always try to check if correspodning start packet for the m_token
    // is still awaiting and do the same (remove the packet from the stack and finish immediately).
    ControlProcessPacket packet(m_token);
    packet.signalType = signalType;
    sendPacket(packet);
}

void CallerHandle::terminate()
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    sendControlPacket(ControlProcessPacket::SignalType::Terminate);
}

void CallerHandle::kill()
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    sendControlPacket(ControlProcessPacket::SignalType::Kill);
}

void CallerHandle::close()
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    sendControlPacket(ControlProcessPacket::SignalType::Close);
}

void CallerHandle::closeWriteChannel()
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    sendControlPacket(ControlProcessPacket::SignalType::CloseWriteChannel);
}

qint64 CallerHandle::processId() const
{
    QTC_ASSERT(isCalledFromCallersThread(), return 0);
    return m_processId;
}

void CallerHandle::start(const QString &program, const QStringList &arguments)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    if (!m_launcherHandle || m_launcherHandle->isSocketError()) {
        const QString errorString = Tr::tr("Process launcher socket error.");
        const ProcessResultData result = { 0, QProcess::NormalExit, QProcess::FailedToStart,
                                           errorString };
        emit done(result);
        return;
    }

    auto startWhenRunning = [&program, &oldProgram = m_command] {
        qWarning() << "Trying to start" << program << "while" << oldProgram
                   << "is still running for the same Process instance."
                   << "The current call will be ignored.";
    };
    QTC_ASSERT(m_processState == QProcess::NotRunning, startWhenRunning(); return);

    auto processLauncherNotStarted = [&program] {
        qWarning() << "Trying to start" << program << "while process launcher wasn't started yet.";
    };
    QTC_ASSERT(LauncherInterface::isStarted(), processLauncherNotStarted());

    QMutexLocker locker(&m_mutex);
    m_command = program;
    m_arguments = arguments;
    m_processState = QProcess::Starting;
    StartProcessPacket p(m_token);
    p.command = m_command;
    p.arguments = m_arguments;
    p.env = m_setup->m_environment.toStringList();
    if (p.env.isEmpty())
        p.env = Environment::systemEnvironment().toStringList();
    p.workingDir = m_setup->m_workingDirectory.path();
    p.processMode = m_setup->m_processMode;
    p.writeData = m_setup->m_writeData;
    p.processChannelMode = m_setup->m_processChannelMode;
    p.standardInputFile = m_setup->m_standardInputFile;
    p.belowNormalPriority = m_setup->m_belowNormalPriority;
    p.nativeArguments = m_setup->m_nativeArguments;
    p.lowPriority = m_setup->m_lowPriority;
    p.unixTerminalDisabled = m_setup->m_unixTerminalDisabled;
    p.useCtrlCStub = m_setup->m_useCtrlCStub;
    p.reaperTimeout = m_setup->m_reaperTimeout;
    p.createConsoleOnWindows = m_setup->m_createConsoleOnWindows;
    sendPacket(p);
}

// Called from caller's thread exclusively.
void CallerHandle::sendPacket(const Internal::LauncherPacket &packet)
{
    LauncherInterface::sendData(packet.serialize());
}

qint64 CallerHandle::write(const QByteArray &data)
{
    QTC_ASSERT(isCalledFromCallersThread(), return -1);

    if (m_processState != QProcess::Running)
        return -1;

    WritePacket p(m_token);
    p.inputData = data;
    sendPacket(p);
    return data.size();
}

QString CallerHandle::program() const
{
    QMutexLocker locker(&m_mutex);
    return m_command;
}

QStringList CallerHandle::arguments() const
{
    QMutexLocker locker(&m_mutex);
    return m_arguments;
}

void CallerHandle::setProcessSetupData(ProcessSetupData *setup)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_setup = setup;
}

bool CallerHandle::waitForSignal(SignalType signalType, int msecs)
{
    QTC_ASSERT(isCalledFromCallersThread(), return false);
    QTC_ASSERT(m_launcherHandle, return false);
    return m_launcherHandle->waitForSignal(signalType, msecs);
}

// Called from caller's or launcher's thread.
bool CallerHandle::isCalledFromCallersThread() const
{
    return QThread::currentThread() == thread();
}

// Called from caller's or launcher's thread. Call me with mutex locked.
bool CallerHandle::isCalledFromLaunchersThread() const
{
    if (!m_launcherHandle)
        return false;
    return QThread::currentThread() == m_launcherHandle->thread();
}

// Called from caller's thread exclusively.
bool LauncherHandle::waitForSignal(CallerHandle::SignalType newSignal, int msecs)
{
    QTC_ASSERT(!isCalledFromLaunchersThread(), return false);
    QDeadlineTimer deadline(msecs);
    while (true) {
        if (deadline.hasExpired())
            break;
        if (!doWaitForSignal(deadline))
            break;
        // Matching (or Done) signal was flushed
        if (m_callerHandle->flushFor(newSignal))
            return true;
        // Otherwise continue awaiting (e.g. when ReadyRead came while waitForFinished())
    }
    return false;
}

// Called from caller's thread exclusively.
bool LauncherHandle::doWaitForSignal(QDeadlineTimer deadline)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return false);

    // Flush, if we have any stored signals.
    // This must be called when holding laucher's mutex locked prior to the call to wait,
    // so that it's done atomically.
    if (m_callerHandle->shouldFlush())
        return true;

    return m_waitCondition.wait(&m_mutex, deadline);
}

// Called from launcher's thread exclusively. Call me with mutex locked.
void LauncherHandle::flushCaller()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    if (!m_callerHandle)
        return;

    m_waitCondition.wakeOne();

    // call in callers thread
    QMetaObject::invokeMethod(m_callerHandle, &CallerHandle::flush);
}

void LauncherHandle::handlePacket(LauncherPacketType type, const QByteArray &payload)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    switch (type) {
    case LauncherPacketType::ProcessStarted:
        handleStartedPacket(payload);
        break;
    case LauncherPacketType::ReadyReadStandardOutput:
        handleReadyReadStandardOutput(payload);
        break;
    case LauncherPacketType::ReadyReadStandardError:
        handleReadyReadStandardError(payload);
        break;
    case LauncherPacketType::ProcessDone:
        handleDonePacket(payload);
        break;
    default:
        QTC_ASSERT(false, break);
    }
}

void LauncherHandle::handleStartedPacket(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ProcessStartedPacket>(m_token, packetData);
    m_callerHandle->appendSignal(new LauncherStartedSignal(packet.processId));
    flushCaller();
}

void LauncherHandle::handleReadyReadStandardOutput(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ReadyReadStandardOutputPacket>(m_token, packetData);
    if (packet.standardChannel.isEmpty())
        return;

    m_callerHandle->appendSignal(new LauncherReadyReadSignal(packet.standardChannel, {}));
    flushCaller();
}

void LauncherHandle::handleReadyReadStandardError(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ReadyReadStandardErrorPacket>(m_token, packetData);
    if (packet.standardChannel.isEmpty())
        return;

    m_callerHandle->appendSignal(new LauncherReadyReadSignal({}, packet.standardChannel));
    flushCaller();
}

void LauncherHandle::handleDonePacket(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ProcessDonePacket>(m_token, packetData);
    const QByteArray stdOut = packet.stdOut;
    const QByteArray stdErr = packet.stdErr;
    const ProcessResultData result = { packet.exitCode, packet.exitStatus,
                                       packet.error, packet.errorString };

    if (!stdOut.isEmpty() || !stdErr.isEmpty())
        m_callerHandle->appendSignal(new LauncherReadyReadSignal(stdOut, stdErr));
    m_callerHandle->appendSignal(new LauncherDoneSignal(result));
    flushCaller();
}

void LauncherHandle::handleSocketError(const QString &message)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    m_socketError = true; // TODO: ???
    QMutexLocker locker(&m_mutex);
    if (!m_callerHandle)
        return;

    // TODO: FailedToStart may be wrong in case process has already started
    const QString errorString = Tr::tr("Internal socket error: %1").arg(message);
    const ProcessResultData result = { 0, QProcess::NormalExit, QProcess::FailedToStart,
                                       errorString };
    m_callerHandle->appendSignal(new LauncherDoneSignal(result));
    flushCaller();
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

LauncherSocket::LauncherSocket(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<Utils::Internal::LauncherPacketType>();
    qRegisterMetaType<quintptr>("quintptr");
}

LauncherSocket::~LauncherSocket()
{
    QMutexLocker locker(&m_mutex);
    auto displayHandles = [&handles = m_handles] {
        qWarning() << "Destroying process launcher while" << handles.count()
                   << "processes are still alive. The following processes are still alive:";
        for (LauncherHandle *handle : handles) {
            CallerHandle *callerHandle = handle->callerHandle();
            if (callerHandle->state() != QProcess::NotRunning) {
                qWarning() << "  " << callerHandle->program() << callerHandle->arguments()
                       << "in thread" << (void *)callerHandle->thread();
            } else {
                qWarning() << "  Not running process in thread" << (void *)callerHandle->thread();
            }
        }
    };
    QTC_ASSERT(m_handles.isEmpty(), displayHandles());
}

void LauncherSocket::sendData(const QByteArray &data)
{
    auto storeRequest = [this](const QByteArray &data)
    {
        QMutexLocker locker(&m_mutex);
        m_requests.push_back(data);
        return m_requests.size() == 1; // Returns true if requests handling should be triggered.
    };

    if (storeRequest(data)) // Call handleRequests() in launcher's thread.
        QMetaObject::invokeMethod(this, &LauncherSocket::handleRequests);
}

CallerHandle *LauncherSocket::registerHandle(QObject *parent, quintptr token)
{
    QTC_ASSERT(!isCalledFromLaunchersThread(), return nullptr);
    QMutexLocker locker(&m_mutex);
    if (m_handles.contains(token))
        return nullptr; // TODO: issue a warning

    CallerHandle *callerHandle = new CallerHandle(parent, token);
    LauncherHandle *launcherHandle = new LauncherHandle(token);
    callerHandle->setLauncherHandle(launcherHandle);
    launcherHandle->setCallerHandle(callerHandle);
    launcherHandle->moveToThread(thread());
    // Call it after moving LauncherHandle to the launcher's thread.
    // Since this method is invoked from caller's thread, CallerHandle will live in caller's thread.
    m_handles.insert(token, launcherHandle);
    connect(this, &LauncherSocket::errorOccurred,
            launcherHandle, &LauncherHandle::handleSocketError);

    return callerHandle;
}

void LauncherSocket::unregisterHandle(quintptr token)
{
    QTC_ASSERT(!isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    auto it = m_handles.constFind(token);
    if (it == m_handles.constEnd())
        return; // TODO: issue a warning

    LauncherHandle *launcherHandle = it.value();
    CallerHandle *callerHandle = launcherHandle->callerHandle();
    launcherHandle->setCallerHandle(nullptr);
    callerHandle->setLauncherHandle(nullptr);
    launcherHandle->deleteLater();
    callerHandle->deleteLater();
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
    connect(m_socket, &QLocalSocket::errorOccurred,
            this, &LauncherSocket::handleSocketError);
    connect(m_socket, &QLocalSocket::readyRead,
            this, &LauncherSocket::handleSocketDataAvailable);
    connect(m_socket, &QLocalSocket::disconnected,
            this, &LauncherSocket::handleSocketDisconnected);
    handleRequests();
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
        handleError(Tr::tr("Socket error: %1").arg(socket->errorString()));
}

void LauncherSocket::handleSocketDataAvailable()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    try {
        if (!m_packetParser.parse())
            return;
    } catch (const PacketParser::InvalidPacketSizeException &e) {
        handleError(Tr::tr("Internal protocol error: invalid packet size %1.").arg(e.size));
        return;
    }
    LauncherHandle *handle = handleForToken(m_packetParser.token());
    if (handle) {
        switch (m_packetParser.type()) {
        case LauncherPacketType::ProcessStarted:
        case LauncherPacketType::ReadyReadStandardOutput:
        case LauncherPacketType::ReadyReadStandardError:
        case LauncherPacketType::ProcessDone:
            handle->handlePacket(m_packetParser.type(), m_packetParser.packetData());
            break;
        default:
            handleError(Tr::tr("Internal protocol error: invalid packet type %1.")
                        .arg(static_cast<int>(m_packetParser.type())));
            return;
        }
    } else {
//        qDebug() << "No handler for token" << m_packetParser.token() << m_handles;
        // in this case the Process was canceled and deleted
    }
    handleSocketDataAvailable();
}

void LauncherSocket::handleSocketDisconnected()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    handleError(Tr::tr("Launcher socket closed unexpectedly."));
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
    if (!socket)
        return;

    std::vector<QByteArray> requests;
    {
        QMutexLocker locker(&m_mutex);
        requests = m_requests;
        m_requests.clear();
    }

    for (const QByteArray &request : std::as_const(requests))
        socket->write(request);
}

bool LauncherSocket::isCalledFromLaunchersThread() const
{
    return QThread::currentThread() == thread();
}

} // namespace Internal
} // namespace Utils
