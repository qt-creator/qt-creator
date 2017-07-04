/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindtestrunnertest.h"

#include "xmlprotocol/frame.h"
#include "xmlprotocol/stack.h"
#include "xmlprotocol/suppression.h"
#include "xmlprotocol/threadedparser.h"
#include "xmlprotocol/parser.h"
#include "valgrindrunner.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runnables.h>

#include <utils/algorithm.h>

#include <QDebug>
#include <QTest>
#include <QDir>
#include <QSignalSpy>

#define HEADER_LENGTH 25

using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Test {

//BEGIN Test Helpers and boilerplate code

static const QString appSrcDir(TESTRUNNER_SRC_DIR);
static const QString appBinDir(TESTRUNNER_APP_DIR);

static bool on64bit()
{
    return sizeof(char*) == 8;
}

static QString srcDirForApp(const QString &app)
{
    return QDir::cleanPath(appSrcDir + QLatin1Char('/') + app);
}

ValgrindTestRunnerTest::ValgrindTestRunnerTest(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<Error>();
}

QString ValgrindTestRunnerTest::runTestBinary(const QString &binary, const QStringList &vArgs)
{
    const QFileInfo binPathFileInfo(appBinDir, binary);
    if (!binPathFileInfo.isExecutable())
        return QString();
    ProjectExplorer::StandardRunnable debuggee;
    const QString &binPath = binPathFileInfo.canonicalFilePath();
    debuggee.executable = binPath;
    debuggee.environment = Utils::Environment::systemEnvironment();
    m_runner->setLocalServerAddress(QHostAddress::LocalHost);
    m_runner->setValgrindArguments(QStringList() << "--num-callers=50" << "--track-origins=yes" << vArgs);
    m_runner->setDebuggee(debuggee);
    m_runner->setDevice(ProjectExplorer::DeviceManager::instance()->defaultDevice(
                            Core::Id(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)));
    m_runner->start();
    m_runner->waitForFinished();
    return binPath;
}

void ValgrindTestRunnerTest::logMessageReceived(const QByteArray &message)
{
    qDebug() << "log message received:" << message;
    m_logMessages << message;
}

void ValgrindTestRunnerTest::internalError(const QString &error)
{
    if (!m_expectCrash)
        QFAIL(qPrintable(error));
    else
        qDebug() << "expected crash:" << error;
}

void ValgrindTestRunnerTest::error(const Error &error)
{
    m_errors << error;
}

void ValgrindTestRunnerTest::cleanup()
{
    Q_ASSERT(m_runner);
    delete m_runner;
    m_runner = 0;

    m_logMessages.clear();
    m_errors.clear();
    m_expectCrash = false;
}

void ValgrindTestRunnerTest::init()
{
    const Utils::Environment &sysEnv = Utils::Environment::systemEnvironment();
    auto fileName = sysEnv.searchInPath("valgrind");
    if (fileName.isEmpty())
        QSKIP("This test needs valgrind in PATH");
    Q_ASSERT(m_logMessages.isEmpty());

    Q_ASSERT(!m_runner);
    m_runner = new ValgrindRunner;
    m_runner->setValgrindExecutable("valgrind");
    m_runner->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(m_runner, &ValgrindRunner::logMessageReceived,
            this, &ValgrindTestRunnerTest::logMessageReceived);
    connect(m_runner, &ValgrindRunner::processErrorReceived,
            this, &ValgrindTestRunnerTest::internalError);
    connect(m_runner->parser(), &ThreadedParser::internalError,
            this, &ValgrindTestRunnerTest::internalError);
    connect(m_runner->parser(), &ThreadedParser::error,
            this, &ValgrindTestRunnerTest::error);
}

//BEGIN: Actual test cases

void ValgrindTestRunnerTest::testLeak1()
{
    const QString binary = runTestBinary(QLatin1String("leak1/leak1"));
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(Leak_DefinitelyLost));
    QCOMPARE(error.leakedBlocks(), qint64(1));
    QCOMPARE(error.leakedBytes(), quint64(8));
    QCOMPARE(error.stacks().count(), 1);
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 2);
    {
        const Frame frame = stack.frames().at(0);
        if (on64bit())
            QCOMPARE(frame.functionName(), QLatin1String("operator new(unsigned long)"));
        else
            QCOMPARE(frame.functionName(), QLatin1String("operator new(unsigned int)"));
    }
    {
        const Frame frame = stack.frames().at(1);
        QCOMPARE(frame.functionName(), QLatin1String("main"));
        QCOMPARE(frame.line(), 5 + HEADER_LENGTH);

        QCOMPARE(frame.object(), binary);
        QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
        QCOMPARE(QDir::cleanPath(frame.directory()), srcDirForApp("leak1"));
    }
}

