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

#include "algorithm.h"
#include "launchersocket.h"
#include "launcherinterface.h"

#include "qtcassert.h"

#include <QCoreApplication>
#include <QLocalSocket>
#include <QMutexLocker>

#include <iostream>

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

class ErrorSignal : public LauncherSignal
{
public:
    ErrorSignal(QProcess::ProcessError error, const QString &errorString)
        : LauncherSignal(CallerHandle::SignalType::Error)
        , m_error(error)
        , m_errorString(errorString) {}
    QProcess::ProcessError error() const { return m_error; }
    QString errorString() const { return m_errorString; }
private:
    const QProcess::ProcessError m_error;
    const QString m_errorString;
};

class StartedSignal : public LauncherSignal
{
public:
    StartedSignal(int processId)
        : LauncherSignal(CallerHandle::SignalType::Started)
        , m_processId(processId) {}
    int processId() const { return m_processId; }
private:
    const int m_processId;
};

class ReadyReadSignal : public LauncherSignal
{
public:
    ReadyReadSignal(const QByteArray &stdOut, const QByteArray &stdErr)
        : LauncherSignal(CallerHandle::SignalType::ReadyRead)
        , m_stdOut(stdOut)
        , m_stdErr(stdErr) {}
    QByteArray stdOut() const { return m_stdOut; }
    QByteArray stdErr() const { return m_stdErr; }
    void mergeWith(ReadyReadSignal *newSignal) {
        m_stdOut += newSignal->stdOut();
        m_stdErr += newSignal->stdErr();
    }
private:
    QByteArray m_stdOut;
    QByteArray m_stdErr;
};

class FinishedSignal : public LauncherSignal
{
public:
    FinishedSignal(QProcess::ExitStatus exitStatus,
                   int exitCode)
        : LauncherSignal(CallerHandle::SignalType::Finished)
        , m_exitStatus(exitStatus)
        , m_exitCode(exitCode) {}
    QProcess::ExitStatus exitStatus() const { return m_exitStatus; }
    int exitCode() const { return m_exitCode; }
private:
    const QProcess::ExitStatus m_exitStatus;
    const int m_exitCode;
};

CallerHandle::~CallerHandle()
{
    qDeleteAll(m_signals);
}

bool CallerHandle::waitForStarted(int msecs)
{
    return waitForSignal(msecs, SignalType::Started);
}

bool CallerHandle::waitForReadyRead(int msces)
{
    return waitForSignal(msces, SignalType::ReadyRead);
}

bool CallerHandle::waitForFinished(int msecs)
{
    return waitForSignal(msecs, SignalType::Finished);
}

QList<CallerHandle::SignalType> CallerHandle::flush()
{
    return flushFor(SignalType::NoSignal);
}

QList<CallerHandle::SignalType> CallerHandle::flushFor(SignalType signalType)
{
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    QList<LauncherSignal *> oldSignals;
    QList<SignalType> flushedSignals;
    {
        // 1. If signalType is no signal - flush all
        // 2. Flush all if we have any error
        // 3. If we are flushing for Finished or ReadyRead, flush all, too
        // 4. If we are flushing for Started, flush Started only

        // In short: only when we are flushing for Started, flush started only signal,
        // otherwise flush always all. (note: we can't flush for Error, since we don't have
        // waitForError() method).

        QMutexLocker locker(&m_mutex);

        const QList<SignalType> storedSignals =
                Utils::transform(qAsConst(m_signals), [](const LauncherSignal *launcherSignal) {
                                   return launcherSignal->signalType();
        });

        const bool flushAll = (signalType != SignalType::Started)
                           || storedSignals.contains(SignalType::Error);
        if (flushAll) {
            oldSignals = m_signals;
            m_signals = {};
            flushedSignals = storedSignals;
        } else {
            auto matchingIndex = storedSignals.lastIndexOf(signalType);
            if (matchingIndex >= 0) {
                oldSignals = m_signals.mid(0, matchingIndex + 1);
                m_signals = m_signals.mid(matchingIndex + 1);
                flushedSignals = storedSignals.mid(0, matchingIndex + 1);
            }
        }
    }
    for (const LauncherSignal *storedSignal : qAsConst(oldSignals)) {
        const SignalType storedSignalType = storedSignal->signalType();
        switch (storedSignalType) {
        case SignalType::NoSignal:
            break;
        case SignalType::Error:
            handleError(static_cast<const ErrorSignal *>(storedSignal));
            break;
        case SignalType::Started:
            handleStarted(static_cast<const StartedSignal *>(storedSignal));
            break;
        case SignalType::ReadyRead:
            handleReadyRead(static_cast<const ReadyReadSignal *>(storedSignal));
            break;
        case SignalType::Finished:
            handleFinished(static_cast<const FinishedSignal *>(storedSignal));
            break;
        }
        delete storedSignal;
    }
    return flushedSignals;
}

