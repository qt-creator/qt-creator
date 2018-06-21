/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "remoteprocesstest.h"

#include <ssh/sshpseudoterminal.h>

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

#include <iostream>

using namespace QSsh;

const QByteArray StderrOutput("ChannelTest");

RemoteProcessTest::RemoteProcessTest(const SshConnectionParameters &params)
    : m_sshParams(params),
      m_timeoutTimer(new QTimer(this)),
      m_sshConnection(0),
      m_remoteRunner(new SshRemoteProcessRunner(this)),
      m_state(Inactive)
{
    m_timeoutTimer->setInterval(5000);
    connect(m_timeoutTimer, &QTimer::timeout, this, &RemoteProcessTest::handleTimeout);
}

RemoteProcessTest::~RemoteProcessTest()
{
    delete m_sshConnection;
}

void RemoteProcessTest::run()
{
    connect(m_remoteRunner, &SshRemoteProcessRunner::connectionError,
            this, &RemoteProcessTest::handleConnectionError);
    connect(m_remoteRunner, &SshRemoteProcessRunner::processStarted,
            this, &RemoteProcessTest::handleProcessStarted);
    connect(m_remoteRunner, &SshRemoteProcessRunner::readyReadStandardOutput,
            this, &RemoteProcessTest::handleProcessStdout);
    connect(m_remoteRunner, &SshRemoteProcessRunner::readyReadStandardError,
            this, &RemoteProcessTest::handleProcessStderr);
    connect(m_remoteRunner, &SshRemoteProcessRunner::processClosed,
            this, &RemoteProcessTest::handleProcessClosed);

    std::cout << "Testing successful remote process... " << std::flush;
    m_state = TestingSuccess;
    m_started = false;
    m_timeoutTimer->start();
    m_remoteRunner->run("ls -a /tmp", m_sshParams);
}

void RemoteProcessTest::handleConnectionError()
{
    const QString error = m_state == TestingIoDevice || m_state == TestingProcessChannels
        ? m_sshConnection->errorString() : m_remoteRunner->lastConnectionErrorString();

    std::cerr << "Error: Connection failure (" << qPrintable(error) << ")." << std::endl;
    QCoreApplication::exit(EXIT_FAILURE);
}

void RemoteProcessTest::handleProcessStarted()
{
    if (m_started) {
        std::cerr << "Error: Received started() signal again." << std::endl;
        QCoreApplication::exit(EXIT_FAILURE);
    } else {
        m_started = true;
        if (m_state == TestingCrash) {
            SshRemoteProcessRunner * const killer = new SshRemoteProcessRunner(this);
            killer->run("pkill -9 sleep", m_sshParams);
        } else if (m_state == TestingIoDevice) {
            connect(m_catProcess.data(), &QIODevice::readyRead,
                    this, &RemoteProcessTest::handleReadyRead);
            m_textStream = new QTextStream(m_catProcess.data());
            *m_textStream << testString();
            m_textStream->flush();
        }
    }
}

void RemoteProcessTest::handleProcessStdout()
{
    if (!m_started) {
        std::cerr << "Error: Remote output from non-started process."
            << std::endl;
        QCoreApplication::exit(EXIT_FAILURE);
    } else if (m_state != TestingSuccess && m_state != TestingTerminal) {
        std::cerr << "Error: Got remote standard output in state " << m_state
            << "." << std::endl;
        QCoreApplication::exit(EXIT_FAILURE);
    } else {
        m_remoteStdout += m_remoteRunner->readAllStandardOutput();
    }
}

void RemoteProcessTest::handleProcessStderr()
{
    if (!m_started) {
        std::cerr << "Error: Remote error output from non-started process."
            << std::endl;
        QCoreApplication::exit(EXIT_FAILURE);
    } else if (m_state == TestingSuccess) {
        std::cerr << "Error: Unexpected remote standard error output."
            << std::endl;
        QCoreApplication::exit(EXIT_FAILURE);
    } else {
        m_remoteStderr += m_remoteRunner->readAllStandardError();
    }
}

