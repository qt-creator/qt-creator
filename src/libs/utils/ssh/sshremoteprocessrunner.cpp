/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "sshremoteprocessrunner.h"

#include "sshconnectionmanager.h"
#include "sshpseudoterminal.h"

#define ASSERT_STATE(states) assertState(states, Q_FUNC_INFO)

/*!
    \class Utils::SshRemoteProcessRunner

    \brief Convenience class for running a remote process over an SSH connection.
*/

namespace Utils {
namespace Internal {

class SshRemoteProcessRunnerPrivate : public QObject
{
    Q_OBJECT
public:
    SshRemoteProcessRunnerPrivate(const SshConnectionParameters &params,
        QObject *parent);
    SshRemoteProcessRunnerPrivate(const SshConnection::Ptr &connection,
        QObject *parent);
    ~SshRemoteProcessRunnerPrivate();
    void runWithoutTerminal(const QByteArray &command);
    void runInTerminal(const QByteArray &command,
        const SshPseudoTerminal &terminal);
    QByteArray command() const { return m_command; }

    const SshConnection::Ptr m_connection;
    SshRemoteProcess::Ptr m_process;

signals:
    void connectionError(Utils::SshError);
    void processStarted();
    void processOutputAvailable(const QByteArray &output);
    void processErrorOutputAvailable(const QByteArray &output);
    void processClosed(int exitStatus);

private slots:
    void handleConnected();
    void handleConnectionError(Utils::SshError error);
    void handleDisconnected();
    void handleProcessStarted();
    void handleProcessFinished(int exitStatus);

private:
    enum State { Inactive, Connecting, Connected, ProcessRunning };

    void run(const QByteArray &command);
    void setState(State state);
    void assertState(const QList<State> &allowedStates, const char *func);
    void assertState(State allowedState, const char *func);

    State m_state;
    bool m_needsRelease;
    bool m_runInTerminal;
    SshPseudoTerminal m_terminal;
    QByteArray m_command;
};


SshRemoteProcessRunnerPrivate::SshRemoteProcessRunnerPrivate(const SshConnectionParameters &params,
    QObject *parent)
    : QObject(parent),
      m_connection(SshConnectionManager::instance().acquireConnection(params)),
      m_state(Inactive),
      m_needsRelease(true)
{
}

SshRemoteProcessRunnerPrivate::SshRemoteProcessRunnerPrivate(const SshConnection::Ptr &connection,
    QObject *parent)
        : QObject(parent),
          m_connection(connection),
          m_state(Inactive),
          m_needsRelease(false)
{
}

SshRemoteProcessRunnerPrivate::~SshRemoteProcessRunnerPrivate()
{
    setState(Inactive);
}

void SshRemoteProcessRunnerPrivate::runWithoutTerminal(const QByteArray &command)
{
    m_runInTerminal = false;
    run(command);
}

void SshRemoteProcessRunnerPrivate::runInTerminal(const QByteArray &command,
    const SshPseudoTerminal &terminal)
{
    m_terminal = terminal;
    m_runInTerminal = true;
    run(command);
}

void SshRemoteProcessRunnerPrivate::run(const QByteArray &command)
{
    ASSERT_STATE(Inactive);
    setState(Connecting);

    m_command = command;
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)),
        SLOT(handleConnectionError(Utils::SshError)));
    connect(m_connection.data(), SIGNAL(disconnected()),
        SLOT(handleDisconnected()));
    if (m_connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        connect(m_connection.data(), SIGNAL(connected()),
            SLOT(handleConnected()));
        if (m_connection->state() == SshConnection::Unconnected)
            m_connection->connectToHost();
    }
}

void SshRemoteProcessRunnerPrivate::handleConnected()
{
    ASSERT_STATE(Connecting);
    setState(Connected);

    m_process = m_connection->createRemoteProcess(m_command);
    connect(m_process.data(), SIGNAL(started()), SLOT(handleProcessStarted()));
    connect(m_process.data(), SIGNAL(closed(int)),
        SLOT(handleProcessFinished(int)));
    connect(m_process.data(), SIGNAL(outputAvailable(QByteArray)),
        SIGNAL(processOutputAvailable(QByteArray)));
    connect(m_process.data(), SIGNAL(errorOutputAvailable(QByteArray)),
        SIGNAL(processErrorOutputAvailable(QByteArray)));
    if (m_runInTerminal)
        m_process->requestTerminal(m_terminal);
    m_process->start();
}