// Called from caller's thread exclusively.
bool CallerHandle::shouldFlush() const
{
    QTC_ASSERT(isCalledFromCallersThread(), return false);
    QMutexLocker locker(&m_mutex);
    return !m_signals.isEmpty();
}

void CallerHandle::handleError(const ErrorSignal *launcherSignal)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_processState = QProcess::NotRunning;
    m_result.m_error = launcherSignal->error();
    if (!launcherSignal->errorString().isEmpty())
        m_result.m_errorString = launcherSignal->errorString();
    if (m_result.m_error == QProcess::FailedToStart)
        m_result.m_exitCode = 255; // This code is being returned by QProcess when FailedToStart error occurred
    emit errorOccurred(m_result.m_error);
}

void CallerHandle::handleStarted(const StartedSignal *launcherSignal)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_processState = QProcess::Running;
    m_processId = launcherSignal->processId();
    emit started();
}

void CallerHandle::handleReadyRead(const ReadyReadSignal *launcherSignal)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    if (m_setup->m_processChannelMode == QProcess::MergedChannels) {
        m_stdout += launcherSignal->stdOut();
        m_stdout += launcherSignal->stdErr();
        if (!m_stdout.isEmpty())
            emit readyReadStandardOutput();
    } else {
        if (m_setup->m_processChannelMode == QProcess::ForwardedOutputChannel
                || m_setup->m_processChannelMode == QProcess::ForwardedChannels) {
            std::cout << launcherSignal->stdOut().constData() << std::flush;
        } else {
            m_stdout += launcherSignal->stdOut();
            if (!m_stdout.isEmpty())
                emit readyReadStandardOutput();
        }
        if (m_setup->m_processChannelMode == QProcess::ForwardedErrorChannel
                || m_setup->m_processChannelMode == QProcess::ForwardedChannels) {
            std::cerr << launcherSignal->stdErr().constData() << std::flush;
        } else {
            m_stderr += launcherSignal->stdErr();
            if (!m_stderr.isEmpty())
                emit readyReadStandardError();
        }
    }
}

void CallerHandle::handleFinished(const FinishedSignal *launcherSignal)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_processState = QProcess::NotRunning;
    m_result.m_exitStatus = launcherSignal->exitStatus();
    m_result.m_exitCode = launcherSignal->exitCode();
    emit finished();
}

