// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "localqmlprofilerrunner_test.h"

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <qmlprofiler/qmlprofilerruncontrol.h>
#include <qmlprofiler/qmlprofilertool.h>

#include <utils/url.h>
#include <utils/temporaryfile.h>

#include <QtTest>
#include <QTcpServer>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProfiler {
namespace Internal {

LocalQmlProfilerRunnerTest::LocalQmlProfilerRunnerTest(QObject *parent) : QObject(parent)
{
}

void LocalQmlProfilerRunnerTest::testRunner()
{
    QPointer<RunControl> runControl;
    QPointer<LocalQmlProfilerSupport> profiler;
    QUrl serverUrl;

    bool running = false;
    bool started = false;
    int startCount = 0;
    int runCount = 0;
    int stopCount = 0;

    // should not be used anywhere but cannot be empty
    serverUrl.setScheme(Utils::urlSocketScheme());
    serverUrl.setPath("invalid");

    runControl = new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    runControl->setCommandLine({"\\-/|\\-/", {}});

    profiler = new LocalQmlProfilerSupport(runControl, serverUrl);

    auto connectRunner = [&]() {
        connect(runControl, &RunControl::aboutToStart, this, [&] {
            QVERIFY(!started);
            QVERIFY(!running);
            ++startCount;
            started = true;
        });
        connect(runControl, &RunControl::started, this, [&] {
            QVERIFY(started);
            QVERIFY(!running);
            ++runCount;
            running = true;
        });
        connect(runControl, &RunControl::stopped, this, [&] {
            QVERIFY(started);
            ++stopCount;
            running = false;
            started = false;
        });
    };

    connectRunner();

    QTest::ignoreMessage(
                QtDebugMsg, "Invalid run control state transition from  "
                            "\"RunControlState::Starting\"  to  \"RunControlState::Stopped\"");
    runControl->initiateStart();

    QTRY_COMPARE_WITH_TIMEOUT(startCount, 1, 30000);
    QTRY_VERIFY_WITH_TIMEOUT(!started, 30000);
    QCOMPARE(stopCount, 1);
    QCOMPARE(runCount, 0);

    runControl->setAutoDeleteOnStop(true);
    runControl->initiateStop();
    QTRY_VERIFY(runControl.isNull());
    QVERIFY(profiler.isNull());

    serverUrl = Utils::urlFromLocalSocket();
    // comma is used to specify a test function. In this case, an invalid one.
    runControl = new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);

    const FilePath app = FilePath::fromString(QCoreApplication::applicationFilePath());
    runControl->setCommandLine({app, {"-test", "QmlProfiler,"}});
    profiler = new LocalQmlProfilerSupport(runControl, serverUrl);
    connectRunner();
    runControl->initiateStart();

    QTRY_VERIFY_WITH_TIMEOUT(running, 30000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 30000);
    QCOMPARE(startCount, 2);
    QCOMPARE(stopCount, 2);
    QCOMPARE(runCount, 1);

    runControl->setAutoDeleteOnStop(true);
    runControl->initiateStop();
    QTRY_VERIFY(runControl.isNull());
    QVERIFY(profiler.isNull());

    serverUrl.clear();
    serverUrl = Utils::urlFromLocalHostAndFreePort();
    runControl = new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    runControl->setCommandLine({app, {}});
    profiler = new LocalQmlProfilerSupport(runControl, serverUrl);
    connectRunner();
    runControl->initiateStart();

    QTRY_VERIFY_WITH_TIMEOUT(running, 30000);
    runControl->initiateStop();
    QTRY_VERIFY_WITH_TIMEOUT(!running, 30000);
    QCOMPARE(startCount, 3);
    QCOMPARE(stopCount, 3);
    QCOMPARE(runCount, 2);

    runControl->setAutoDeleteOnStop(true);
    runControl->initiateStop();
    QTRY_VERIFY(runControl.isNull());
    QVERIFY(profiler.isNull());

    serverUrl.setScheme(Utils::urlSocketScheme());
    {
        Utils::TemporaryFile file("file with spaces");
        if (file.open())
            serverUrl.setPath(file.fileName());
    }

    runControl = new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    runControl->setCommandLine({app, {"-test", "QmlProfiler,"}});
    profiler = new LocalQmlProfilerSupport(runControl, serverUrl);
    connectRunner();
    runControl->initiateStart();

    QTRY_VERIFY_WITH_TIMEOUT(running, 30000);
    QTRY_VERIFY_WITH_TIMEOUT(!running, 30000);
    QCOMPARE(startCount, 4);
    QCOMPARE(stopCount, 4);
    QCOMPARE(runCount, 3);

    runControl->setAutoDeleteOnStop(true);
    runControl->initiateStop();
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
    QVERIFY(!QFileInfo::exists(socket));
    QFile file(socket);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
}

} // namespace Internal
} // namespace QmlProfiler
