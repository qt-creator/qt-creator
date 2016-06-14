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
#include <debugger/analyzer/analyzerruncontrol.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <utils/hostosinfo.h>
#include <QtTest>
#include <QTcpServer>

namespace QmlProfiler {
namespace Internal {

LocalQmlProfilerRunnerTest::LocalQmlProfilerRunnerTest(QObject *parent) : QObject(parent)
{
}

void LocalQmlProfilerRunnerTest::testRunner()
{
    if (Utils::HostOsInfo::isWindowsHost())
        QSKIP("This test is disabled on Windows as it produces a blocking dialog.");

    LocalQmlProfilerRunner::Configuration configuration;
    Debugger::AnalyzerRunControl *rc = Debugger::createAnalyzerRunControl(
                nullptr, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    rc->setConnection(Debugger::AnalyzerConnection());
    auto runner = new LocalQmlProfilerRunner(configuration, rc);

    bool running = false;
    int runCount = 0;
    int errors = 0;

    auto connectRunner = [&]() {
        connect(runner, &LocalQmlProfilerRunner::started, this, [&running, &runCount](){
            QVERIFY(!running);
            ++runCount;
            running = true;
        });
        connect(runner, &LocalQmlProfilerRunner::stopped, this, [&running](){
            QVERIFY(running);
            running = false;
        });

        connect(runner, &LocalQmlProfilerRunner::appendMessage, this,
                [&errors](const QString &message, Utils::OutputFormat format) {
            Q_UNUSED(message);
            if (format == Utils::ErrorMessageFormat)
                ++errors;
        });
    };

    connectRunner();

    rc->start();

    QTRY_COMPARE_WITH_TIMEOUT(runCount, 1, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);
    QCOMPARE(errors, 1);

    configuration.debuggee.environment = Utils::Environment::systemEnvironment();
    configuration.debuggee.executable = qApp->applicationFilePath();
    configuration.debuggee.commandLineArguments = QString("-version");

    delete rc;
    rc = Debugger::createAnalyzerRunControl(
                nullptr, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    rc->setConnection(Debugger::AnalyzerConnection());
    runner = new LocalQmlProfilerRunner(configuration, rc);
    connectRunner();
    rc->start();

    QTRY_COMPARE_WITH_TIMEOUT(runCount, 2, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);
    QCOMPARE(errors, 1);

    delete rc;

    configuration.debuggee.commandLineArguments.clear();
    rc = Debugger::createAnalyzerRunControl(
                nullptr, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    rc->setConnection(Debugger::AnalyzerConnection());
    runner = new LocalQmlProfilerRunner(configuration, rc);
    connectRunner();
    rc->start();

    QTRY_COMPARE_WITH_TIMEOUT(runCount, 3, 10000);
    QTest::qWait(1000);
    QVERIFY(running); // verify it doesn't spontaneously stop
    QCOMPARE(errors, 1);

    rc->stop();
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);
    QCOMPARE(errors, 2); // "The program has unexpectedly finished."

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