// Called from launcher's thread exclusively.
void CallerHandle::appendSignal(LauncherSignal *newSignal)
{
    QTC_ASSERT(!isCalledFromCallersThread(), return);
    QTC_ASSERT(newSignal->signalType() != SignalType::NoSignal, delete newSignal; return);

    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromLaunchersThread(), return);

    // TODO: we might assert if the caller's state is proper, e.g.
    // start signal can't appear if we are in Running or NotRunning state,
    // or finish signal can't appear if we are in NotRunning or Starting state,
    // or readyRead signal can't appear if we are in NotRunning or Starting state,
    // or error signal can't appear if we are in NotRunning state
    // or FailedToStart error signal can't appear if we are in Running state
    // or other than FailedToStart error signal can't appear if we are in Starting state.
    if (!m_signals.isEmpty()) {
        LauncherSignal *lastSignal = m_signals.last();

        QTC_ASSERT(lastSignal->signalType() != SignalType::Finished,
                   qWarning() << "Buffering new signal for process" << m_command
                   << "while the last finished() signal wasn't flushed yet.");

        if (lastSignal->signalType() == SignalType::Error) {
            ErrorSignal *lastError = static_cast<ErrorSignal *>(lastSignal);
            QTC_ASSERT(lastError->error() != QProcess::FailedToStart,
                       qWarning() << "Buffering new signal for process" << m_command
                       << "while the last FailedToStart error signal wasn't flushed yet.");
            QTC_ASSERT(newSignal->signalType() == SignalType::Finished,
                       qWarning() << "Buffering non finished signal for process" << m_command
                       << "while the last buffered signal was an error.");
        }

        // Merge ReadyRead signals into one.
        if (lastSignal->signalType() == SignalType::ReadyRead
                && newSignal->signalType() == SignalType::ReadyRead) {
            ReadyReadSignal *lastRead = static_cast<ReadyReadSignal *>(lastSignal);
            ReadyReadSignal *newRead = static_cast<ReadyReadSignal *>(newSignal);
            lastRead->mergeWith(newRead);
            delete newRead;
            return;
        }
    }
    m_signals.append(newSignal);
}

QProcess::ProcessState CallerHandle::state() const
{
    return m_processState;
}

bool CallerHandle::isStartPacketAwaitingAndClear()
{
    QMutexLocker locker(&m_mutex);
    const bool startPacketExisted = m_startPacket.get();
    m_startPacket.reset();
    return startPacketExisted;
}

void CallerHandle::sendStopPacket(StopProcessPacket::SignalType signalType)
{
    if (m_processState == QProcess::NotRunning)
        return;

    if (m_processState == QProcess::Running || !isStartPacketAwaitingAndClear()) {
        StopProcessPacket packet(m_token);
        packet.signalType = signalType;
        sendPacket(packet);
        return;
    }

    m_processState.store(QProcess::NotRunning);
    m_result.m_errorString = QCoreApplication::translate("Utils::LauncherHandle",
                                                "Process was canceled before it was started.");
    m_result.m_error = QProcess::FailedToStart;
    emit errorOccurred(m_result.m_error);
}


void CallerHandle::terminate()
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    sendStopPacket(StopProcessPacket::SignalType::Terminate);
}

void CallerHandle::kill()
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    sendStopPacket(StopProcessPacket::SignalType::Kill);
}

QByteArray CallerHandle::readAllStandardOutput()
{
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    return readAndClear(m_stdout);
}

QByteArray CallerHandle::readAllStandardError()
{
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    return readAndClear(m_stderr);
}

qint64 CallerHandle::processId() const
{
    QTC_ASSERT(isCalledFromCallersThread(), return 0);
    return m_processId;
}

ProcessResultData CallerHandle::resultData() const
{
    QTC_ASSERT(isCalledFromCallersThread(), return {});
    return m_result;
}

void CallerHandle::setErrorString(const QString &str)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_result.m_errorString = str;
}

void CallerHandle::start(const QString &program, const QStringList &arguments)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    if (!m_launcherHandle || m_launcherHandle->isSocketError()) {
        m_result.m_error = QProcess::FailedToStart;
        emit errorOccurred(m_result.m_error);
        return;
    }

    auto startWhenRunning = [&program, &oldProgram = m_command] {
        qWarning() << "Trying to start" << program << "while" << oldProgram
                   << "is still running for the same QtcProcess instance."
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
    StartProcessPacket *p = new StartProcessPacket(m_token);
    p->command = m_command;
    p->arguments = m_arguments;
    p->env = m_setup->m_environment.toStringList();
    p->workingDir = m_setup->m_workingDirectory.path();
    p->processMode = m_setup->m_processMode;
    p->writeData = m_setup->m_writeData;
    p->standardInputFile = m_setup->m_standardInputFile;
    p->belowNormalPriority = m_setup->m_belowNormalPriority;
    p->nativeArguments = m_setup->m_nativeArguments;
    p->lowPriority = m_setup->m_lowPriority;
    p->unixTerminalDisabled = m_setup->m_unixTerminalDisabled;
    p->useCtrlCStub = m_setup->m_useCtrlCStub;
    m_startPacket.reset(p);
    if (LauncherInterface::isReady())
        doStart();
}