void ValgrindTestRunnerTest::testLeak2()
{
    const QString binary = runTestBinary(QLatin1String("leak2/leak2"));
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");

    QVERIFY(m_logMessages.isEmpty());
    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(Leak_PossiblyLost));
    QCOMPARE(error.leakedBlocks(), qint64(1));
    QCOMPARE(error.leakedBytes(), quint64(5));
    QCOMPARE(error.stacks().count(), 1);
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 3);
    {
        const Frame frame = stack.frames().at(0);
        QCOMPARE(frame.functionName(), QLatin1String("malloc"));
    }
    {
        const Frame frame = stack.frames().at(1);
        QCOMPARE(frame.functionName(), QLatin1String("strdup"));
    }
    {
        const Frame frame = stack.frames().at(2);
        if (on64bit()) {
            QCOMPARE(frame.functionName(), QLatin1String("main"));
            QCOMPARE(frame.line(), 7 + HEADER_LENGTH);

            QCOMPARE(frame.object(), binary);
            QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
            QCOMPARE(QDir::cleanPath(frame.directory()), srcDirForApp("leak2"));
        } else {
           QCOMPARE(frame.functionName(), QLatin1String("(below main)"));
        }
    }
}

void ValgrindTestRunnerTest::testLeak3()
{
    const QString binary = runTestBinary(QLatin1String("leak3/leak3"), QStringList() << "--show-reachable=yes");
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(Leak_StillReachable));
    QCOMPARE(error.leakedBlocks(), qint64(1));
    QCOMPARE(error.leakedBytes(), quint64(5));
    QCOMPARE(error.stacks().count(), 1);
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 3);
    {
        const Frame frame = stack.frames().at(0);
        QCOMPARE(frame.functionName(), QLatin1String("malloc"));
    }
    {
        const Frame frame = stack.frames().at(1);
        QCOMPARE(frame.functionName(), QLatin1String("strdup"));
    }
    {
        const Frame frame = stack.frames().at(2);
        if (on64bit()) {
            QCOMPARE(frame.functionName(), QLatin1String("main"));
            QCOMPARE(frame.line(), 7 + HEADER_LENGTH);

            QCOMPARE(frame.object(), binary);
            QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
            QCOMPARE(QDir::cleanPath(frame.directory()), srcDirForApp("leak3"));
        } else {
            QCOMPARE(frame.functionName(), QLatin1String("(below main)"));
        }
    }
}

