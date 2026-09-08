// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <mcp/server/mcpserver.h>

#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include <memory>

using namespace Mcp;

static const char token[] = "0123456789abcdef0123456789abcdef";

class TestServer
{
public:
    TestServer()
        : server(Schema::Implementation{}.name("tst_mcpauth").version("1"))
    {}

    ~TestServer() { qDeleteAll(server.boundTcpServers()); }

    bool start()
    {
        auto tcpServer = std::make_unique<QTcpServer>();
        if (!tcpServer->listen(QHostAddress::LocalHost) || !server.bind(tcpServer.get()))
            return false;
        tcpServer.release();
        return true;
    }

    quint16 port() const { return server.boundTcpServers().constFirst()->serverPort(); }

    Server server;
};

// Reads until the end of the response header block, which every answer here
// has; nothing is waited on that the server is not required to send.
static void fetch(quint16 port, const QByteArray &wire, QByteArray *response)
{
    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client.waitForConnected());
    client.write(wire);
    QTRY_VERIFY(response->append(client.readAll()).contains("\r\n\r\n"));
}

static QByteArray post(const QByteArray &authorization)
{
    QByteArray wire = "POST / HTTP/1.1\r\nHost: 127.0.0.1\r\n";
    if (!authorization.isEmpty())
        wire += authorization + "\r\n";
    return wire + "\r\n";
}

class tst_McpAuth : public QObject
{
    Q_OBJECT

private slots:
    void servesAnyPeerWithoutToken();
    void refusesMissingToken();
    void refusesWrongToken();
    void refusesTokenOfWrongLength();
    void servesRightToken();
    void acceptsSchemeInAnyCase();
    void servesPreflightWithoutToken();
};

// A request that gets past the token check reaches the handler, which refuses
// it for a reason of its own. That reason is what tells the two apart: the
// checks in front of the handler never name a header the handler wants.
static constexpr const char *kReachedHandler = "Missing Accept header";

void tst_McpAuth::servesAnyPeerWithoutToken()
{
    TestServer test;
    QVERIFY(test.start());

    QByteArray response;
    fetch(test.port(), post({}), &response);
    QVERIFY(response.startsWith("HTTP/1.1 400"));
    QVERIFY(response.contains(kReachedHandler));
}

void tst_McpAuth::refusesMissingToken()
{
    TestServer test;
    QVERIFY(test.start());
    test.server.setAuthToken(token);

    QByteArray response;
    fetch(test.port(), post({}), &response);
    QVERIFY(response.startsWith("HTTP/1.1 401"));
    QVERIFY(response.toLower().contains("www-authenticate: bearer"));
    QVERIFY(!response.contains(kReachedHandler));
}

void tst_McpAuth::refusesWrongToken()
{
    TestServer test;
    QVERIFY(test.start());
    test.server.setAuthToken(token);

    QByteArray wrong = token;
    wrong[0] = 'f';

    QByteArray response;
    fetch(test.port(), post("Authorization: Bearer " + wrong), &response);
    QVERIFY(response.startsWith("HTTP/1.1 401"));
    QVERIFY(!response.contains(kReachedHandler));
}

void tst_McpAuth::refusesTokenOfWrongLength()
{
    TestServer test;
    QVERIFY(test.start());
    test.server.setAuthToken(token);

    QByteArray response;
    fetch(test.port(), post(QByteArray("Authorization: Bearer ") + token + "f"), &response);
    QVERIFY(response.startsWith("HTTP/1.1 401"));
    QVERIFY(!response.contains(kReachedHandler));
}

void tst_McpAuth::servesRightToken()
{
    TestServer test;
    QVERIFY(test.start());
    test.server.setAuthToken(token);

    QByteArray response;
    fetch(test.port(), post(QByteArray("Authorization: Bearer ") + token), &response);
    QVERIFY(response.startsWith("HTTP/1.1 400"));
    QVERIFY(response.contains(kReachedHandler));
}

void tst_McpAuth::acceptsSchemeInAnyCase()
{
    TestServer test;
    QVERIFY(test.start());
    test.server.setAuthToken(token);

    QByteArray response;
    fetch(test.port(), post(QByteArray("authorization: bEaReR ") + token), &response);
    QVERIFY(response.startsWith("HTTP/1.1 400"));
    QVERIFY(response.contains(kReachedHandler));
}

// The preflight a browser sends before an authenticated request carries no
// Authorization header of its own, so refusing it would refuse the question
// whether the real request may be sent at all.
void tst_McpAuth::servesPreflightWithoutToken()
{
    TestServer test;
    QVERIFY(test.start());
    test.server.setAuthToken(token);

    QByteArray response;
    fetch(test.port(), "OPTIONS / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n", &response);
    QVERIFY(response.startsWith("HTTP/1.1 200"));
}

QTEST_GUILESS_MAIN(tst_McpAuth)

#include "tst_mcpauth.moc"