// Called from caller's or launcher's thread.
void CallerHandle::startIfNeeded()
{
    QMutexLocker locker(&m_mutex);
    if (m_processState == QProcess::Starting)
        doStart();
}

// Called from caller's or launcher's thread. Call me with mutex locked.
void CallerHandle::doStart()
{
    if (!m_startPacket)
        return;
    sendPacket(*m_startPacket);
    m_startPacket.reset();
}

// Called from caller's or launcher's thread.
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

void CallerHandle::setProcessSetupData(const ProcessSetupData::Ptr &setup)
{
    QTC_ASSERT(isCalledFromCallersThread(), return);
    m_setup = setup;
}

bool CallerHandle::waitForSignal(int msecs, SignalType newSignal)
{
    QTC_ASSERT(isCalledFromCallersThread(), return false);
    if (!canWaitFor(newSignal))
        return false;
    if (!m_launcherHandle)
        return false;
    return m_launcherHandle->waitForSignal(msecs, newSignal);
}

bool CallerHandle::canWaitFor(SignalType newSignal) const
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
bool LauncherHandle::waitForSignal(int msecs, CallerHandle::SignalType newSignal)
{
    QTC_ASSERT(!isCalledFromLaunchersThread(), return false);
    QDeadlineTimer deadline(msecs);
    while (true) {
        if (deadline.hasExpired())
            break;
        if (!doWaitForSignal(deadline, newSignal))
            break;
        const QList<CallerHandle::SignalType> flushedSignals = m_callerHandle->flushFor(newSignal);
        const bool errorOccurred = flushedSignals.contains(CallerHandle::SignalType::Error);
        if (errorOccurred)
            return true; // apparently QProcess behaves like this in case of error
        const bool newSignalFlushed = flushedSignals.contains(newSignal);
        if (newSignalFlushed) // so we don't continue waiting
            return true;
        const bool finishedSignalFlushed = flushedSignals.contains(CallerHandle::SignalType::Finished);
        if (finishedSignalFlushed)
            return true; // finish has appeared but we were waiting for other signal
    }
    return false;
}

// Called from caller's thread exclusively.
bool LauncherHandle::doWaitForSignal(QDeadlineTimer deadline, CallerHandle::SignalType newSignal)
{
    QMutexLocker locker(&m_mutex);
    QTC_ASSERT(isCalledFromCallersThread(), return false);
    QTC_ASSERT(m_waitingFor == CallerHandle::SignalType::NoSignal, return false);

    // Flush, if we have any stored signals.
    // This must be called when holding laucher's mutex locked prior to the call to wait,
    // so that it's done atomically.
    if (m_callerHandle->shouldFlush())
        return true;

    m_waitingFor = newSignal;
    const bool ret = m_waitCondition.wait(&m_mutex, deadline);
    m_waitingFor = CallerHandle::SignalType::NoSignal;
    return ret;
}

// Called from launcher's thread exclusively. Call me with mutex locked.
void LauncherHandle::wakeUpIfWaitingFor(CallerHandle::SignalType newSignal)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    // TODO: should we always wake up in case m_waitingFor != NoSignal?
    // The matching signal came
    const bool signalMatched = (m_waitingFor == newSignal);
    // E.g. if we are waiting for ReadyRead and we got Finished or Error signal instead -> wake it, too.
    const bool finishedOrErrorWhileWaiting =
            (m_waitingFor != CallerHandle::SignalType::NoSignal)
            && ((newSignal == CallerHandle::SignalType::Finished) || (newSignal == CallerHandle::SignalType::Error));
    // Wake up, flush and continue waiting.
    // E.g. when being in waitingForFinished() state and Started or ReadyRead signal came.
    const bool continueWaitingAfterFlushing =
            ((m_waitingFor == CallerHandle::SignalType::Finished) && (newSignal != CallerHandle::SignalType::Finished))
            || ((m_waitingFor == CallerHandle::SignalType::ReadyRead) && (newSignal == CallerHandle::SignalType::Started));
    const bool shouldWake = signalMatched
                         || finishedOrErrorWhileWaiting
                         || continueWaitingAfterFlushing;

    if (shouldWake)
        m_waitCondition.wakeOne();
}

