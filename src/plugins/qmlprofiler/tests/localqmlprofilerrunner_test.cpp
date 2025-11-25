// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "localqmlprofilerrunner_test.h"

#include <debugger/analyzer/analyzerutils.h>

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <qmlprofiler/qmlprofilerruncontrol.h>
#include <qmlprofiler/qmlprofilerstatemanager.h>
#include <qmlprofiler/qmlprofilertool.h>

#include <utils/url.h>
#include <utils/temporaryfile.h>

#include <QTcpServer>
#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProfiler {
namespace Internal {

LocalQmlProfilerRunnerTest::LocalQmlProfilerRunnerTest(QObject *parent) : QObject(parent)
{
}

void LocalQmlProfilerRunnerTest::testRunner()
{
    QmlProfilerStateManager *stateManager = QmlProfilerTool::instance()->stateManager();
    QVERIFY(stateManager);

    // Request some (invalid) feature so that old Qt versions don't run into interesting
    // situations when starting and stopping profiler adapters.
    if (!stateManager->requestedFeatures())
        stateManager->setRequestedFeatures(1ull << 63);

    std::unique_ptr<RunControl> runControl;

    bool running = false;
    bool started = false;
    int startCount = 0;
    int runCount = 0;
    int stopCount = 0;

    runControl.reset(new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE));
    runControl->setKit(KitManager::defaultKit());
    runControl->setCommandLine(CommandLine{"\\-/|\\-/"});
    runControl->setRunRecipe(localQmlProfilerRecipe(runControl.get()));

    auto connectRunner = [&] {
        running = false;
        started = false;
        connect(runControl.get(), &RunControl::aboutToStart, this, [&] {
            QVERIFY(!started);
            QVERIFY(!running);
            ++startCount;
            started = true;
        });
        connect(runControl.get(), &RunControl::started, this, [&] {
            QVERIFY(started);
            QVERIFY(!running);
            ++runCount;
            running = true;
        });
        connect(runControl.get(), &RunControl::stopped, this, [&] {
            QVERIFY(started);
            ++stopCount;
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
    QVERIFY(runControl->isStopped());

    // comma is used to specify a test function. In this case, an invalid one.
    runControl.reset(new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE));
    runControl->setKit(KitManager::defaultKit());
    const FilePath app = FilePath::fromString(QCoreApplication::applicationFilePath());
    runControl->setCommandLine({app, {"-test", "QmlProfiler,"}});
    runControl->setRunRecipe(localQmlProfilerRecipe(runControl.get()));
    connectRunner();
    runControl->initiateStart();

    // initiateStart() may immediately stop, without giving us a chance to see running == true.
    QTRY_COMPARE_WITH_TIMEOUT(stopCount, 2, 30000);
    QVERIFY(!running);
    QCOMPARE(startCount, 2);
    QCOMPARE(runCount, 1);
    QVERIFY(runControl->isStopped());

    runControl.reset(new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE));
    runControl->setKit(KitManager::defaultKit());
    runControl->setCommandLine(CommandLine{app});
    runControl->setRunRecipe(localQmlProfilerRecipe(runControl.get()));
    connectRunner();
    runControl->initiateStart();

    QTRY_VERIFY_WITH_TIMEOUT(running, 30000);
    runControl->initiateStop();
    QTRY_COMPARE_WITH_TIMEOUT(stopCount, 3, 30000);
    QVERIFY(!running);
    QCOMPARE(startCount, 3);
    QCOMPARE(stopCount, 3);
    QCOMPARE(runCount, 2);
    QVERIFY(runControl->isStopped());

    runControl.reset(new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE));
    runControl->setKit(KitManager::defaultKit());
    runControl->setCommandLine({app, {"-test", "QmlProfiler,"}});
    runControl->setRunRecipe(localQmlProfilerRecipe(runControl.get()));
    connectRunner();
    runControl->initiateStart();

    QTRY_COMPARE_WITH_TIMEOUT(stopCount, 4, 30000);
    QVERIFY(!running);
    QCOMPARE(startCount, 4);
    QCOMPARE(runCount, 3);
    QVERIFY(runControl->isStopped());
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
