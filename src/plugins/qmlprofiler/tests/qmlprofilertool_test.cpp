/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qmlprofilertool_test.h"
#include "fakedebugserver.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <qmlprofiler/qmlprofilerattachdialog.h>
#include <qmlprofiler/qmlprofilerclientmanager.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <qmlprofiler/qmlprofilerstatemanager.h>
#include <utils/url.h>

#include <QTcpServer>
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

void QmlProfilerToolTest::testAttachToWaitingApplication()
{
    auto newKit = std::make_unique<ProjectExplorer::Kit>("fookit");
    ProjectExplorer::Kit * newKitPtr = newKit.get();
    ProjectExplorer::KitManager *kitManager = ProjectExplorer::KitManager::instance();
    QVERIFY(kitManager);
    QVERIFY(kitManager->registerKit(std::move(newKit)));
    QSettings *settings = Core::ICore::settings();
    QVERIFY(settings);
    settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/kitId"), newKitPtr->id().toSetting());

    QmlProfilerTool profilerTool;

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
    QmlProfilerTool profilerTool;
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
