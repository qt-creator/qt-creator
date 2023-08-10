// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "launcherpackets.h"
#include "processinterface.h"

#include <QDeadlineTimer>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QWaitCondition>

#include <atomic>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace Utils {
namespace Internal {

class LauncherInterfacePrivate;
class LauncherHandle;
class LauncherSignal;
class LauncherStartedSignal;
class LauncherReadyReadSignal;
class LauncherDoneSignal;

// All the methods and data fields in this class are called / accessed from the caller's thread.
// Exceptions are explicitly marked.
class CallerHandle : public QObject
{
    Q_OBJECT
public:
    enum class SignalType {
        NoSignal,
        Started,
        ReadyRead,
        Done
    };
    Q_ENUM(SignalType)
    CallerHandle(QObject *parent, quintptr token)
        : QObject(parent), m_token(token) {}
    ~CallerHandle() override;

    LauncherHandle *launcherHandle() const { return m_launcherHandle; }
    void setLauncherHandle(LauncherHandle *handle) { QMutexLocker locker(&m_mutex); m_launcherHandle = handle; }

    bool waitForSignal(CallerHandle::SignalType signalType, int msecs);

    // Returns the list of flushed signals.
    void flush();
    bool flushFor(SignalType signalType);
    bool shouldFlush() const;
    // Called from launcher's thread exclusively.
    void appendSignal(LauncherSignal *launcherSignal);

    // Called from caller's or launcher's thread.
    QProcess::ProcessState state() const;
    void sendControlPacket(ControlProcessPacket::SignalType signalType);
    void terminate();
    void kill();
    void close();
    void closeWriteChannel();

    qint64 processId() const;

    void start(const QString &program, const QStringList &arguments);

    qint64 write(const QByteArray &data);

    // Called from caller's or launcher's thread.
    QString program() const;
    // Called from caller's or launcher's thread.
    QStringList arguments() const;
    void setProcessSetupData(ProcessSetupData *setup);

signals:
    void started(qint64 processId, qint64 applicationMainThreadId = 0);
    void readyRead(const QByteArray &outputData, const QByteArray &errorData);
    void done(const Utils::ProcessResultData &resultData);

private:
    // Called from caller's thread exclusively.
    void sendPacket(const Internal::LauncherPacket &packet);
    // Called from caller's or launcher's thread.
    bool isCalledFromCallersThread() const;
    // Called from caller's or launcher's thread. Call me with mutex locked.
    bool isCalledFromLaunchersThread() const;

    QByteArray readAndClear(QByteArray &data) const
    {
        const QByteArray tmp = data;
        data.clear();
        return tmp;
    }

    void handleStarted(const LauncherStartedSignal *launcherSignal);
    void handleReadyRead(const LauncherReadyReadSignal *launcherSignal);
    void handleDone(const LauncherDoneSignal *launcherSignal);

    // Lives in launcher's thread. Modified from caller's thread.
    LauncherHandle *m_launcherHandle = nullptr;

    mutable QMutex m_mutex;
    // Accessed from caller's and launcher's thread
    QList<LauncherSignal *> m_signals;

    const quintptr m_token;

    // Modified from caller's thread, read from launcher's thread
    std::atomic<QProcess::ProcessState> m_processState = QProcess::NotRunning;
    int m_processId = 0;

    QString m_command;
    QStringList m_arguments;
    ProcessSetupData *m_setup = nullptr;
};

// Moved to the launcher thread, returned to caller's thread.
// It's assumed that this object will be alive at least
// as long as the corresponding Process is alive.

class LauncherHandle : public QObject
{
public:
    // Called from caller's thread, moved to launcher's thread afterwards.
    LauncherHandle(quintptr token) : m_token(token) {}
    // Called from caller's thread exclusively.
    bool waitForSignal(CallerHandle::SignalType newSignal, int msecs);
    CallerHandle *callerHandle() const { return m_callerHandle; }
    void setCallerHandle(CallerHandle *handle) { QMutexLocker locker(&m_mutex); m_callerHandle = handle; }

    // Called from launcher's thread exclusively.
    void handleSocketError(const QString &message);
    void handlePacket(LauncherPacketType type, const QByteArray &payload);

    // Called from caller's thread exclusively.
    bool isSocketError() const { return m_socketError; }

private:
    // Called from caller's thread exclusively.
    bool doWaitForSignal(QDeadlineTimer deadline);
    // Called from launcher's thread exclusively. Call me with mutex locked.
    void flushCaller();
    // Called from launcher's thread exclusively.
    void handleStartedPacket(const QByteArray &packetData);
    void handleReadyReadStandardOutput(const QByteArray &packetData);
    void handleReadyReadStandardError(const QByteArray &packetData);
    void handleDonePacket(const QByteArray &packetData);

    // Called from caller's or launcher's thread.
    bool isCalledFromLaunchersThread() const;
    bool isCalledFromCallersThread() const;

    // Lives in caller's thread. Modified only in caller's thread. TODO: check usages - all should be with mutex
    CallerHandle *m_callerHandle = nullptr;

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;
    const quintptr m_token;
    std::atomic_bool m_socketError = false;
};

class LauncherSocket : public QObject
{
    Q_OBJECT
    friend class LauncherInterfacePrivate;
public:
    // Called from caller's thread exclusively.
    void sendData(const QByteArray &data);
    CallerHandle *registerHandle(QObject *parent, quintptr token);
    void unregisterHandle(quintptr token);

signals:
    void errorOccurred(const QString &error);

private:
    // Called from caller's thread, moved to launcher's thread.
    LauncherSocket(QObject *parent = nullptr);
    // Called from launcher's thread exclusively.
    ~LauncherSocket() override;

    // Called from launcher's thread exclusively.
    LauncherHandle *handleForToken(quintptr token) const;

    // Called from launcher's thread exclusively.
    void setSocket(QLocalSocket *socket);
    void shutdown();

    // Called from launcher's thread exclusively.
    void handleSocketError();
    void handleSocketDataAvailable();
    void handleSocketDisconnected();
    void handleError(const QString &error);
    void handleRequests();

    // Called from caller's or launcher's thread.
    bool isCalledFromLaunchersThread() const;

    std::atomic<QLocalSocket *> m_socket{nullptr};
    PacketParser m_packetParser;
    std::vector<QByteArray> m_requests;
    mutable QMutex m_mutex;
    QHash<quintptr, LauncherHandle *> m_handles;
};

} // namespace Internal
} // namespace Utils
