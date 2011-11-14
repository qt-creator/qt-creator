/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
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
    SshRemoteProcessRunnerPrivate(QObject *parent);
    ~SshRemoteProcessRunnerPrivate();
    void runWithoutTerminal(const QByteArray &command, const SshConnectionParameters &sshParams);
    void runInTerminal(const QByteArray &command, const SshPseudoTerminal &terminal,
        const SshConnectionParameters &sshParams);
    QByteArray command() const { return m_command; }
    Utils::SshError lastConnectionError() const { return m_lastConnectionError; }
    QString lastConnectionErrorString() const { return m_lastConnectionErrorString; }

    SshRemoteProcess::Ptr m_process;

signals:
    void connectionError();
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
    void handleStdout();
    void handleStderr();

private:
    enum State { Inactive, Connecting, Connected, ProcessRunning };

    void run(const QByteArray &command, const SshConnectionParameters &sshParams);
    void setState(State state);
    void assertState(const QList<State> &allowedStates, const char *func);
    void assertState(State allowedState, const char *func);

    SshConnection::Ptr m_connection;
    State m_state;
    bool m_runInTerminal;
    SshPseudoTerminal m_terminal;
    QByteArray m_command;
    Utils::SshError m_lastConnectionError;
    QString m_lastConnectionErrorString;
};


SshRemoteProcessRunnerPrivate::SshRemoteProcessRunnerPrivate(QObject *parent)
    : QObject(parent), m_state(Inactive)
{
}

SshRemoteProcessRunnerPrivate::~SshRemoteProcessRunnerPrivate()
{
    setState(Inactive);
}

void SshRemoteProcessRunnerPrivate::runWithoutTerminal(const QByteArray &command,
    const SshConnectionParameters &sshParams)
{
    m_runInTerminal = false;
    run(command, sshParams);
}

void SshRemoteProcessRunnerPrivate::runInTerminal(const QByteArray &command,
    const SshPseudoTerminal &terminal, const SshConnectionParameters &sshParams)
{
    m_terminal = terminal;
    m_runInTerminal = true;
    run(command, sshParams);
}

void SshRemoteProcessRunnerPrivate::run(const QByteArray &command,
    const SshConnectionParameters &sshParams)
{
    setState(Inactive);
    setState(Connecting);

    m_lastConnectionError = SshNoError;
    m_lastConnectionErrorString.clear();
    m_command = command;
    m_connection = SshConnectionManager::instance().acquireConnection(sshParams);
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)),
        SLOT(handleConnectionError(Utils::SshError)));
    connect(m_connection.data(), SIGNAL(disconnected()), SLOT(handleDisconnected()));
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
    connect(m_process.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleStdout()));
    connect(m_process.data(), SIGNAL(readyReadStandardError()), SLOT(handleStderr()));
    if (m_runInTerminal)
        m_process->requestTerminal(m_terminal);
    m_process->start();
}

void SshRemoteProcessRunnerPrivate::handleConnectionError(Utils::SshError error)
{
    m_lastConnectionError = error;
    m_lastConnectionErrorString = m_connection->errorString();
    handleDisconnected();
    emit connectionError();
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

void SshRemoteProcessRunnerPrivate::handleStdout()
{
    emit processOutputAvailable(m_process->readAllStandardOutput());
}

void SshRemoteProcessRunnerPrivate::handleStderr()
{
    emit processErrorOutputAvailable(m_process->readAllStandardError());
}

void SshRemoteProcessRunnerPrivate::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        if (m_state == Inactive) {
            if (m_process)
                disconnect(m_process.data(), 0, this, 0);
            if (m_connection) {
                disconnect(m_connection.data(), 0, this, 0);
                SshConnectionManager::instance().releaseConnection(m_connection);
                m_connection.clear();
            }
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

SshRemoteProcessRunner::SshRemoteProcessRunner(QObject *parent)
    : QObject(parent), d(new Internal::SshRemoteProcessRunnerPrivate(this))
{
    connect(d, SIGNAL(connectionError()), SIGNAL(connectionError()));
    connect(d, SIGNAL(processStarted()), SIGNAL(processStarted()));
    connect(d, SIGNAL(processClosed(int)), SIGNAL(processClosed(int)));
    connect(d, SIGNAL(processOutputAvailable(QByteArray)),
        SIGNAL(processOutputAvailable(QByteArray)));
    connect(d, SIGNAL(processErrorOutputAvailable(QByteArray)),
        SIGNAL(processErrorOutputAvailable(QByteArray)));
}

void SshRemoteProcessRunner::run(const QByteArray &command, const SshConnectionParameters &params)
{
    d->runWithoutTerminal(command, params);
}

void SshRemoteProcessRunner::runInTerminal(const QByteArray &command,
    const SshPseudoTerminal &terminal, const SshConnectionParameters &params)
{
    d->runInTerminal(command, terminal, params);
}

QByteArray SshRemoteProcessRunner::command() const { return d->command(); }
SshError SshRemoteProcessRunner::lastConnectionError() const { return d->lastConnectionError(); }
SshRemoteProcess::Ptr SshRemoteProcessRunner::process() const { return d->m_process; }

QString SshRemoteProcessRunner::lastConnectionErrorString() const
{
    return d->lastConnectionErrorString();
}


} // namespace Utils


#include "sshremoteprocessrunner.moc"
