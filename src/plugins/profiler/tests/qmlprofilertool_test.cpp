// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertool_test.h"
#include "fakedebugserver.h"

#include <coreplugin/icore.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>

#include <profiler/qmlprofilerattachdialog.h>
#include <profiler/qmlprofilerclientmanager.h>
#include <profiler/qmlprofilermodelmanager.h>
#include <profiler/qmlprofilerruncontrol.h>
#include <profiler/qmlprofilerstatemanager.h>
#include <profiler/qmlprofilertool.h>
#include <profiler/qmlprofilertracebackend.h>

#include <tracing/timelinedetailswidget.h>

#include <utils/qtcsettings.h>
#include <utils/url.h>

#include <QPointer>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;
using namespace QmlDebug;
using namespace std::chrono_literals;

namespace Profiler::Internal {

void QmlProfilerToolTest::testAttachToWaitingApplication()
{
    KitManager *kitManager = KitManager::instance();
    QVERIFY(kitManager);
    Kit * const newKit = KitManager::registerKit({}, "fookit");
    QVERIFY(newKit);
    QtcSettings *settings = Core::ICore::settings();
    QVERIFY(settings);
    settings->setValue("AnalyzerQmlAttachDialog/kitId", newKit->id().toSetting());

    QmlProfilerTool &profilerTool = *QmlProfilerTool::instance();

    // The run opens the trace it records into, and with it the client manager,
    // just before connecting; this is the point at which it can be configured.
    QmlProfilerClientManager *clientManager = nullptr;
    QMetaObject::Connection failedConnection;
    const QMetaObject::Connection backendConnection = connect(
        &profilerTool, &QmlProfilerTool::liveBackendChanged, this,
        [this, &clientManager, &failedConnection](QmlProfilerTraceBackend *backend) {
            clientManager = backend->clientManager();
            clientManager->setRetryInterval(10);
            clientManager->setMaximumRetries(10);
            failedConnection = connect(clientManager,
                                       &QmlProfilerClientManager::connectionFailed, this, [] {
                QFAIL("Connection failed");
            });
        });
    // The tool is a singleton, so these outlive the locals they write to unless
    // they are taken down here; a failing QVERIFY returns without reaching the
    // end of the function.
    const auto disconnectFromTool = qScopeGuard([&] {
        disconnect(backendConnection);
        disconnect(failedConnection);
    });

    QTcpServer server;
    QUrl serverUrl = Utils::urlFromLocalHostAndFreePort();
    QVERIFY(serverUrl.port() >= 0);
    QVERIFY(serverUrl.port() <= std::numeric_limits<quint16>::max());
    server.listen(QHostAddress::Any, static_cast<quint16>(serverUrl.port()));

    QScopedPointer<QTcpSocket> connection;
    connect(&server, &QTcpServer::newConnection, this, [&]() {
        connection.reset(server.nextPendingConnection());
        fakeDebugServer(connection.data());
    });

    QTimer timer;
    timer.setInterval(100);

    bool modalSeen = false;
    connect(&timer, &QTimer::timeout, this, [&]() {
        if (QWidget *activeModal = QApplication::activeModalWidget()) {
            modalSeen = true;
            auto dialog = qobject_cast<QmlProfilerAttachDialog *>(activeModal);
            if (dialog) {
                dialog->setPort(serverUrl.port());
                dialog->accept();
                timer.stop();
            } else {
                qWarning() << "Some other modal widget popped up:" << activeModal;
                activeModal->close();
            }
        }
    });

    timer.start();
    RunControl *runControl = profilerTool.attachToWaitingApplication();
    QVERIFY(runControl);

    QTRY_VERIFY(connection);
    QTRY_VERIFY(runControl->isRunning());
    QTRY_VERIFY(modalSeen);
    QTRY_VERIFY(!timer.isActive());
    QVERIFY(clientManager);
    QTRY_VERIFY(clientManager->isConnected());

    connection.reset();
    QTRY_VERIFY(runControl->isStopped());
}

void QmlProfilerToolTest::testClearEvents()
{
    // A trace's own models and recording state, as a document owns them.
    Timeline::TimelineDetailsWidget details;
    QmlProfilerTraceBackend backend(&details);
    QmlProfilerModelManager *modelManager = backend.modelManager();
    QVERIFY(modelManager);
    QmlProfilerStateManager *stateManager = backend.stateManager();
    QVERIFY(stateManager);

    stateManager->setCurrentState(QmlProfilerStateManager::AppRunning);
    stateManager->setServerRecording(true);
    QCOMPARE(modelManager->numEventTypes(), 0);
    QCOMPARE(modelManager->numEvents(), 0);
    const int typeIndex = modelManager->appendEventType(QmlEventType());
    QCOMPARE(typeIndex, 0);
    modelManager->appendEvent(QmlEvent(0ns, typeIndex, ""));
    QCOMPARE(modelManager->numEventTypes(), 1);
    QCOMPARE(modelManager->numEvents(), 1);
    stateManager->setServerRecording(false);
    QCOMPARE(modelManager->numEventTypes(), 1);
    QCOMPARE(modelManager->numEvents(), 1);
    stateManager->setServerRecording(true); // clears previous events, but not types
    QCOMPARE(modelManager->numEventTypes(), 1);
    QCOMPARE(modelManager->numEvents(), 0);
    modelManager->appendEvent(QmlEvent(0ns, typeIndex, ""));
    QCOMPARE(modelManager->numEventTypes(), 1);
    QCOMPARE(modelManager->numEvents(), 1);
}

void QmlProfilerToolTest::testCancelBeforeConnectReportsOnce()
{
    // The stop path must report a never-connected run only once even when it
    // is entered repeatedly; calling handleStop() twice here exercises that
    // guard directly.
    Timeline::TimelineDetailsWidget details;
    QmlProfilerTraceBackend backend(&details);
    QSignalSpy errors(&backend, &ProfilerTraceBackend::error);

    backend.prepareRun(nullptr, false, 100ms);
    backend.handleStop();
    backend.handleStop();

    QCOMPARE(errors.count(), 1);
}

// Options asked for a run that never starts must not end up on the next QML
// profiler run, which may have been started some other way, such as the Re-run
// button in Application Output.
void QmlProfilerToolTest::testOptionsStayWithTheirRun()
{
    if (ProjectManager::startupProject())
        QSKIP("A startup project is open, so the first run would start.");

    Kit *const kit = KitManager::registerKit({}, "optionskit");
    QVERIFY(kit);
    const auto deregisterKit = qScopeGuard([kit] { KitManager::deregisterKit(kit); });

    QmlProfilerTool &profilerTool = *QmlProfilerTool::instance();
    QmlRecordingOptions options;
    options.features = 1ULL << ProfileBinding;
    options.flushInterval = 250ms;
    profilerTool.profileStartupProject(options);

    QPointer<QmlProfilerTraceBackend> backend;
    const QMetaObject::Connection backendConnection = connect(
        &profilerTool, &QmlProfilerTool::liveBackendChanged, this,
        [&backend](QmlProfilerTraceBackend *live) {
            backend = live;
            live->clientManager()->setRetryInterval(10);
            live->clientManager()->setMaximumRetries(10);
        });
    const auto disconnectFromTool = qScopeGuard([&] { disconnect(backendConnection); });

    const QUrl serverUrl = Utils::urlFromLocalHostAndFreePort();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, quint16(serverUrl.port())));
    QScopedPointer<QTcpSocket> connection;
    connect(&server, &QTcpServer::newConnection, this, [&] {
        connection.reset(server.nextPendingConnection());
        fakeDebugServer(connection.data());
    });

    auto runControl = new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    runControl->setKit(kit);
    runControl->setQmlChannel(serverUrl);
    runControl->setRunRecipe(qmlProfilerRecipe(runControl));
    runControl->start();

    QTRY_VERIFY(backend);
    QTRY_VERIFY(backend->clientManager()->isConnected());
    QVERIFY(backend->stateManager()->requestedFeatures() != options.features);
    QCOMPARE(backend->clientManager()->flushInterval(), 0ms);

    connection.reset();
    QTRY_VERIFY(runControl->isStopped());
}

} // namespace Profiler::Internal
