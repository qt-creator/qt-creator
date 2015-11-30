/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "testrunner.h"

#include <valgrind/xmlprotocol/frame.h>
#include <valgrind/xmlprotocol/stack.h>
#include <valgrind/xmlprotocol/suppression.h>
#include <valgrind/xmlprotocol/threadedparser.h>
#include <valgrind/xmlprotocol/parser.h>
#include <valgrind/memcheck/memcheckrunner.h>

#include <projectexplorer/projectexplorer.h>
#include <extensionsystem/pluginmanager.h>

#include <QDebug>
#include <QTest>
#include <QDir>
#include <QSignalSpy>

const QString appSrcDir(TESTRUNNER_SRC_DIR);
const QString appBinDir(TESTRUNNER_APP_DIR);

#define HEADER_LENGTH 30

bool on64bit()
{
    return sizeof(char*) == 8;
}

QString srcDirForApp(const QString &app)
{
    return appSrcDir + QLatin1Char('/') + app;
}

QTEST_MAIN(Valgrind::TestRunner)

using namespace Valgrind;
using namespace Valgrind::XmlProtocol;
using namespace Valgrind::Memcheck;

//BEGIN Test Helpers and boilerplate code

TestRunner::TestRunner(QObject *parent)
    : QObject(parent),
      m_parser(0),
      m_runner(0)
{
    qRegisterMetaType<Error>();
}

QString TestRunner::runTestBinary(const QString &binary, const QStringList &vArgs)
{
    const QString binPath = appBinDir + QLatin1Char('/') + binary;
    if (!QFileInfo(binPath).isExecutable())
        qFatal("No such test app: %s", qPrintable(binPath));
    m_runner->setValgrindArguments(QStringList() << "--num-callers=50" << "--track-origins=yes" << vArgs);
    m_runner->setDebuggeeExecutable(binPath);
    m_runner->start();
    m_runner->waitForFinished();
    return binPath;
}

void TestRunner::logMessageReceived(const QByteArray &message)
{
    qDebug() << "log message received:" << message;
    m_logMessages << message;
}

void TestRunner::internalError(const QString &error)
{
    if (!m_expectCrash)
        QFAIL(qPrintable(error));
    else
        qDebug() << "expected crash:" << error;
}

void TestRunner::error(const Error &error)
{
    m_errors << error;
}

void TestRunner::cleanup()
{
    Q_ASSERT(m_runner);
    delete m_runner;
    m_runner = 0;
    Q_ASSERT(m_parser);
    delete m_parser;
    m_parser = 0;

    m_logMessages.clear();
    m_errors.clear();
    m_expectCrash = false;
}

void TestRunner::initTestCase()
{
    new ExtensionSystem::PluginManager;
    new ProjectExplorer::ProjectExplorerPlugin;
}

void TestRunner::init()
{
    Q_ASSERT(m_logMessages.isEmpty());

    Q_ASSERT(!m_runner);
    m_runner = new MemcheckRunner;
    m_runner->setValgrindExecutable(QLatin1String("valgrind"));
    m_runner->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(m_runner, &MemcheckRunner::logMessageReceived,
            this, &TestRunner::logMessageReceived);
    connect(m_runner, &ValgrindRunner::processErrorReceived,
            this, &TestRunner::internalError);
    Q_ASSERT(!m_parser);
    m_parser = new ThreadedParser;
    connect(m_parser, &ThreadedParser::internalError,
            this, &TestRunner::internalError);
    connect(m_parser, &ThreadedParser::error,
            this, &TestRunner::error);

    m_runner->setParser(m_parser);
}

//BEGIN: Actual test cases

void TestRunner::testLeak1()
{
    const QString binary = runTestBinary(QLatin1String("leak1/leak1"));

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

void TestRunner::testLeak2()
{
    const QString binary = runTestBinary(QLatin1String("leak2/leak2"));

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

void TestRunner::testLeak3()
{
    const QString binary = runTestBinary(QLatin1String("leak3/leak3"), QStringList() << "--show-reachable=yes");

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

void TestRunner::testLeak4()
{
    const QString app("leak4");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app,
                                         QStringList() << "--show-reachable=yes");
    const QString srcDir = srcDirForApp("leak4");

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 2);
    //BEGIN first error
    {
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(Leak_IndirectlyLost));
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
    const Error error = m_errors.last();
    QCOMPARE(error.kind(), int(Leak_DefinitelyLost));
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

void TestRunner::uninit1()
{
    const QString app("uninit1");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
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

void TestRunner::uninit2()
{
    const QString app("uninit2");
    m_expectCrash = true;
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
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

void TestRunner::uninit3()
{
    const QString app("uninit3");
    m_expectCrash = true;
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
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

void TestRunner::syscall()
{
    const QString app("syscall");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
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

void TestRunner::free1()
{
    const QString app("free1");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    const QString srcDir = srcDirForApp(app);

    QVERIFY(m_logMessages.isEmpty());

    QCOMPARE(m_errors.count(), 1);
    const Error error = m_errors.first();
    QCOMPARE(error.kind(), int(InvalidFree));
    QCOMPARE(error.stacks().count(), 2);
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
    const Stack stack = error.stacks().last();
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

void TestRunner::free2()
{
    const QString app("free2");
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
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

void TestRunner::invalidjump()
{
    const QString app("invalidjump");
    m_expectCrash = true;
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
    const QString srcDir = srcDirForApp(app);

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


void TestRunner::overlap()
{
    const QString app("overlap");
    m_expectCrash = true;
    const QString binary = runTestBinary(app + QLatin1Char('/') + app);
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