void SshRemoteProcessRunnerPrivate::handleConnectionError(Utils::SshError error)
{
    handleDisconnected();
    emit connectionError(error);
}

void SshRemoteProcessRunnerPrivate::handleDisconnected()
{
    ASSERT_STATE(QList<State>() << Connecting << Connected << ProcessRunning);
    setState(Inactive);
}

void SshRemoteProcessRunnerPrivate::handleProcessStarted()
{
    ASSERT_STATE(Connected);
    setState(ProcessRunning);

    emit processStarted();
}

void SshRemoteProcessRunnerPrivate::handleProcessFinished(int exitStatus)
{
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        ASSERT_STATE(Connected);
        break;
    case SshRemoteProcess::KilledBySignal:
    case SshRemoteProcess::ExitedNormally:
        ASSERT_STATE(ProcessRunning);
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Impossible exit status.");
    }
    setState(Inactive);
    emit processClosed(exitStatus);
}

void SshRemoteProcessRunnerPrivate::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        if (m_state == Inactive) {
            if (m_process)
                disconnect(m_process.data(), 0, this, 0);
            disconnect(m_connection.data(), 0, this, 0);
            if (m_needsRelease)
                SshConnectionManager::instance().releaseConnection(m_connection);
        }
    }
}

void SshRemoteProcessRunnerPrivate::assertState(const QList<State> &allowedStates,
    const char *func)
{
    if (!allowedStates.contains(m_state))
        qWarning("Unexpected state %d in function %s", m_state, func);
}

void SshRemoteProcessRunnerPrivate::assertState(State allowedState,
    const char *func)
{
    assertState(QList<State>() << allowedState, func);
}

} // namespace Internal

SshRemoteProcessRunner::Ptr SshRemoteProcessRunner::create(const SshConnectionParameters &params)
{
    return SshRemoteProcessRunner::Ptr(new SshRemoteProcessRunner(params));
}

SshRemoteProcessRunner::Ptr SshRemoteProcessRunner::create(const SshConnection::Ptr &connection)
{
    return SshRemoteProcessRunner::Ptr(new SshRemoteProcessRunner(connection));
}

SshRemoteProcessRunner::SshRemoteProcessRunner(const SshConnectionParameters &params)
    : d(new Internal::SshRemoteProcessRunnerPrivate(params, this))
{
    init();
}

SshRemoteProcessRunner::SshRemoteProcessRunner(const SshConnection::Ptr &connection)
    : d(new Internal::SshRemoteProcessRunnerPrivate(connection, this))
{
    init();
}

void SshRemoteProcessRunner::init()
{
    connect(d, SIGNAL(connectionError(Utils::SshError)),
        SIGNAL(connectionError(Utils::SshError)));
    connect(d, SIGNAL(processStarted()), SIGNAL(processStarted()));
    connect(d, SIGNAL(processClosed(int)), SIGNAL(processClosed(int)));
    connect(d, SIGNAL(processOutputAvailable(QByteArray)),
        SIGNAL(processOutputAvailable(QByteArray)));
    connect(d, SIGNAL(processErrorOutputAvailable(QByteArray)),
        SIGNAL(processErrorOutputAvailable(QByteArray)));
}

void SshRemoteProcessRunner::run(const QByteArray &command)
{
    d->runWithoutTerminal(command);
}

void SshRemoteProcessRunner::runInTerminal(const QByteArray &command,
    const SshPseudoTerminal &terminal)
{
    d->runInTerminal(command, terminal);
}

QByteArray SshRemoteProcessRunner::command() const { return d->command(); }
SshConnection::Ptr SshRemoteProcessRunner::connection() const { return d->m_connection; }
SshRemoteProcess::Ptr SshRemoteProcessRunner::process() const { return d->m_process; }

} // namespace Utils


#include "sshremoteprocessrunner.moc"