void RemoteProcessTest::handleProcessClosed(int exitStatus)
{
    switch (exitStatus) {
    case SshRemoteProcess::NormalExit:
        if (!m_started) {
            std::cerr << "Error: Process exited without starting." << std::endl;
            QCoreApplication::exit(EXIT_FAILURE);
            return;
        }
        switch (m_state) {
        case TestingSuccess: {
            const int exitCode = m_remoteRunner->processExitCode();
            if (exitCode != 0) {
                std::cerr << "Error: exit code is " << exitCode
                    << ", expected zero." << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }
            if (m_remoteStdout.isEmpty()) {
                std::cerr << "Error: Command did not produce output."
                    << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }

            std::cout << "Ok.\nTesting unsuccessful remote process... " << std::flush;
            m_state = TestingFailure;
            m_started = false;
            m_timeoutTimer->start();
            m_remoteRunner->run("top -n 1", m_sshParams); // Does not succeed without terminal.
            break;
        }
        case TestingFailure: {
            const int exitCode = m_remoteRunner->processExitCode();
            if (exitCode == 0) {
                std::cerr << "Error: exit code is zero, expected non-zero."
                    << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }
            if (m_remoteStderr.isEmpty()) {
                std::cerr << "Error: Command did not produce error output." << std::flush;
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }

            std::cout << "Ok.\nTesting crashing remote process... " << std::flush;
            m_state = TestingCrash;
            m_started = false;
            m_timeoutTimer->start();
            m_remoteRunner->run("/bin/sleep 100", m_sshParams);
            break;
        }
        case TestingCrash:
            if (m_remoteRunner->processExitCode() == 0) {
                std::cerr << "Error: Successful exit from process that was "
                    "supposed to crash." << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
            } else {
                // Some shells (e.g. mksh) don't report "killed", but just a non-zero exit code.
                handleSuccessfulCrashTest();
            }
            break;
        case TestingTerminal: {
            const int exitCode = m_remoteRunner->processExitCode();
            if (exitCode != 0) {
                std::cerr << "Error: exit code is " << exitCode
                    << ", expected zero." << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }
            if (m_remoteStdout.isEmpty()) {
                std::cerr << "Error: Command did not produce output."
                    << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }
            std::cout << "Ok.\nTesting I/O device functionality... " << std::flush;
            m_state = TestingIoDevice;
            m_sshConnection = new SshConnection(m_sshParams);
            connect(m_sshConnection, &SshConnection::connected,
                    this, &RemoteProcessTest::handleConnected);
            connect(m_sshConnection, &SshConnection::error,
                    this, &RemoteProcessTest::handleConnectionError);
            m_sshConnection->connectToHost();
            m_timeoutTimer->start();
            break;
        }
        case TestingIoDevice:
            if (m_catProcess->exitCode() == 0) {
                std::cerr << "Error: Successful exit from process that was supposed to crash."
                    << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
            } else {
                handleSuccessfulIoTest();
            }
            break;
        case TestingProcessChannels:
            if (m_remoteStderr.isEmpty()) {
                std::cerr << "Error: Did not receive readyReadStderr()." << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }
            if (m_remoteData != StderrOutput) {
                std::cerr << "Error: Expected output '" << StderrOutput.data() << "', received '"
                    << m_remoteData.data() << "'." << std::endl;
                QCoreApplication::exit(EXIT_FAILURE);
                return;
            }
            std::cout << "Ok.\nAll tests succeeded." << std::endl;
            QCoreApplication::quit();
            break;
        case Inactive:
            Q_ASSERT(false);
        }
        break;
    case SshRemoteProcess::FailedToStart:
        if (m_started) {
            std::cerr << "Error: Got 'failed to start' signal for process "
                "that has not started yet." << std::endl;
        } else {
            std::cerr << "Error: Process failed to start." << std::endl;
        }
        QCoreApplication::exit(EXIT_FAILURE);
        break;
    case SshRemoteProcess::CrashExit:
        switch (m_state) {
        case TestingCrash:
            handleSuccessfulCrashTest();
            break;
        case TestingIoDevice:
            handleSuccessfulIoTest();
            break;
        default:
            std::cerr << "Error: Unexpected crash." << std::endl;
            QCoreApplication::exit(EXIT_FAILURE);
            return;
        }
    }
}

