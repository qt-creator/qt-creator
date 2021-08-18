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

#pragma once

#include "launcherpackets.h"
#include "processutils.h"

#include <QtCore/qobject.h>

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QProcess>
#include <QWaitCondition>

#include <vector>
#include <atomic>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace Utils {
class LauncherInterface;

namespace Internal {

class LauncherInterfacePrivate;
class CallerHandle;

// Moved to the launcher thread, returned to caller's thread.
// It's assumed that this object will be alive at least
// as long as the corresponding QtcProcess is alive.

class LauncherHandle : public QObject
{
    Q_OBJECT
public:
    // All the public methods in this class are called exclusively from the caller's thread.
    bool waitForStarted(int msecs)
    { return waitForSignal(msecs, SignalType::Started); }
    bool waitForReadyRead(int msces)
    { return waitForSignal(msces, SignalType::ReadyRead); }
    bool waitForFinished(int msecs)
    { return waitForSignal(msecs, SignalType::Finished); }

    QProcess::ProcessState state() const
    { QMutexLocker locker(&m_mutex); return m_processState; }
    void cancel();

    QByteArray readAllStandardOutput()
    { QMutexLocker locker(&m_mutex); return readAndClear(m_stdout); }
    QByteArray readAllStandardError()
    { QMutexLocker locker(&m_mutex); return readAndClear(m_stderr); }

    qint64 processId() const { QMutexLocker locker(&m_mutex); return m_processId; }
    QString errorString() const { QMutexLocker locker(&m_mutex); return m_errorString; }
    void setErrorString(const QString &str) { QMutexLocker locker(&m_mutex); m_errorString = str; }

    void start(const QString &program, const QStringList &arguments, const QByteArray &writeData);

    qint64 write(const QByteArray &data);

    QProcess::ProcessError error() const { QMutexLocker locker(&m_mutex); return m_error; }
    QString program() const { QMutexLocker locker(&m_mutex); return m_command; }
    void setStandardInputFile(const QString &fileName) { QMutexLocker locker(&m_mutex); m_standardInputFile = fileName; }
    void setProcessChannelMode(QProcess::ProcessChannelMode mode) {
        QMutexLocker locker(&m_mutex);
        if (mode != QProcess::SeparateChannels && mode != QProcess::MergedChannels) {
            qWarning("setProcessChannelMode: The only supported modes are SeparateChannels and MergedChannels.");
            return;
        }
        m_channelMode = mode;
    }
    void setProcessEnvironment(const QProcessEnvironment &environment)
    { QMutexLocker locker(&m_mutex); m_environment = environment; }
    void setWorkingDirectory(const QString &dir) { QMutexLocker locker(&m_mutex); m_workingDirectory = dir; }
    QProcess::ExitStatus exitStatus() const { QMutexLocker locker(&m_mutex); return m_exitStatus; }

    void setBelowNormalPriority() { m_belowNormalPriority = true; }
    void setNativeArguments(const QString &arguments) { m_nativeArguments = arguments; }
    void setLowPriority() { m_lowPriority = true; }
    void setUnixTerminalDisabled() { m_unixTerminalDisabled = true; }

signals:
    void errorOccurred(QProcess::ProcessError error);
    void started();
    void finished(int exitCode, QProcess::ExitStatus status);
    void readyReadStandardOutput();
    void readyReadStandardError();
private:
    enum class SignalType {
        NoSignal,
        Error,
        Started,
        ReadyRead,
        Finished
    };

    // Called from caller's thread exclusively.
    bool waitForSignal(int msecs, SignalType newSignal);
    bool doWaitForSignal(int msecs, SignalType newSignal);
    bool canWaitFor(SignalType newSignal) const;

    // Called from caller's or launcher's thread.
    void doStart();

    // Called from caller's thread exclusively.
    void slotErrorOccurred();
    void slotStarted();
    void slotReadyRead();
    void slotFinished();

    // Called from caller's thread, moved to launcher's thread.
    LauncherHandle(quintptr token, ProcessMode mode) : m_token(token), m_processMode(mode) {}

    // Called from caller's thread exclusively.
    void createCallerHandle();
    void destroyCallerHandle();

    // Called from launcher's thread exclusively.
    void flushCaller();
    void handlePacket(LauncherPacketType type, const QByteArray &payload);
    void handleErrorPacket(const QByteArray &packetData);
    void handleStartedPacket(const QByteArray &packetData);
    void handleReadyReadStandardOutput(const QByteArray &packetData);
    void handleReadyReadStandardError(const QByteArray &packetData);
    void handleFinishedPacket(const QByteArray &packetData);

    // Called from launcher's thread exclusively.
    void handleSocketReady();
    void handleSocketError(const QString &message);

    // Called from launcher's thread exclusively.
    void wakeUpIfWaitingFor(SignalType newSignal);

    // Called from caller's thread exclusively.
    QByteArray readAndClear(QByteArray &data) const
    {
        const QByteArray tmp = data;
        data.clear();
        return tmp;
    }

    // Called from caller's or launcher's thread.
    void sendPacket(const Internal::LauncherPacket &packet);

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;
    const quintptr m_token;
    const ProcessMode m_processMode;
    SignalType m_waitingFor = SignalType::NoSignal;

    QProcess::ProcessState m_processState = QProcess::NotRunning;
    // cancel() sets it to false, modified only in caller's thread.
    bool m_awaitingShouldContinue = false;
    int m_processId = 0;
    int m_exitCode = 0;
    QProcess::ExitStatus m_exitStatus = QProcess::ExitStatus::NormalExit;
    QByteArray m_stdout;
    QByteArray m_stderr;
    QString m_errorString;
    QProcess::ProcessError m_error = QProcess::UnknownError;
    bool m_socketError = false;

    QString m_command;
    QStringList m_arguments;
    QProcessEnvironment m_environment;
    QString m_workingDirectory;
    QByteArray m_writeData;
    QProcess::ProcessChannelMode m_channelMode = QProcess::SeparateChannels;
    QString m_standardInputFile;

    // Lives in caller's thread.
    CallerHandle *m_callerHandle = nullptr;

    bool m_belowNormalPriority = false;
    QString m_nativeArguments;
    bool m_lowPriority = false;
    bool m_unixTerminalDisabled = false;

    friend class LauncherSocket;
    friend class CallerHandle;
};

class LauncherSocket : public QObject
{
    Q_OBJECT
    friend class LauncherInterfacePrivate;
public:
    // Called from caller's or launcher's thread.
    bool isReady() const { return m_socket.load(); }
    void sendData(const QByteArray &data);

    // Called from caller's thread exclusively.
    LauncherHandle *registerHandle(quintptr token, ProcessMode mode);
    void unregisterHandle(quintptr token);

signals:
    void ready();
    void errorOccurred(const QString &error);

private:
    // Called from caller's thread, moved to launcher's thread.
    LauncherSocket(QObject *parent = nullptr);

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

    std::atomic<QLocalSocket *> m_socket{nullptr};
    PacketParser m_packetParser;
    std::vector<QByteArray> m_requests;
    mutable QMutex m_mutex;
    QHash<quintptr, LauncherHandle *> m_handles;
};

} // namespace Internal
} // namespace Utils
