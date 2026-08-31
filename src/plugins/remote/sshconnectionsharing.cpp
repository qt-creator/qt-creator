// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sshconnectionsharing.h"

#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>

#include <utils/environment.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>

#include <QMutex>
#include <QPointer>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote::Internal {

class SshSharedConnection : public QObject
{
    Q_OBJECT

public:
    explicit SshSharedConnection(const SshParameters &sshParameters, QObject *parent = nullptr);
    ~SshSharedConnection() override;

    SshParameters sshParameters() const { return m_sshParameters; }
    void ref();
    void deref();
    void makeStale();

    void connectToHost();
    void disconnectFromHost();

    QProcess::ProcessState state() const;
    QString socketFilePath() const
    {
        QTC_ASSERT(m_masterSocketDir, return {});
        return m_masterSocketDir->path() + "/cs";
    }

signals:
    void connected(const QString &socketFilePath);
    void disconnected(const ProcessResultData &result);

    void autoDestructRequested();

private:
    void emitConnected();
    void emitError(ProcessError processError, const QString &errorString);
    QString fullProcessError() const;
    QStringList connectionArgs(const FilePath &binary) const;

    const SshParameters m_sshParameters;
    std::unique_ptr<Process> m_masterProcess;
    std::unique_ptr<QTemporaryDir> m_masterSocketDir;
    QTimer m_timer;
    int m_ref = 0;
    bool m_stale = false;
    QProcess::ProcessState m_state = QProcess::NotRunning;
};

SshSharedConnection::SshSharedConnection(const SshParameters &sshParameters, QObject *parent)
    : QObject(parent), m_sshParameters(sshParameters)
{
}

SshSharedConnection::~SshSharedConnection()
{
    // Would be desirable to have, but the destruction order is not well-defined
    // between SshConnectionHandle and SshSharedConnection.
    // QTC_CHECK(m_ref == 0);

    disconnect();
    disconnectFromHost();
}

void SshSharedConnection::ref()
{
    ++m_ref;
    m_timer.stop();
}

void SshSharedConnection::deref()
{
    QTC_ASSERT(m_ref, return);
    if (--m_ref)
        return;
    if (m_stale) // no one uses it
        deleteLater();
    // not stale, so someone may reuse it
    m_timer.start(sshSettings().connectionSharingTimeoutInMinutes() * 1000 * 60);
}

void SshSharedConnection::makeStale()
{
    m_stale = true;
    if (!m_ref) // no one uses it
        deleteLater();
}

void SshSharedConnection::connectToHost()
{
    if (state() != QProcess::NotRunning)
        return;

    const FilePath sshBinary = sshSettings().sshFilePath();
    if (!sshBinary.exists()) {
        emitError(ProcessError::FailedToStart, Tr::tr("Cannot establish SSH connection: ssh binary "
                  "\"%1\" does not exist.").arg(sshBinary.toUserOutput()));
        return;
    }

    m_masterSocketDir.reset(new QTemporaryDir);
    if (!m_masterSocketDir->isValid()) {
        emitError(ProcessError::FailedToStart,
                    Tr::tr("Cannot establish SSH connection: Failed to create temporary "
                           "directory for control socket: %1")
                  .arg(m_masterSocketDir->errorString()));
        m_masterSocketDir.reset();
        return;
    }

    m_masterProcess.reset(new Process);
    SshParameters::setupSshEnvironment(m_masterProcess.get());
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SshSharedConnection::autoDestructRequested);
    connect(m_masterProcess.get(), &Process::readyReadStandardOutput, this, [this] {
        const QByteArray reply = m_masterProcess->readAllRawStandardOutput();
        if (reply == "\n")
            emitConnected();
        // TODO: otherwise emitError and finish master process?
    });
    // TODO: in case of refused connection we are getting the following on stdErr:
    // ssh: connect to host 127.0.0.1 port 22: Connection refused\r\n
    connect(m_masterProcess.get(), &Process::done, this, [this] {
        m_state = QProcess::NotRunning;
        const ProcessResult result = m_masterProcess->result();
        const ProcessResultData resultData = m_masterProcess->resultData();
        if (result == ProcessResult::StartFailed) {
            emitError(ProcessError::FailedToStart, Tr::tr("Cannot establish SSH connection.\n"
                                                      "Control process failed to start."));
            return;
        } else if (result == ProcessResult::FinishedWithError) {
            emitError(resultData.m_error, fullProcessError());
            return;
        }
        emit disconnected(resultData);
    });

    QStringList args = QStringList{"-M", "-N", "-o", "ControlPersist=no",
            "-o", "ServerAliveInterval=10", // TODO: Make configurable?
            "-o", "PermitLocalCommand=yes", // Enable local command
            "-o", "LocalCommand=echo"}      // Local command is executed after successfully
                                            // connecting to the server. "echo" will print "\n"
                                            // on the process output if everything went fine.
            << connectionArgs(sshBinary);
    if (!m_sshParameters.x11DisplayName().isEmpty()) {
        args.prepend("-X");
        Environment env = m_masterProcess->environment();
        env.set("DISPLAY", m_sshParameters.x11DisplayName());
        m_masterProcess->setEnvironment(env);
    }
    m_masterProcess->setCommand(CommandLine(sshBinary, args));
    m_state = QProcess::Starting;
    m_masterProcess->start();
}