void RemoteProcessTest::handleTimeout()
{
    std::cerr << "Error: Timeout waiting for progress." << std::endl;
    QCoreApplication::exit(EXIT_FAILURE);
}

void RemoteProcessTest::handleConnected()
{
    Q_ASSERT(m_state == TestingIoDevice);

    m_catProcess = m_sshConnection->createRemoteProcess(QString::fromLatin1("/bin/cat").toUtf8());
    connect(m_catProcess.data(), &SshRemoteProcess::started,
            this, &RemoteProcessTest::handleProcessStarted);
    connect(m_catProcess.data(), &SshRemoteProcess::closed,
            this, &RemoteProcessTest::handleProcessClosed);
    m_started = false;
    m_timeoutTimer->start();
    m_catProcess->start();
}

QString RemoteProcessTest::testString() const
{
    return QLatin1String("x\r\n");
}

void RemoteProcessTest::handleReadyRead()
{
    switch (m_state) {
    case TestingIoDevice: {
        const QString &data = QString::fromUtf8(m_catProcess->readAll());
        if (data != testString()) {
            std::cerr << "Testing of QIODevice functionality failed: Expected '"
                << qPrintable(testString()) << "', got '" << qPrintable(data) << "'." << std::endl;
            QCoreApplication::exit(EXIT_FAILURE);
        }
        SshRemoteProcessRunner * const killer = new SshRemoteProcessRunner(this);
        killer->run("pkill -9 cat", m_sshParams);
        break;
    }
    case TestingProcessChannels:
        m_remoteData += m_echoProcess->readAll();
        break;
    default:
        qFatal("%s: Unexpected state %d.", Q_FUNC_INFO, m_state);
    }

}

void RemoteProcessTest::handleReadyReadStdout()
{
    Q_ASSERT(m_state == TestingProcessChannels);

    std::cerr << "Error: Received unexpected stdout data." << std::endl;
    QCoreApplication::exit(EXIT_FAILURE);
}

void RemoteProcessTest::handleReadyReadStderr()
{
    Q_ASSERT(m_state == TestingProcessChannels);

    m_remoteStderr = "dummy";
}

void RemoteProcessTest::handleSuccessfulCrashTest()
{
    std::cout << "Ok.\nTesting remote process with terminal... " << std::flush;
    m_state = TestingTerminal;
    m_started = false;
    m_timeoutTimer->start();
    m_remoteRunner->runInTerminal("top -n 1", SshPseudoTerminal(), m_sshParams);
}

void RemoteProcessTest::handleSuccessfulIoTest()
{
    std::cout << "Ok\nTesting process channels... " << std::flush;
    m_state = TestingProcessChannels;
    m_started = false;
    m_remoteStderr.clear();
    m_echoProcess = m_sshConnection->createRemoteProcess("printf " + StderrOutput + " >&2");
    m_echoProcess->setReadChannel(QProcess::StandardError);
    connect(m_echoProcess.data(), &SshRemoteProcess::started,
            this, &RemoteProcessTest::handleProcessStarted);
    connect(m_echoProcess.data(), &SshRemoteProcess::closed,
            this, &RemoteProcessTest::handleProcessClosed);
    connect(m_echoProcess.data(), &QIODevice::readyRead,
            this, &RemoteProcessTest::handleReadyRead);
    connect(m_echoProcess.data(), &SshRemoteProcess::readyReadStandardError,
            this, &RemoteProcessTest::handleReadyReadStderr);
    m_echoProcess->start();
    m_timeoutTimer->start();
}