// Called from launcher's thread exclusively. Call me with mutex locked.
void LauncherHandle::flushCaller()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    if (!m_callerHandle)
        return;

    // call in callers thread
    QMetaObject::invokeMethod(m_callerHandle, &CallerHandle::flush);
}

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
    wakeUpIfWaitingFor(CallerHandle::SignalType::Error);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ProcessErrorPacket>(m_token, packetData);
    m_callerHandle->appendSignal(new ErrorSignal(packet.error, packet.errorString));
    flushCaller();
}

void LauncherHandle::handleStartedPacket(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(CallerHandle::SignalType::Started);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ProcessStartedPacket>(m_token, packetData);
    m_callerHandle->appendSignal(new StartedSignal(packet.processId));
    flushCaller();
}

void LauncherHandle::handleReadyReadStandardOutput(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(CallerHandle::SignalType::ReadyRead);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ReadyReadStandardOutputPacket>(m_token, packetData);
    if (packet.standardChannel.isEmpty())
        return;

    m_callerHandle->appendSignal(new ReadyReadSignal(packet.standardChannel, {}));
    flushCaller();
}

void LauncherHandle::handleReadyReadStandardError(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(CallerHandle::SignalType::ReadyRead);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ReadyReadStandardErrorPacket>(m_token, packetData);
    if (packet.standardChannel.isEmpty())
        return;

    m_callerHandle->appendSignal(new ReadyReadSignal({}, packet.standardChannel));
    flushCaller();
}

void LauncherHandle::handleFinishedPacket(const QByteArray &packetData)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(CallerHandle::SignalType::Finished);
    if (!m_callerHandle)
        return;

    const auto packet = LauncherPacket::extractPacket<ProcessFinishedPacket>(m_token, packetData);
    const QByteArray stdOut = packet.stdOut;
    const QByteArray stdErr = packet.stdErr;
    const QProcess::ProcessError error = packet.error;
    const QString errorString = packet.errorString;

    // We assume that if error is UnknownError, everything went fine.
    // By default QProcess returns "Unknown error" for errorString()
    if (error != QProcess::UnknownError)
        m_callerHandle->appendSignal(new ErrorSignal(error, errorString));
    if (!stdOut.isEmpty() || !stdErr.isEmpty())
        m_callerHandle->appendSignal(new ReadyReadSignal(stdOut, stdErr));
    m_callerHandle->appendSignal(new FinishedSignal(packet.exitStatus, packet.exitCode));
    flushCaller();
}

void LauncherHandle::handleSocketReady()
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    m_socketError = false;
    QMutexLocker locker(&m_mutex);
    if (m_callerHandle)
        m_callerHandle->startIfNeeded();
}

void LauncherHandle::handleSocketError(const QString &message)
{
    QTC_ASSERT(isCalledFromLaunchersThread(), return);
    m_socketError = true; // TODO: ???
    QMutexLocker locker(&m_mutex);
    wakeUpIfWaitingFor(CallerHandle::SignalType::Error);
    if (!m_callerHandle)
        return;

    const QString errorString = QCoreApplication::translate("Utils::QtcProcess",
                                "Internal socket error: %1").arg(message);
    m_callerHandle->appendSignal(new ErrorSignal(QProcess::FailedToStart, errorString));
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
    connect(this, &LauncherSocket::ready,
            launcherHandle, &LauncherHandle::handleSocketReady);
    connect(this, &LauncherSocket::errorOccurred,
            launcherHandle, &LauncherHandle::handleSocketError);

    return callerHandle;
}

void LauncherSocket::unregisterHandle(quintptr token)
{
    QTC_ASSERT(!isCalledFromLaunchersThread(), return);
    QMutexLocker locker(&m_mutex);
    auto it = m_handles.find(token);
    if (it == m_handles.end())
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
    connect(m_socket,
            &QLocalSocket::errorOccurred,
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
                "Launcher socket closed unexpectedly."));
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
