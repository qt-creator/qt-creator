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

#include "localqmlprofilerrunner_test.h"
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <QtTest>
#include <QTcpServer>

namespace QmlProfiler {
namespace Internal {

LocalQmlProfilerRunnerTest::LocalQmlProfilerRunnerTest(QObject *parent) : QObject(parent)
{
}

void LocalQmlProfilerRunnerTest::connectRunner(LocalQmlProfilerRunner *runner)
{
    connect(runner, &LocalQmlProfilerRunner::started, this, [this] {
        QVERIFY(!running);
        ++runCount;
        running = true;
    });
    connect(runner, &LocalQmlProfilerRunner::stopped, this, [this] {
        QVERIFY(running);
        running = false;
    });
}

void LocalQmlProfilerRunnerTest::testRunner()
{
    configuration.debuggee.executable = "\\-/|\\-/";
    configuration.debuggee.environment = Utils::Environment::systemEnvironment();

    // should not be used anywhere but cannot be empty
    configuration.socket = connection.analyzerSocket = QString("invalid");

    rc = Debugger::createAnalyzerRunControl(
                nullptr, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    rc->setConnection(connection);
    auto runner = new LocalQmlProfilerRunner(configuration, rc);
    connectRunner(runner);

    rc->initiateStart();
    QTimer::singleShot(0, this, &LocalQmlProfilerRunnerTest::testRunner1);
}

void LocalQmlProfilerRunnerTest::testRunner1()
{
    QTRY_COMPARE_WITH_TIMEOUT(runCount, 1, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);

    configuration.socket = connection.analyzerSocket = LocalQmlProfilerRunner::findFreeSocket();
    configuration.debuggee.executable = qApp->applicationFilePath();

    // comma is used to specify a test function. In this case, an invalid one.
    configuration.debuggee.commandLineArguments = QString("-test QmlProfiler,");

    delete rc;
    rc = Debugger::createAnalyzerRunControl(
                nullptr, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    rc->setConnection(connection);
    auto runner = new LocalQmlProfilerRunner(configuration, rc);
    connectRunner(runner);
    rc->initiateStart();
    QTimer::singleShot(0, this, &LocalQmlProfilerRunnerTest::testRunner2);
}

void LocalQmlProfilerRunnerTest::testRunner2()
{
    QTRY_COMPARE_WITH_TIMEOUT(runCount, 2, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);

    delete rc;

    configuration.debuggee.commandLineArguments.clear();
    configuration.socket.clear();
    connection.analyzerSocket.clear();
    configuration.port = connection.analyzerPort =
            LocalQmlProfilerRunner::findFreePort(connection.analyzerHost);
    rc = Debugger::createAnalyzerRunControl(
                nullptr, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    rc->setConnection(connection);
    auto runner = new LocalQmlProfilerRunner(configuration, rc);
    connectRunner(runner);
    rc->initiateStart();

    QTimer::singleShot(0, this, &LocalQmlProfilerRunnerTest::testRunner3);
}

void LocalQmlProfilerRunnerTest::testRunner3()
{
    QTRY_COMPARE_WITH_TIMEOUT(runCount, 3, 10000);
    rc->initiateStop();
    QTimer::singleShot(0, this, &LocalQmlProfilerRunnerTest::testRunner4);
}

void LocalQmlProfilerRunnerTest::testRunner4()
{
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);
    delete rc;
}

void LocalQmlProfilerRunnerTest::testFindFreePort()
{
    QString host;
    Utils::Port port = LocalQmlProfilerRunner::findFreePort(host);
    QVERIFY(port.isValid());
    QVERIFY(!host.isEmpty());
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress(host), port.number()));
}

void LocalQmlProfilerRunnerTest::testFindFreeSocket()
{
    QString socket = LocalQmlProfilerRunner::findFreeSocket();
    QVERIFY(!socket.isEmpty());
    QVERIFY(!QFile::exists(socket));
    QFile file(socket);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
}


} // namespace Internal
} // namespace QmlProfiler