void SshSharedConnection::disconnectFromHost()
{
    m_masterProcess.reset();
    m_masterSocketDir.reset();
}

QProcess::ProcessState SshSharedConnection::state() const
{
    return m_state;
}

void SshSharedConnection::emitConnected()
{
    m_state = QProcess::Running;
    emit connected(socketFilePath());
}

void SshSharedConnection::emitError(ProcessError error, const QString &errorString)
{
    m_state = QProcess::NotRunning;
    ProcessResultData resultData{-1, ProcessExitStatus::CrashExit,
                                 ProcessError::UnknownError, {}};
    if (m_masterProcess)
        resultData = m_masterProcess->resultData();
    resultData.m_error = error;
    resultData.m_errorString = errorString;
    emit disconnected(resultData);
}

QString SshSharedConnection::fullProcessError() const
{
    const QString errorString = m_masterProcess->exitStatus() == ProcessExitStatus::CrashExit
            ? m_masterProcess->errorString() : QString();
    const QString standardError = m_masterProcess->cleanedStdErr();
    const QString errorPrefix = errorString.isEmpty() && standardError.isEmpty()
            ? Tr::tr("SSH connection failure.") : Tr::tr("SSH connection failure:");
    QStringList allErrors {errorPrefix, errorString, standardError};
    allErrors.removeAll({});
    return allErrors.join('\n');
}

QStringList SshSharedConnection::connectionArgs(const FilePath &binary) const
{
    return m_sshParameters.connectionOptions(binary) << "-o" << ("ControlPath=" + socketFilePath())
                                                     << m_sshParameters.host();
}

// public methods are thread-safe
// Starts a new thread and manages shared connections there.
// The new thread is needed since SshSharedConnection is not thread-safe itself,
// and it uses deleteLater, so we need an event loop to run things on that is never blocked even
// if some thread waits for e.g. a device process to finish.
class SshConnectionHandler final : public QThread
{
public:
    SshConnectionHandler()
    {
        setObjectName("SshConnectionHandler");
        m_guard.moveToThread(this);
    }
    ~SshConnectionHandler() final
    {
        quit();
        wait();
    }

    void attachToSharedConnection(
        SshConnectionHandle *connectionHandle, const SshParameters &sshParameters);

    QString socketFilePath(const SshParameters &sshParameters);
    void closeConnections(const SshParameters &sshParameters);

private:
    void ensureRunning();
    void run() final;
    // The following three always run in the handler thread.
    SshSharedConnection *connectionFor(const SshParameters &sshParameters, bool *isNew);
    QString attachToSharedConnectionImpl(
        SshConnectionHandle *connectionHandle, const SshParameters &sshParameters);
    QString socketFilePathImpl(const SshParameters &sshParameters);
    void closeConnectionsImpl(const SshParameters &sshParameters);

    QObject m_guard;
    mutable QMutex m_mutex;
    QList<SshSharedConnection *> m_connections;
};

// Runs the passed function in the handler thread and returns its result.
template <typename Result, typename Function>
static Result invokeInHandlerThread(QObject *guard, Function function)
{
    Result result;
    const Qt::ConnectionType connectionType = QThread::currentThread() == guard->thread()
                                                  ? Qt::DirectConnection
                                                  : Qt::BlockingQueuedConnection;
    QTC_CHECK(connectionType != Qt::DirectConnection); // should never happen
    QMetaObject::invokeMethod(guard, function, connectionType, &result);
    return result;
}

void SshConnectionHandler::ensureRunning()
{
    QMutexLocker locker(&m_mutex);
    if (isRunning())
        return;
    start();
}

void SshConnectionHandler::run()
{
    exec();
    QMutexLocker locker(&m_mutex);
    qDeleteAll(m_connections);
    m_connections.clear();
}

void SshConnectionHandler::attachToSharedConnection(
    SshConnectionHandle *connectionHandle, const SshParameters &sshParameters)
{
    ensureRunning();
    const QString socketFilePath = invokeInHandlerThread<QString>(&m_guard,
        [this, connectionHandle, sshParameters] {
            return attachToSharedConnectionImpl(connectionHandle, sshParameters);
        });

    if (!socketFilePath.isEmpty())
        emit connectionHandle->connected(socketFilePath);
}

QString SshConnectionHandler::socketFilePath(const SshParameters &sshParameters)
{
    ensureRunning();
    return invokeInHandlerThread<QString>(&m_guard,
        [this, sshParameters] { return socketFilePathImpl(sshParameters); });
}

