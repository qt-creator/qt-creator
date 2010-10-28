#include "remoteprocesstest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <iostream>

using namespace Core;

RemoteProcessTest::RemoteProcessTest(const SshConnectionParameters &params)
    : m_timeoutTimer(new QTimer(this)),
      m_remoteRunner(SshRemoteProcessRunner::create(params)),
      m_state(Inactive)
{
    m_timeoutTimer->setInterval(5000);
    connect(m_timeoutTimer, SIGNAL(timeout()), SLOT(handleTimeout()));
}

RemoteProcessTest::~RemoteProcessTest() { }

void RemoteProcessTest::run()
{
    connect(m_remoteRunner.data(), SIGNAL(connectionError(Core::SshError)),
        SLOT(handleConnectionError()));
    connect(m_remoteRunner.data(), SIGNAL(processStarted()),
        SLOT(handleProcessStarted()));
    connect(m_remoteRunner.data(), SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleProcessStdout(QByteArray)));
    connect(m_remoteRunner.data(),
        SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleProcessStderr(QByteArray)));
    connect(m_remoteRunner.data(), SIGNAL(processClosed(int)),
        SLOT(handleProcessClosed(int)));

    std::cout << "Testing successful remote process..." << std::endl;
    m_state = TestingSuccess;
    m_started = false;
    m_timeoutTimer->start();
    m_remoteRunner->run("ls -a /tmp");
}

void RemoteProcessTest::handleConnectionError()
{
    std::cerr << "Error: Connection failure ("
        << qPrintable(m_remoteRunner->connection()->errorString()) << ")."
        << std::endl;
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
            Core::SshRemoteProcess::Ptr killer
                = m_remoteRunner->connection()->createRemoteProcess("pkill -9 sleep");
            killer->start();
        }
    }
}

void RemoteProcessTest::handleProcessStdout(const QByteArray &output)
{
    if (!m_started) {
        std::cerr << "Error: Remote output from non-started process."
            << std::endl;
        qApp->quit();
    } else if (m_state != TestingSuccess) {
        std::cerr << "Error: Got remote standard output in state " << m_state
            << "." << std::endl;
        qApp->quit();
    } else {
        m_remoteStdout += output;
    }
}

void RemoteProcessTest::handleProcessStderr(const QByteArray &output)
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
        m_remoteStderr += output;
    }
}

void RemoteProcessTest::handleProcessClosed(int exitStatus)
{
    switch (exitStatus) {
    case SshRemoteProcess::ExitedNormally:
        if (!m_started) {
            std::cerr << "Error: Process exited without starting." << std::endl;
            qApp->quit();
            return;
        }
        switch (m_state) {
        case TestingSuccess: {
            const int exitCode = m_remoteRunner->process()->exitCode();
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

            std::cout << "Ok. Testing unsuccessful remote process..."
                << std::endl;
            m_state = TestingFailure;
            m_started = false;
            m_timeoutTimer->start();
            m_remoteRunner->run("ls /wedontexepectsuchafiletoexist");
            break;
        }
        case TestingFailure: {
            const int exitCode = m_remoteRunner->process()->exitCode();
            if (exitCode == 0) {
                std::cerr << "Error: exit code is zero, expected non-zero."
                    << std::endl;
                qApp->quit();
                return;
            }
            if (m_remoteStderr.isEmpty()) {
                std::cerr << "Error: Command did not produce error output."
                    << std::endl;
                qApp->quit();
                return;
            }

            std::cout << "Ok. Testing crashing remote process..."
                << std::endl;
            m_state = TestingCrash;
            m_started = false;
            m_timeoutTimer->start();
            m_remoteRunner->run("sleep 100");
            break;
        }
        case TestingCrash:
            std::cerr << "Error: Successful exit from process that was "
                "supposed to crash." << std::endl;
            qApp->quit();
            return;
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
    case SshRemoteProcess::KilledBySignal:
        if (m_state != TestingCrash) {
            std::cerr << "Error: Unexpected crash." << std::endl;
            qApp->quit();
            return;
        }
        std::cout << "Ok. All tests succeeded." << std::endl;
        qApp->quit();
    }
}

void RemoteProcessTest::handleTimeout()
{
    std::cerr << "Error: Timeout waiting for progress." << std::endl;
    qApp->quit();
}
