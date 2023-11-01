// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertool_test.h"
#include "fakedebugserver.h"

#include <coreplugin/icore.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/runcontrol.h>

#include <qmlprofiler/qmlprofilerattachdialog.h>
#include <qmlprofiler/qmlprofilerclientmanager.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <qmlprofiler/qmlprofilerstatemanager.h>

#include <utils/qtcsettings.h>
#include <utils/url.h>

#include <QTcpServer>
#include <QtTest>

using namespace Utils;

namespace QmlProfiler {
namespace Internal {

void QmlProfilerToolTest::testAttachToWaitingApplication()
{
    ProjectExplorer::KitManager *kitManager = ProjectExplorer::KitManager::instance();
    QVERIFY(kitManager);
    ProjectExplorer::Kit * const newKit = ProjectExplorer::KitManager::registerKit({}, "fookit");
    QVERIFY(newKit);
    QtcSettings *settings = Core::ICore::settings();
    QVERIFY(settings);
    settings->setValue("AnalyzerQmlAttachDialog/kitId", newKit->id().toSetting());

    QmlProfilerTool &profilerTool = *QmlProfilerTool::instance();

    QmlProfilerClientManager *clientManager = profilerTool.clientManager();
    clientManager->setRetryInterval(10);
    clientManager->setMaximumRetries(10);
    connect(clientManager, &QmlProfilerClientManager::connectionFailed,
            clientManager, &QmlProfilerClientManager::retryConnect);

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
    ProjectExplorer::RunControl *runControl = profilerTool.attachToWaitingApplication();
    QVERIFY(runControl);

    QTRY_VERIFY(connection);
    QTRY_VERIFY(runControl->isRunning());
    QTRY_VERIFY(modalSeen);
    QTRY_VERIFY(!timer.isActive());
    QTRY_VERIFY(clientManager->isConnected());

    connection.reset();
    QTRY_VERIFY(runControl->isStopped());
}

void QmlProfilerToolTest::testClearEvents()
{
    QmlProfilerTool &profilerTool = *QmlProfilerTool::instance();
    QmlProfilerModelManager *modelManager = profilerTool.modelManager();
    QVERIFY(modelManager);
    QmlProfilerStateManager *stateManager = profilerTool.stateManager();
    QVERIFY(stateManager);

    stateManager->setCurrentState(QmlProfilerStateManager::AppRunning);
    stateManager->setServerRecording(true);
    QCOMPARE(modelManager->numEventTypes(), 0);
    QCOMPARE(modelManager->numEvents(), 0);
    const int typeIndex = modelManager->appendEventType(QmlEventType());
    QCOMPARE(typeIndex, 0);
    modelManager->appendEvent(QmlEvent(0, typeIndex, ""));
    QCOMPARE(modelManager->numEventTypes(), 1);
    QCOMPARE(modelManager->numEvents(), 1);
    stateManager->setServerRecording(false);
    QCOMPARE(modelManager->numEventTypes(), 1);
    QCOMPARE(modelManager->numEvents(), 1);
    stateManager->setServerRecording(true); // clears previous events, but not types
    QCOMPARE(modelManager->numEventTypes(), 1);
    QCOMPARE(modelManager->numEvents(), 0);
    modelManager->appendEvent(QmlEvent(0, typeIndex, ""));
    QCOMPARE(modelManager->numEventTypes(), 1);
    QCOMPARE(modelManager->numEvents(), 1);
}

} // namespace Internal
} // namespace QmlProfiler
