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
#include <projectexplorer/runnables.h>
#include <qmlprofiler/qmlprofilerruncontrol.h>

#include <utils/url.h>

#include <QtTest>
#include <QTcpServer>

namespace QmlProfiler {
namespace Internal {

LocalQmlProfilerRunnerTest::LocalQmlProfilerRunnerTest(QObject *parent) : QObject(parent)
{
}

void LocalQmlProfilerRunnerTest::testRunner()
{
    QPointer<ProjectExplorer::RunControl> runControl;
    QPointer<LocalQmlProfilerSupport> profiler;
    ProjectExplorer::StandardRunnable debuggee;
    QUrl serverUrl;

    bool running = false;
    bool started = false;
    int startCount = 0;
    int runCount = 0;
    int stopCount = 0;

    debuggee.executable = "\\-/|\\-/";
    debuggee.environment = Utils::Environment::systemEnvironment();

    // should not be used anywhere but cannot be empty
    serverUrl.setScheme(Utils::urlSocketScheme());
    serverUrl.setPath("invalid");

    runControl = new ProjectExplorer::RunControl(nullptr,
                                                 ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    runControl->setRunnable(debuggee);
    profiler = new LocalQmlProfilerSupport(runControl, serverUrl);

    auto connectRunner = [&]() {
        connect(runControl, &ProjectExplorer::RunControl::aboutToStart, this, [&]() {
            QVERIFY(!started);
            QVERIFY(!running);
            ++startCount;
            started = true;
        });
        connect(runControl, &ProjectExplorer::RunControl::started, this, [&]() {
            QVERIFY(started);
            QVERIFY(!running);
            ++runCount;
            running = true;
        });
        connect(runControl, &ProjectExplorer::RunControl::stopped, this, [&]() {
            QVERIFY(started);
            ++stopCount;
            running = false;
            started = false;
        });
        connect(runControl, &ProjectExplorer::RunControl::finished, this, [&]() {
            running = false;
            started = false;
        });
    };

    connectRunner();

    runControl->initiateStart();

    QTRY_COMPARE_WITH_TIMEOUT(startCount, 1, 30000);
    QTRY_VERIFY_WITH_TIMEOUT(!started, 30000);
    QCOMPARE(stopCount, 1);
    QCOMPARE(runCount, 0);

    runControl->initiateFinish();
    QTRY_VERIFY(runControl.isNull());
    QVERIFY(profiler.isNull());

    serverUrl = Utils::urlFromLocalSocket();
    debuggee.executable = qApp->applicationFilePath();

    // comma is used to specify a test function. In this case, an invalid one.
    debuggee.commandLineArguments = QString("-test QmlProfiler,");
    runControl = new ProjectExplorer::RunControl(nullptr,
                                                 ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    runControl->setRunnable(debuggee);
    profiler = new LocalQmlProfilerSupport(runControl, serverUrl);
    connectRunner();
    runControl->initiateStart();

    QTRY_VERIFY_WITH_TIMEOUT(running, 30000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 30000);
    QCOMPARE(startCount, 2);
    QCOMPARE(stopCount, 2);
    QCOMPARE(runCount, 1);

    runControl->initiateFinish();
    QTRY_VERIFY(runControl.isNull());
    QVERIFY(profiler.isNull());

    debuggee.commandLineArguments.clear();
    serverUrl.clear();
    serverUrl = Utils::urlFromLocalHostAndFreePort();
    runControl = new ProjectExplorer::RunControl(nullptr,
                                                 ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    runControl->setRunnable(debuggee);
    profiler = new LocalQmlProfilerSupport(runControl, serverUrl);
    connectRunner();
    runControl->initiateStart();

    QTRY_VERIFY_WITH_TIMEOUT(running, 30000);
    runControl->initiateStop();
    QTRY_VERIFY_WITH_TIMEOUT(!running, 30000);
    QCOMPARE(startCount, 3);
    QCOMPARE(stopCount, 3);
    QCOMPARE(runCount, 2);

    runControl->initiateFinish();
    QTRY_VERIFY(runControl.isNull());
    QVERIFY(profiler.isNull());
}

void LocalQmlProfilerRunnerTest::testFindFreePort()
{
    QUrl serverUrl = Utils::urlFromLocalHostAndFreePort();
    QVERIFY(serverUrl.port() != -1);
    QVERIFY(!serverUrl.host().isEmpty());
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress(serverUrl.host()), serverUrl.port()));
}

void LocalQmlProfilerRunnerTest::testFindFreeSocket()
{
    QUrl serverUrl = Utils::urlFromLocalSocket();
    QString socket = serverUrl.path();
    QVERIFY(!socket.isEmpty());
    QVERIFY(!QFile::exists(socket));
    QFile file(socket);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
}

} // namespace Internal
} // namespace QmlProfiler
