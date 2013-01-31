/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    connect(m_timeoutTimer, SIGNAL(timeout()), SLOT(handleTimeout()));
}

RemoteProcessTest::~RemoteProcessTest()
{
    delete m_sshConnection;
}

void RemoteProcessTest::run()
{
    connect(m_remoteRunner, SIGNAL(connectionError()),
        SLOT(handleConnectionError()));
    connect(m_remoteRunner, SIGNAL(processStarted()),
        SLOT(handleProcessStarted()));
    connect(m_remoteRunner, SIGNAL(readyReadStandardOutput()), SLOT(handleProcessStdout()));
    connect(m_remoteRunner, SIGNAL(readyReadStandardError()), SLOT(handleProcessStderr()));
    connect(m_remoteRunner, SIGNAL(processClosed(int)),
        SLOT(handleProcessClosed(int)));

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
    qApp->quit();
}

void RemoteProcessTest::handleProcessStarted()
{
    if (m_started) {
        std::cerr << "Error: Received started() signal again." << std::endl;
        qApp->quit();
    } else {
        m_started = true;
        if (m_state == TestingCrash) {
            QSsh::SshRemoteProcessRunner * const killer
                = new QSsh::SshRemoteProcessRunner(this);
            killer->run("pkill -9 sleep", m_sshParams);
        } else if (m_state == TestingIoDevice) {
            connect(m_catProcess.data(), SIGNAL(readyRead()), SLOT(handleReadyRead()));
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
        qApp->quit();
    } else if (m_state != TestingSuccess && m_state != TestingTerminal) {
        std::cerr << "Error: Got remote standard output in state " << m_state
            << "." << std::endl;
        qApp->quit();
    } else {
        m_remoteStdout += m_remoteRunner->readAllStandardOutput();
    }
}

void RemoteProcessTest::handleProcessStderr()
{
    if (!m_started) {
        std::cerr << "Error: Remote error output from non-started process."
            << std::endl;
        qApp->quit();
    } else if (m_state == TestingSuccess) {
        std::cerr << "Error: Unexpected remote standard error output."
            << std::endl;
        qApp->quit();
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
            qApp->quit();
            return;
        }
        switch (m_state) {
        case TestingSuccess: {
            const int exitCode = m_remoteRunner->processExitCode();
            if (exitCode != 0) {
                std::cerr << "Error: exit code is " << exitCode
                    << ", expected zero." << std::endl;
                qApp->quit();
                return;
            }
            if (m_remoteStdout.isEmpty()) {
                std::cerr << "Error: Command did not produce output."
                    << std::endl;
                qApp->quit();
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
                qApp->quit();
                return;
            }
            if (m_remoteStderr.isEmpty()) {
                std::cerr << "Error: Command did not produce error output." << std::flush;
                qApp->quit();
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
                qApp->quit();
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
                qApp->quit();
                return;
            }
            if (m_remoteStdout.isEmpty()) {
                std::cerr << "Error: Command did not produce output."
                    << std::endl;
                qApp->quit();
                return;
            }
            std::cout << "Ok.\nTesting I/O device functionality... " << std::flush;
            m_state = TestingIoDevice;
            m_sshConnection = new QSsh::SshConnection(m_sshParams);
            connect(m_sshConnection, SIGNAL(connected()), SLOT(handleConnected()));
            connect(m_sshConnection, SIGNAL(error(QSsh::SshError)),
                SLOT(handleConnectionError()));
            m_sshConnection->connectToHost();
            m_timeoutTimer->start();
            break;
        }
        case TestingIoDevice:
            if (m_catProcess->exitCode() == 0) {
                std::cerr << "Error: Successful exit from process that was supposed to crash."
                    << std::endl;
                qApp->exit(EXIT_FAILURE);
            } else {
                handleSuccessfulIoTest();
            }
            break;
        case TestingProcessChannels:
            if (m_remoteStderr.isEmpty()) {
                std::cerr << "Error: Did not receive readyReadStderr()." << std::endl;
                qApp->exit(EXIT_FAILURE);
                return;
            }
            if (m_remoteData != StderrOutput) {
                std::cerr << "Error: Expected output '" << StderrOutput.data() << "', received '"
                    << m_remoteData.data() << "'." << std::endl;
                qApp->exit(EXIT_FAILURE);
                return;
            }
            std::cout << "Ok.\nAll tests succeeded." << std::endl;
            qApp->quit();
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
        qApp->quit();
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
            qApp->quit();
            return;
        }
    }
}

void RemoteProcessTest::handleTimeout()
{
    std::cerr << "Error: Timeout waiting for progress." << std::endl;
    qApp->quit();
}

void RemoteProcessTest::handleConnected()
{
    Q_ASSERT(m_state == TestingIoDevice);

    m_catProcess = m_sshConnection->createRemoteProcess(QString::fromLocal8Bit("/bin/cat").toUtf8());
    connect(m_catProcess.data(), SIGNAL(started()), SLOT(handleProcessStarted()));
    connect(m_catProcess.data(), SIGNAL(closed(int)), SLOT(handleProcessClosed(int)));
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
            qApp->exit(1);
        }
        QSsh::SshRemoteProcessRunner * const killer = new QSsh::SshRemoteProcessRunner(this);
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
    qApp->exit(EXIT_FAILURE);
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
    connect(m_echoProcess.data(), SIGNAL(started()), SLOT(handleProcessStarted()));
    connect(m_echoProcess.data(), SIGNAL(closed(int)), SLOT(handleProcessClosed(int)));
    connect(m_echoProcess.data(), SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(m_echoProcess.data(), SIGNAL(readyReadStandardError()),
            SLOT(handleReadyReadStderr()));
    m_echoProcess->start();
    m_timeoutTimer->start();
}
