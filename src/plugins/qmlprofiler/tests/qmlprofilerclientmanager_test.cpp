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

#include "qmlprofilerclientmanager_test.h"
#include <qmlprofiler/localqmlprofilerrunner.h>
#include <qmldebug/qpacketprotocol.h>
#include <projectexplorer/applicationlauncher.h>

#include <QTcpServer>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QQmlDebuggingEnabler>

#include <QtTest>

namespace QmlProfiler {
namespace Internal {

struct MessageHandler {
    MessageHandler(QtMessageHandler handler)
    {
        defaultHandler = qInstallMessageHandler(handler);
    }

    ~MessageHandler()
    {
        qInstallMessageHandler(defaultHandler);
    }

    static QtMessageHandler defaultHandler;
};

QtMessageHandler MessageHandler::defaultHandler;

QmlProfilerClientManagerTest::QmlProfilerClientManagerTest(QObject *parent) :
    QObject(parent), modelManager(nullptr)
{
    clientManager.setRetryParams(10, 10);
}

void QmlProfilerClientManagerTest::testConnectionFailure_data()
{
    QTest::addColumn<QmlProfilerModelManager *>("modelManager");
    QVarLengthArray<QmlProfilerModelManager *> modelManagers({nullptr, &modelManager});

    QTest::addColumn<QmlProfilerStateManager *>("stateManager");
    QVarLengthArray<QmlProfilerStateManager *> stateManagers({nullptr, &stateManager});

    QString hostName;
    Utils::Port port = LocalQmlProfilerRunner::findFreePort(hostName);

    QTest::addColumn<QString>("host");
    QVarLengthArray<QString> hosts({"", "/-/|\\-\\|/-", hostName});

    QTest::addColumn<Utils::Port>("port");
    QVarLengthArray<Utils::Port> ports({Utils::Port(), Utils::Port(5), port});

    QTest::addColumn<QString>("socket");
    QVarLengthArray<QString> sockets({"", "/-/|\\-\\|/-",
                                      LocalQmlProfilerRunner::findFreeSocket()});

    foreach (QmlProfilerModelManager *modelManager, modelManagers) {
        foreach (QmlProfilerStateManager *stateManager, stateManagers) {
            foreach (QString host, hosts) {
                foreach (Utils::Port port, ports) {
                    foreach (QString socket, sockets) {
                        QString tag = QString::fromLatin1("%1, %2, %3, %4, %5")
                                .arg(QLatin1String(modelManager ? "modelManager" : "<null>"))
                                .arg(QLatin1String(stateManager ? "stateManager" : "<null>"))
                                .arg(host.isEmpty() ? "<empty>" : host)
                                .arg(port.isValid() ? port.number() : 0)
                                .arg(socket.isEmpty() ? "<empty>" : socket);
                        QTest::newRow(tag.toLatin1().constData())
                                << modelManager << stateManager << host << port << socket;
                    }
                }
            }
        }
    }
}

void softAssertMessageHandler(QtMsgType type, const QMessageLogContext &context,
                              const QString &message)
{
    if (type != QtDebugMsg || !message.startsWith("SOFT ASSERT: "))
        MessageHandler::defaultHandler(type, context, message);
}

void QmlProfilerClientManagerTest::testConnectionFailure()
{
    // This triggers a lot of soft asserts. We test that it still doesn't crash and stays in a
    // consistent state.
    QByteArray fatalAsserts =  qgetenv("QTC_FATAL_ASSERTS");
    qunsetenv("QTC_FATAL_ASSERTS");
    MessageHandler handler(&softAssertMessageHandler);
    Q_UNUSED(handler);

    QFETCH(QmlProfilerModelManager *, modelManager);
    QFETCH(QmlProfilerStateManager *, stateManager);
    QFETCH(QString, host);
    QFETCH(Utils::Port, port);
    QFETCH(QString, socket);

    QSignalSpy openedSpy(&clientManager, SIGNAL(connectionOpened()));
    QSignalSpy closedSpy(&clientManager, SIGNAL(connectionClosed()));
    QSignalSpy failedSpy(&clientManager, SIGNAL(connectionFailed()));

    QVERIFY(!clientManager.isConnected());

    clientManager.setModelManager(modelManager);
    clientManager.setProfilerStateManager(stateManager);
    if (socket.isEmpty()) {
        clientManager.setTcpConnection(host, port);
    } else {
        clientManager.setLocalSocket(socket);
    }

    QVERIFY(!clientManager.isConnected());

    clientManager.connectToTcpServer();
    QTRY_COMPARE(failedSpy.count(), 1);
    QCOMPARE(closedSpy.count(), 0);
    QCOMPARE(openedSpy.count(), 0);
    QVERIFY(!clientManager.isConnected());

    clientManager.startLocalServer();
    QTRY_COMPARE(failedSpy.count(), 2);
    QCOMPARE(closedSpy.count(), 0);
    QCOMPARE(openedSpy.count(), 0);
    QVERIFY(!clientManager.isConnected());

    clientManager.retryConnect();
    QTRY_COMPARE(failedSpy.count(), 3);
    QCOMPARE(closedSpy.count(), 0);
    QCOMPARE(openedSpy.count(), 0);
    QVERIFY(!clientManager.isConnected());

    clientManager.clearConnection();

    qputenv("QTC_FATAL_ASSERTS", fatalAsserts);
}

void QmlProfilerClientManagerTest::testUnresponsiveTcp()
{
    QSignalSpy openedSpy(&clientManager, SIGNAL(connectionOpened()));
    QSignalSpy closedSpy(&clientManager, SIGNAL(connectionClosed()));
    QSignalSpy failedSpy(&clientManager, SIGNAL(connectionFailed()));

    QVERIFY(!clientManager.isConnected());

    clientManager.setProfilerStateManager(&stateManager);
    clientManager.setModelManager(&modelManager);

    QString hostName;
    Utils::Port port = LocalQmlProfilerRunner::findFreePort(hostName);

    QTcpServer server;
    server.listen(QHostAddress(hostName), port.number());
    QSignalSpy connectionSpy(&server, SIGNAL(newConnection()));

    clientManager.setTcpConnection(hostName, port);
    clientManager.connectToTcpServer();

    QTRY_VERIFY(connectionSpy.count() > 0);
    QTRY_COMPARE(failedSpy.count(), 1);
    QCOMPARE(openedSpy.count(), 0);
    QCOMPARE(closedSpy.count(), 0);
    QVERIFY(!clientManager.isConnected());

    clientManager.clearConnection();
}

void QmlProfilerClientManagerTest::testUnresponsiveLocal()
{
    QSignalSpy openedSpy(&clientManager, SIGNAL(connectionOpened()));
    QSignalSpy closedSpy(&clientManager, SIGNAL(connectionClosed()));
    QSignalSpy failedSpy(&clientManager, SIGNAL(connectionFailed()));

    QVERIFY(!clientManager.isConnected());

    clientManager.setProfilerStateManager(&stateManager);
    clientManager.setModelManager(&modelManager);

    QString socketFile = LocalQmlProfilerRunner::findFreeSocket();
    QLocalSocket socket;
    QSignalSpy connectionSpy(&socket, SIGNAL(connected()));

    clientManager.setLocalSocket(socketFile);
    clientManager.startLocalServer();

    socket.connectToServer(socketFile);
    QTRY_COMPARE(connectionSpy.count(), 1);
    QTRY_COMPARE(failedSpy.count(), 1);
    QCOMPARE(openedSpy.count(), 0);
    QCOMPARE(closedSpy.count(), 0);
    QVERIFY(!clientManager.isConnected());

    clientManager.clearConnection();
}

void responsiveTestData()
{
    QTest::addColumn<quint32>("flushInterval");

    QTest::newRow("no flush") << 0u;
    QTest::newRow("flush")    << 1u;
}

void QmlProfilerClientManagerTest::testResponsiveTcp_data()
{
    responsiveTestData();
}

void fakeDebugServer(QIODevice *socket)
{
    QmlDebug::QPacketProtocol *protocol = new QmlDebug::QPacketProtocol(socket, socket);
    QObject::connect(protocol, &QmlDebug::QPacketProtocol::readyRead, [protocol]() {
        QmlDebug::QPacket packet(QDataStream::Qt_4_7);
        const int messageId = 0;
        const int protocolVersion = 1;
        const QStringList pluginNames({"CanvasFrameRate", "EngineControl", "DebugMessages"});
        const QList<float> pluginVersions({1.0f, 1.0f, 1.0f});

        packet << QString::fromLatin1("QDeclarativeDebugClient") << messageId << protocolVersion
               << pluginNames << pluginVersions << QDataStream::Qt_DefaultCompiledVersion;
        protocol->send(packet.data());
        protocol->disconnect();
        protocol->deleteLater();
    });
}

void QmlProfilerClientManagerTest::testResponsiveTcp()
{
    QFETCH(quint32, flushInterval);

    QString hostName;
    Utils::Port port = LocalQmlProfilerRunner::findFreePort(hostName);

    QSignalSpy openedSpy(&clientManager, SIGNAL(connectionOpened()));
    QSignalSpy closedSpy(&clientManager, SIGNAL(connectionClosed()));

    QVERIFY(!clientManager.isConnected());

    {
        QTcpServer server;
        QScopedPointer<QTcpSocket> socket;
        connect(&server, &QTcpServer::newConnection, [&server, &socket]() {
            socket.reset(server.nextPendingConnection());
            fakeDebugServer(socket.data());
        });

        server.listen(QHostAddress(hostName), port.number());

        clientManager.setProfilerStateManager(&stateManager);
        clientManager.setModelManager(&modelManager);
        clientManager.setFlushInterval(flushInterval);

        connect(&clientManager, &QmlProfilerClientManager::connectionFailed,
                &clientManager, &QmlProfilerClientManager::retryConnect);

        clientManager.setTcpConnection(hostName, port);
        clientManager.connectToTcpServer();

        QTRY_COMPARE(openedSpy.count(), 1);
        QCOMPARE(closedSpy.count(), 0);
        QVERIFY(clientManager.isConnected());

        // Do some nasty things and make sure it doesn't crash
        stateManager.setCurrentState(QmlProfilerStateManager::AppRunning);
        stateManager.setClientRecording(false);
        stateManager.setClientRecording(true);
        clientManager.clearBufferedData();
        stateManager.setCurrentState(QmlProfilerStateManager::AppStopRequested);

        QVERIFY(socket);
    }

    QTRY_COMPARE(closedSpy.count(), 1);
    QVERIFY(!clientManager.isConnected());

    disconnect(&clientManager, &QmlProfilerClientManager::connectionFailed,
               &clientManager, &QmlProfilerClientManager::retryConnect);

    stateManager.setCurrentState(QmlProfilerStateManager::Idle);
}

void QmlProfilerClientManagerTest::testResponsiveLocal_data()
{
    responsiveTestData();
}

void QmlProfilerClientManagerTest::testResponsiveLocal()
{
    QFETCH(quint32, flushInterval);

    QString socketFile = LocalQmlProfilerRunner::findFreeSocket();

    QSignalSpy openedSpy(&clientManager, SIGNAL(connectionOpened()));
    QSignalSpy closedSpy(&clientManager, SIGNAL(connectionClosed()));

    QVERIFY(!clientManager.isConnected());

    clientManager.setProfilerStateManager(&stateManager);
    clientManager.setModelManager(&modelManager);
    clientManager.setFlushInterval(flushInterval);

    connect(&clientManager, &QmlProfilerClientManager::connectionFailed,
            &clientManager, &QmlProfilerClientManager::retryConnect);

    clientManager.setLocalSocket(socketFile);
    clientManager.startLocalServer();

    {
        QScopedPointer<QLocalSocket> socket(new QLocalSocket(this));
        socket->connectToServer(socketFile);
        QVERIFY(socket->isOpen());
        fakeDebugServer(socket.data());

        QTRY_COMPARE(openedSpy.count(), 1);
        QCOMPARE(closedSpy.count(), 0);
        QVERIFY(clientManager.isConnected());

        // Do some nasty things and make sure it doesn't crash
        stateManager.setCurrentState(QmlProfilerStateManager::AppRunning);
        stateManager.setClientRecording(false);
        stateManager.setClientRecording(true);
        clientManager.clearBufferedData();
        stateManager.setCurrentState(QmlProfilerStateManager::AppStopRequested);
    }

    QTRY_COMPARE(closedSpy.count(), 1);
    QVERIFY(!clientManager.isConnected());

    disconnect(&clientManager, &QmlProfilerClientManager::connectionFailed,
               &clientManager, &QmlProfilerClientManager::retryConnect);

    stateManager.setCurrentState(QmlProfilerStateManager::Idle);
}

void invalidHelloMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                const QString &message)
{
    if (type != QtWarningMsg || message != "QML Debug Client: Invalid hello message")
        MessageHandler::defaultHandler(type, context, message);
}

void QmlProfilerClientManagerTest::testInvalidData()
{
    MessageHandler handler(&invalidHelloMessageHandler);
    Q_UNUSED(handler);

    QSignalSpy openedSpy(&clientManager, SIGNAL(connectionOpened()));
    QSignalSpy closedSpy(&clientManager, SIGNAL(connectionClosed()));
    QSignalSpy failedSpy(&clientManager, SIGNAL(connectionFailed()));

    QVERIFY(!clientManager.isConnected());

    clientManager.setProfilerStateManager(&stateManager);
    clientManager.setModelManager(&modelManager);

    QString hostName;
    Utils::Port port = LocalQmlProfilerRunner::findFreePort(hostName);

    bool dataSent = false;
    QTcpServer server;
    connect(&server, &QTcpServer::newConnection, [&server, &dataSent](){
        QTcpSocket *socket = server.nextPendingConnection();

        // emulate packet protocol
        qint32 sendSize32 = 10;
        socket->write((char *)&sendSize32, sizeof(qint32));
        socket->write("----------------------- x -----------------------");

        dataSent = true;
    });

    server.listen(QHostAddress(hostName), port.number());

    clientManager.setTcpConnection(hostName, port);
    clientManager.connectToTcpServer();

    QTRY_VERIFY(dataSent);
    QTRY_COMPARE(failedSpy.count(), 1);
    QCOMPARE(openedSpy.count(), 0);
    QCOMPARE(closedSpy.count(), 0);
    QVERIFY(!clientManager.isConnected());

    clientManager.clearConnection();
}

void QmlProfilerClientManagerTest::testStopRecording()
{
    QString socketFile = LocalQmlProfilerRunner::findFreeSocket();

    {
        QmlProfilerClientManager clientManager;
        clientManager.setRetryParams(10, 10);
        QSignalSpy openedSpy(&clientManager, SIGNAL(connectionOpened()));
        QSignalSpy closedSpy(&clientManager, SIGNAL(connectionClosed()));

        QVERIFY(!clientManager.isConnected());

        clientManager.setProfilerStateManager(&stateManager);
        clientManager.setModelManager(&modelManager);

        connect(&clientManager, &QmlProfilerClientManager::connectionFailed,
                &clientManager, &QmlProfilerClientManager::retryConnect);

        clientManager.setLocalSocket(socketFile);
        clientManager.startLocalServer();

        QScopedPointer<QLocalSocket> socket(new QLocalSocket(this));
        socket->connectToServer(socketFile);
        QVERIFY(socket->isOpen());
        fakeDebugServer(socket.data());

        QTRY_COMPARE(openedSpy.count(), 1);
        QCOMPARE(closedSpy.count(), 0);
        QVERIFY(clientManager.isConnected());

        // We can't verify that it does anything useful, but at least it doesn't crash
        clientManager.stopRecording();
    }

    // Delete while still connected, for added fun
}

} // namespace Internal
} // namespace QmlProfiler