void ValgrindTestRunnerTest::testLeak4()
{
    const QString app("leak4");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app,
                                         QStringList() << "--show-reachable=yes");
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    const QString srcDir = srcDirForApp("leak4");

    QVERIFY(m_logMessages.isEmpty());

    QVERIFY(m_errors.count() >= 3);
    //BEGIN first error
    {
    // depending on the valgrind version the errors can be different - try to find the correct one
    const Error error = Utils::findOrDefault(m_errors, [](const Error &err) {
        return err.kind() == Leak_IndirectlyLost;
    });

    QCOMPARE(error.leakedBlocks(), qint64(1));
    QCOMPARE(error.leakedBytes(), quint64(8));
    QCOMPARE(error.stacks().count(), 1);
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 3);
    {
        const Frame frame = stack.frames().at(0);
        if (on64bit())
            QCOMPARE(frame.functionName(), QLatin1String("operator new(unsigned long)"));
        else
            QCOMPARE(frame.functionName(), QLatin1String("operator new(unsigned int)"));
    }
    {
        const Frame frame = stack.frames().at(1);
        QCOMPARE(frame.functionName(), QLatin1String("Foo::Foo()"));
        QCOMPARE(frame.line(), 6 + HEADER_LENGTH);

        QCOMPARE(frame.object(), binary);
        QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
        QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    {
        const Frame frame = stack.frames().at(2);
        QCOMPARE(frame.functionName(), QLatin1String("main"));
        QCOMPARE(frame.line(), 14 + HEADER_LENGTH);

        QCOMPARE(frame.object(), binary);
        QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
        QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    }
    //BEGIN second error
    {
    const Error error = Utils::findOrDefault(m_errors, [](const Error &err) {
        return err.kind() == Leak_DefinitelyLost;
    });

    QCOMPARE(error.leakedBlocks(), qint64(1));
    if (on64bit())
        QCOMPARE(error.leakedBytes(), quint64(16));
    else
        QCOMPARE(error.leakedBytes(), quint64(12));
    QCOMPARE(error.stacks().count(), 1);
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 2);
    {
        const Frame frame = stack.frames().at(0);
        if (on64bit())
            QCOMPARE(frame.functionName(), QLatin1String("operator new(unsigned long)"));
        else
            QCOMPARE(frame.functionName(), QLatin1String("operator new(unsigned int)"));
    }
    {
        const Frame frame = stack.frames().at(1);
        QCOMPARE(frame.functionName(), QLatin1String("main"));
        QCOMPARE(frame.line(), 14 + HEADER_LENGTH);

        QCOMPARE(frame.object(), binary);
        QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
        QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    }
}

void ValgrindTestRunnerTest::testUninit1()
{
    const QString app("uninit1");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    const QString srcDir = srcDirForApp(app);

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(UninitCondition));
    QCOMPARE(error.stacks().count(), 2);
    //BEGIN first stack
    {
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 4 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    //BEGIN second stack
    {
    const Stack stack = error.stacks().last();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 2 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
}

void ValgrindTestRunnerTest::testUninit2()
{
    const QString app("uninit2");
    m_expectCrash = true;
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    const QString srcDir = srcDirForApp(app);

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 2);
    //BEGIN first error
    {
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(UninitValue));
    QCOMPARE(error.stacks().count(), 2);
    //BEGIN first stack
    {
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 4 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    //BEGIN second stack
    {
    const Stack stack = error.stacks().last();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 2 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    }
    //BEGIN second error
    {
    const Error error = m_errors.last();
    QCOMPARE(error.kind(), int(InvalidWrite));
    QCOMPARE(error.stacks().count(), 1);

    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 4 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
}

void ValgrindTestRunnerTest::testUninit3()
{
    const QString app("uninit3");
    m_expectCrash = true;
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    const QString srcDir = srcDirForApp(app);

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 2);
    //BEGIN first error
    {
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(UninitValue));
    QCOMPARE(error.stacks().count(), 2);
    //BEGIN first stack
    {
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 4 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    //BEGIN second stack
    {
    const Stack stack = error.stacks().last();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 2 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    }
    //BEGIN second error
    {
    const Error error = m_errors.last();
    QCOMPARE(error.kind(), int(InvalidRead));
    QCOMPARE(error.stacks().count(), 1);

    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 4 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
}

void ValgrindTestRunnerTest::testSyscall()
{
    const QString app("syscall");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    const QString srcDir = srcDirForApp(app);

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(SyscallParam));
    QCOMPARE(error.stacks().count(), 2);
    //BEGIN first stack
    {
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    if (on64bit()) {
        QCOMPARE(stack.frames().count(), 4);

        {
            const Frame frame = stack.frames().at(0);
            QCOMPARE(frame.functionName(), QLatin1String("_Exit"));
        }
        {
            const Frame frame = stack.frames().at(1);
            QCOMPARE(frame.functionName(), QLatin1String("__run_exit_handlers"));
        }
        {
            const Frame frame = stack.frames().at(2);
            QCOMPARE(frame.functionName(), QLatin1String("exit"));
        }
        {
            const Frame frame = stack.frames().at(3);
            QCOMPARE(frame.functionName(), QLatin1String("(below main)"));
        }
    } else {
        QCOMPARE(stack.frames().count(), 1);
        {
            const Frame frame = stack.frames().at(0);
            QCOMPARE(frame.functionName(), QLatin1String("_Exit"));
        }
    }
    }
    //BEGIN second stack
    {
    const Stack stack = error.stacks().last();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 1);

    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 2 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
}

void ValgrindTestRunnerTest::testFree1()
{
    const QString app("free1");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    const QString srcDir = srcDirForApp(app);

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(InvalidFree));
    QVERIFY(error.stacks().count() >= 2);
    //BEGIN first stack
    {
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 2);

    {
    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("operator delete(void*)"));
    }
    {
    const Frame frame = stack.frames().last();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 7 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    }
    //BEGIN second stack
    {
    const Stack stack = error.stacks().at(1);
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 2);

    {
    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("operator delete(void*)"));
    }
    {
    const Frame frame = stack.frames().last();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 6 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    }
}

