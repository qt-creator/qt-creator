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

#include "../qmlprofilerruncontrol.h"

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/runnables.h>

#include <QtTest>
#include <QTcpServer>

using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

LocalQmlProfilerRunnerTest::LocalQmlProfilerRunnerTest(QObject *parent) : QObject(parent)
{
}

void LocalQmlProfilerRunnerTest::start()
{
    delete runControl;
    runControl = new ProjectExplorer::RunControl(nullptr, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    runControl->setRunnable(debuggee);
    auto runner = new LocalQmlProfilerSupport(runControl, serverUrl);

    connect(runner, &LocalQmlProfilerSupport::localRunnerStarted, this, [this] {
        QVERIFY(!running);
        ++runCount;
        running = true;
    });
    connect(runner, &LocalQmlProfilerSupport::localRunnerStopped, this, [this] {
        QVERIFY(running);
        running = false;
    });
    runControl->initiateStart();
}

void LocalQmlProfilerRunnerTest::testRunner()
{
    debuggee.executable = "\\-/|\\-/";
    debuggee.environment = Utils::Environment::systemEnvironment();

    // should not be used anywhere but cannot be empty
    serverUrl.setPath("invalid");

    start();

    QTimer::singleShot(0, this, &LocalQmlProfilerRunnerTest::testRunner1);
}

void LocalQmlProfilerRunnerTest::testRunner1()
{
    QTRY_COMPARE_WITH_TIMEOUT(runCount, 1, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);

    serverUrl = urlFromLocalSocket();

    debuggee.executable = QCoreApplication::applicationFilePath();
    // comma is used to specify a test function. In this case, an invalid one.
    debuggee.commandLineArguments = QString("-test QmlProfiler,");

    start();

    QTimer::singleShot(0, this, &LocalQmlProfilerRunnerTest::testRunner2);
}

void LocalQmlProfilerRunnerTest::testRunner2()
{
    QTRY_COMPARE_WITH_TIMEOUT(runCount, 2, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);

    debuggee.commandLineArguments.clear();
    serverUrl = urlFromLocalHostAndFreePort();

    start();

    QTimer::singleShot(0, this, &LocalQmlProfilerRunnerTest::testRunner3);
}

void LocalQmlProfilerRunnerTest::testRunner3()
{
    QTRY_COMPARE_WITH_TIMEOUT(runCount, 3, 10000);
    runControl->initiateStop();
    QTimer::singleShot(0, this, &LocalQmlProfilerRunnerTest::testRunner4);
}

void LocalQmlProfilerRunnerTest::testRunner4()
{
    QTRY_VERIFY_WITH_TIMEOUT(!running, 10000);
    delete runControl;
}

void LocalQmlProfilerRunnerTest::testFindFreePort()
{
    QUrl serverUrl = urlFromLocalHostAndFreePort();
    QVERIFY(serverUrl.port() != -1);
    QVERIFY(!serverUrl.host().isEmpty());
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress(serverUrl.host()), serverUrl.port()));
}

void LocalQmlProfilerRunnerTest::testFindFreeSocket()
{
    QUrl serverUrl = urlFromLocalSocket();
    QString socket = serverUrl.path();
    QVERIFY(!socket.isEmpty());
    QVERIFY(!QFile::exists(socket));
    QFile file(socket);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
}

} // namespace Internal
} // namespace QmlProfiler
