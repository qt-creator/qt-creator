// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processtestapp.h"

#include <utils/process.h>

#include <QCoreApplication>
#include <QDebug>
#include <QHash>
#include <QMutex>
#include <QScopeGuard>
#include <QThread>

#include <iostream>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#else
#include <atomic>
#include <signal.h>
#include <unistd.h>
#endif

using namespace Utils;

static QHash<const char *, ProcessTestApp::SubProcessMain> s_subProcesses = {};

ProcessTestApp::ProcessTestApp() = default;

void ProcessTestApp::invokeSubProcess()
{
    ProcessTestApp processTestApp;
    int returnValue = 1;
    const QScopeGuard cleanup([&returnValue] {
        QMetaObject::invokeMethod(qApp, [returnValue] {
            qApp->exit(returnValue);
        }, Qt::QueuedConnection);
    });

    for (auto it = s_subProcesses.constBegin(); it != s_subProcesses.constEnd(); ++it) {
        if (qEnvironmentVariableIsSet(it.key())) {
            returnValue = it.value()();
            return;
        }
    }
    qWarning() << "No test was run!";
}

void ProcessTestApp::registerSubProcess(const char *envVar, const SubProcessMain &main)
{
    s_subProcesses.insert(envVar, main);
}

void ProcessTestApp::unregisterSubProcess(const char *envVar)
{
    s_subProcesses.remove(envVar);
}

static QString s_pathToProcessTestApp;

static Environment subEnvironment(const char *envVar, const QString &envVal)
{
    Environment env = Environment::systemEnvironment();
    env.set(QString::fromLatin1(envVar), envVal);
    return env;
}

void SubProcessConfig::setPathToProcessTestApp(const QString &path)
{
    s_pathToProcessTestApp = path;
}

SubProcessConfig::SubProcessConfig(const char *envVar, const QString &envVal)
    : m_environment(subEnvironment(envVar, envVal))
{
}

void SubProcessConfig::setupSubProcess(Process *subProcess) const
{
    subProcess->setEnvironment(m_environment);
    const FilePath filePath = FilePath::fromString(s_pathToProcessTestApp
                            + QLatin1String("/processtestapp")).withExecutableSuffix();
    subProcess->setCommand(CommandLine(filePath, {}));
}

void SubProcessConfig::setupSubProcess(QProcess *subProcess) const
{
    subProcess->setProcessEnvironment(m_environment.toProcessEnvironment());
    subProcess->setProgram(FilePath::fromString(s_pathToProcessTestApp
                           + QLatin1String("/processtestapp")).withExecutableSuffix().toString());
}

static void doCrash()
{
    qFatal("The application has crashed purposefully!");
}

int ProcessTestApp::SimpleTest::main()
{
    std::cout << s_simpleTestData << std::endl;
    return 0;
}

int ProcessTestApp::ExitCode::main()
{
    const int exitCode = qEnvironmentVariableIntValue(envVar());
    std::cout << "Exiting with code:" << exitCode << std::endl;
    return exitCode;
}

int ProcessTestApp::RunBlockingStdOut::main()
{
    std::cout << "Wait for the Answer to the Ultimate Question of Life, "
                 "The Universe, and Everything..." << std::endl;
    QThread::msleep(300);
    std::cout << s_runBlockingStdOutSubProcessMagicWord << "...Now wait for the question...";
    if (qEnvironmentVariable(envVar()) == "true")
        std::cout << std::endl;
    else
        std::cout << std::flush; // otherwise it won't reach the original process (will be buffered)
    QThread::msleep(5000);
    return 0;
}

int ProcessTestApp::LineCallback::main()
{
#ifdef Q_OS_WIN
    // Prevent \r\n -> \r\r\n translation.
    _setmode(_fileno(stderr), O_BINARY);
#endif
    fprintf(stderr, "%s", QByteArray(s_lineCallbackData).replace('|', "").data());
    return 0;
}

int ProcessTestApp::StandardOutputAndErrorWriter::main()
{
    std::cout << s_outputData << std::endl;
    std::cerr << s_errorData << std::endl;
    return 0;
}

int ProcessTestApp::ChannelForwarding::main()
{
    const QProcess::ProcessChannelMode channelMode
            = QProcess::ProcessChannelMode(qEnvironmentVariableIntValue(envVar()));
    qunsetenv(envVar());

    SubProcessConfig subConfig(StandardOutputAndErrorWriter::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);

    process.setProcessChannelMode(channelMode);
    process.start();
    process.waitForFinished();
    return 0;
}

int ProcessTestApp::BlockingProcess::main()
{
    std::cout << "Blocking process successfully executed." << std::endl;
    const BlockType blockType = BlockType(qEnvironmentVariableIntValue(envVar()));
    bool dummy = true;
    switch (blockType) {
    case BlockType::EndlessLoop:
        while (true) {
            if (dummy) {
                // Note: Keep these lines, otherwise the compiler may optimize out the empty loop.
                std::cout << "EndlessLoop started" << std::endl;
                dummy = false;
            }
        }
        break;
    case BlockType::InfiniteSleep:
        QThread::sleep(INT_MAX);
        break;
    case BlockType::MutexDeadlock: {
        QMutex mutex;
        mutex.lock();
        mutex.lock();
        break;
    }
    case BlockType::EventLoop: {
        QEventLoop loop;
        loop.exec();
        break;
    }
    }
    return 1;
}

int ProcessTestApp::Crash::main()
{
    doCrash();
    return 1;
}

int ProcessTestApp::CrashAfterOneSecond::main()
{
    QThread::sleep(1);
    doCrash();
    return 1;
}

int ProcessTestApp::RecursiveCrashingProcess::main()
{
    const int currentDepth = qEnvironmentVariableIntValue(envVar());
    if (currentDepth == 1) {
        QThread::sleep(1);
        doCrash();
        return 1;
    }
    SubProcessConfig subConfig(envVar(), QString::number(currentDepth - 1));
    Process process;
    subConfig.setupSubProcess(&process);
    process.start();
    process.waitForFinished();
    if (process.exitStatus() == QProcess::NormalExit)
        return process.exitCode();
    return s_crashCode;
}

#ifndef Q_OS_WIN
static std::atomic_bool s_terminate = false;

void terminate(int signum)
{
    Q_UNUSED(signum)
    s_terminate.store(true);
}
#endif

int ProcessTestApp::RecursiveBlockingProcess::main()
{
#ifndef Q_OS_WIN
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate;
    sigaction(SIGTERM, &action, NULL);
#endif

    const int currentDepth = qEnvironmentVariableIntValue(envVar());
    if (currentDepth == 1) {
        std::cout << s_leafProcessStarted << std::flush;
        while (true) {
            // TODO: make it configurable so that we could test the reaper timeout
            QThread::msleep(10);
#ifndef Q_OS_WIN
            if (s_terminate.load()) {
                std::cout << s_leafProcessTerminated << std::flush;
                return s_crashCode;
            }
#endif
        }
    }
    SubProcessConfig subConfig(envVar(), QString::number(currentDepth - 1));
    Process process;
    subConfig.setupSubProcess(&process);
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start();
    while (true) {
        if (process.waitForFinished(10))
            return 0;
#ifndef Q_OS_WIN
        if (s_terminate.load()) {
            process.terminate();
            process.waitForFinished(-1);
            break;
        }
#endif
    }
    if (process.exitStatus() == QProcess::NormalExit)
        return process.exitCode();
    return s_crashCode;
}