void ValgrindTestRunnerTest::testFree2()
{
    const QString app("free2");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    const QString srcDir = srcDirForApp(app);

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(MismatchedFree));
    QCOMPARE(error.stacks().count(), 2);
    //BEGIN first stack
    {
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 2);

    {
    const Frame frame = stack.frames().first();
    QCOMPARE(frame.functionName(), QLatin1String("free"));
    }
    {
    const Frame frame = stack.frames().last();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 6 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    }
    //BEGIN second stack
    {
    const Stack stack = error.stacks().last();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 2);


    {
    const Frame frame = stack.frames().first();
    if (on64bit())
        QCOMPARE(frame.functionName(), QLatin1String("operator new(unsigned long)"));
    else
        QCOMPARE(frame.functionName(), QLatin1String("operator new(unsigned int)"));
    }
    {
    const Frame frame = stack.frames().last();
    QCOMPARE(frame.functionName(), QLatin1String("main"));
    QCOMPARE(frame.line(), 5 + HEADER_LENGTH);

    QCOMPARE(frame.object(), binary);
    QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
    QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
    }
}

void ValgrindTestRunnerTest::testInvalidjump()
{
    const QString app("invalidjump");
    m_expectCrash = true;
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(InvalidJump));
    QCOMPARE(error.stacks().count(), 1);
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 2);
    QVERIFY(!stack.auxWhat().isEmpty());
    {
        const Frame frame = stack.frames().at(0);
        QCOMPARE(frame.instructionPointer(), quint64(0));
    }
    {
        const Frame frame = stack.frames().at(1);
        QCOMPARE(frame.functionName(), QLatin1String("(below main)"));
    }
}

void ValgrindTestRunnerTest::testOverlap()
{
    const QString app("overlap");
    m_expectCrash = true;
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    if (binary.isEmpty())
        QSKIP("You need to pass BUILD_TESTS when building Qt Creator or build valgrind testapps "
              "manually before executing this test.");
    const QString srcDir = srcDirForApp(app);

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(Overlap));
    QCOMPARE(error.stacks().count(), 1);
    const Stack stack = error.stacks().first();
    QCOMPARE(stack.line(), qint64(-1));
    QCOMPARE(stack.frames().count(), 2);
    {
        const Frame frame = stack.frames().at(0);
        QVERIFY(frame.functionName().startsWith(QLatin1String("memcpy")));
    }
    {
        const Frame frame = stack.frames().last();
        QCOMPARE(frame.functionName(), QLatin1String("main"));
        QCOMPARE(frame.line(), 6 + HEADER_LENGTH);

        QCOMPARE(frame.object(), binary);
        QCOMPARE(frame.fileName(), QLatin1String("main.cpp"));
        QCOMPARE(QDir::cleanPath(frame.directory()), srcDir);
    }
}

} // namespace Test
} // namespace Valgrind