void SshConnectionHandler::closeConnections(const SshParameters &sshParameters)
{
    if (!isRunning())
        return;
    invokeInHandlerThread<QString>(&m_guard,
        [this, sshParameters] { closeConnectionsImpl(sshParameters); return QString(); });
}

SshSharedConnection *SshConnectionHandler::connectionFor(const SshParameters &sshParameters,
                                                         bool *isNew)
{
    for (SshSharedConnection *connection : std::as_const(m_connections)) {
        if (connection->sshParameters() == sshParameters) {
            *isNew = false;
            return connection;
        }
    }

    auto connection = new SshSharedConnection(sshParameters);
    connect(
        connection,
        &SshSharedConnection::autoDestructRequested,
        &m_guard,
        [that = QPointer(this), connection = QPointer(connection)] {
            QTC_ASSERT(that && connection, return);
            // This slot is just for removing the connection from the connection list.
            // The SshSharedConnection could have deleted itself otherwise.
            QMutexLocker locker(&that->m_mutex);
            that->m_connections.removeOne(connection);
            connection->deleteLater();
        });
    m_connections.append(connection);
    *isNew = true;
    return connection;
}

QString SshConnectionHandler::attachToSharedConnectionImpl(
    SshConnectionHandle *connectionHandle, const SshParameters &sshParameters)
{
    QMutexLocker locker(&m_mutex);
    bool isNew = false;
    SshSharedConnection *matchingConnection = connectionFor(sshParameters, &isNew);

    matchingConnection->ref();

    connect(
        matchingConnection,
        &SshSharedConnection::connected,
        connectionHandle,
        &SshConnectionHandle::connected);
    connect(
        matchingConnection,
        &SshSharedConnection::disconnected,
        connectionHandle,
        &SshConnectionHandle::disconnected);

    connect(
        connectionHandle,
        &SshConnectionHandle::detachFromSharedConnection,
        matchingConnection,
        &SshSharedConnection::deref,
        // Ensure the signal is delivered before sender's
        // destruction, otherwise we may get out of sync
        // with ref count.
        Qt::BlockingQueuedConnection);

    if (matchingConnection->state() == QProcess::Running)
        return matchingConnection->socketFilePath();

    if (matchingConnection->state() == QProcess::NotRunning)
        matchingConnection->connectToHost();

    return {};
}

// A caller without a handle to wait on gets the socket only once the connection is up, and
// starts it on the way. Its ref keeps the connection from timing out while commands keep
// coming, and lets it go once they stop. A connection that failed to come up is not started
// again: it would cost every later command a second ssh process on top of its own.
QString SshConnectionHandler::socketFilePathImpl(const SshParameters &sshParameters)
{
    QMutexLocker locker(&m_mutex);
    bool isNew = false;
    SshSharedConnection *connection = connectionFor(sshParameters, &isNew);

    connection->ref();
    if (isNew)
        connection->connectToHost();
    const QString socketFilePath = connection->state() == QProcess::Running
                                       ? connection->socketFilePath() : QString();
    connection->deref();
    return socketFilePath;
}

// Takes the connections to the host out of the pool, so that the next command sets up its own,
// and lets those still carrying a command end with it.
void SshConnectionHandler::closeConnectionsImpl(const SshParameters &sshParameters)
{
    QMutexLocker locker(&m_mutex);
    const QList<SshSharedConnection *> connections = m_connections;
    for (SshSharedConnection *connection : connections) {
        if (connection->sshParameters().userAtHostAndPort() == sshParameters.userAtHostAndPort()) {
            m_connections.removeOne(connection);
            connection->makeStale();
        }
    }
}

// One connection pool for the whole client: which device asked for a connection to a host says
// nothing about whether the next command can use it.
static SshConnectionHandler *connectionHandler()
{
    static GuardedObject<SshConnectionHandler> theHandler;
    return theHandler.get(); // null once the shutdown guard has taken it down
}

void setupSshConnectionSharing()
{
    connectionHandler();
}

void attachToSharedConnection(SshConnectionHandle *handle, const SshParameters &parameters)
{
    if (SshConnectionHandler *handler = connectionHandler())
        handler->attachToSharedConnection(handle, parameters);
}

QStringList sharedConnectionOptions(const SshParameters &parameters)
{
    if (!sshSettings().useConnectionSharing())
        return {};

    SshConnectionHandler *handler = connectionHandler();
    if (!handler)
        return {};

    const QString socketFilePath = handler->socketFilePath(parameters);
    if (socketFilePath.isEmpty())
        return {};
    return {"-o", "ControlPath=" + socketFilePath};
}

void closeSharedConnections(const SshParameters &parameters)
{
    if (SshConnectionHandler *handler = connectionHandler())
        handler->closeConnections(parameters);
}

} // namespace Remote::Internal

#include "sshconnectionsharing.moc"
