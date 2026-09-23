// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmldebug/qmldebugconnection.h>

#include <QTest>

using namespace QmlDebug;

class tst_QmlDebugConnection : public QObject
{
    Q_OBJECT

private slots:
    void closeResetsPendingConnection();
};

// A connect attempt that never reaches ConnectedState emits neither
// disconnected() nor errorOccurred() when closed, so close() itself must reset
// the pending connection. Otherwise isConnecting() stays true forever and any
// later connect attempt is skipped, hanging an application launched with
// "-qmljsdebugger=...,block". Regression test for QTCREATORBUG-34848.
void tst_QmlDebugConnection::closeResetsPendingConnection()
{
    QmlDebugConnection connection;
    QVERIFY(!connection.isConnecting());
    QVERIFY(!connection.isConnected());

    // Start connecting without spinning the event loop, so the socket stays in
    // a pre-connected state (no connection result is delivered).
    connection.connectToHost("127.0.0.1", 1);
    QVERIFY(connection.isConnecting());
    QVERIFY(!connection.isConnected());

    connection.close();
    QVERIFY(!connection.isConnecting());

    // A subsequent attempt must be possible again.
    connection.connectToHost("127.0.0.1", 1);
    QVERIFY(connection.isConnecting());
    connection.close();
    QVERIFY(!connection.isConnecting());
}

QTEST_GUILESS_MAIN(tst_QmlDebugConnection)

#include "tst_qmldebugconnection.moc"
