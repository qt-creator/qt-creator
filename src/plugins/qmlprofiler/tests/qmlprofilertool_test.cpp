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
    ProjectExplorer::Kit *newKit = new ProjectExplorer::Kit("fookit");
    ProjectExplorer::KitManager *kitManager = ProjectExplorer::KitManager::instance();
    QVERIFY(kitManager);
    QVERIFY(kitManager->registerKit(newKit));
    QSettings *settings = Core::ICore::settings();
    QVERIFY(settings);
    settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/kitId"), newKit->id().toSetting());

    QmlProfilerTool profilerTool;
    QTcpServer server;
    QUrl serverUrl = Utils::urlFromLocalHostAndFreePort();
    QVERIFY(serverUrl.port() >= 0);
    QVERIFY(serverUrl.port() <= std::numeric_limits<quint16>::max());
    server.listen(QHostAddress(serverUrl.host()), static_cast<quint16>(serverUrl.port()));

    QScopedPointer<QTcpSocket> connection;
    connect(&server, &QTcpServer::newConnection, this, [&]() {
        connection.reset(server.nextPendingConnection());
        fakeDebugServer(connection.data());
        server.close();
    });

    QTimer timer;
    timer.setInterval(100);
    connect(&timer, &QTimer::timeout, this, [&]() {
        if (auto activeModal
                = qobject_cast<QmlProfilerAttachDialog *>(QApplication::activeModalWidget())) {
            activeModal->setPort(serverUrl.port());
            activeModal->accept();
        }
    });

    timer.start();
    ProjectExplorer::RunControl *runControl = profilerTool.attachToWaitingApplication();
    QVERIFY(runControl);

    QTRY_VERIFY(connection);
    QTRY_VERIFY(runControl->isRunning());
    QTRY_VERIFY(profilerTool.clientManager()->isConnected());

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
