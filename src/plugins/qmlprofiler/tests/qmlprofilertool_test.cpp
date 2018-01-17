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

#include <projectexplorer/runconfiguration.h>
#include <qmlprofiler/qmlprofilerclientmanager.h>
#include <qmlprofiler/qmlprofilerattachdialog.h>
#include <utils/url.h>

#include <QTcpServer>
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

void QmlProfilerToolTest::testAttachToWaitingApplication()
{
    QmlProfilerTool profilerTool(nullptr);
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

} // namespace Internal
} // namespace QmlProfiler
