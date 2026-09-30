// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <mcp/server/mcpserver.h>

#include <utils/result.h>

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include <memory>
#include <vector>

using namespace Mcp;

static constexpr const char *kPendingTool = "pending";

class TestServer
{
public:
    TestServer()
        : server(Schema::Implementation{}.name("tst_mcpsse").version("1"))
    {
        // A call that never finishes, held past its callback so the server's
        // weak entry for it can still be locked: reclaiming a session cancels
        // the calls left open on it, and that cancellation is what writes from
        // inside ~SseStream.
        const Server::ToolInterfaceCallback keepPending =
            [this](const Schema::CallToolRequestParams &, const ToolInterface &tool) {
                pending.push_back(std::make_shared<ToolInterface>(tool));
                return Utils::ResultOk;
            };

        server.addTool(
            Schema::Tool()
                .name(kPendingTool)
                .title(kPendingTool)
                .description(kPendingTool)
                .inputSchema(Schema::Tool::InputSchema()),
            keepPending);
    }

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
    std::vector<std::shared_ptr<ToolInterface>> pending;
};

static QByteArray requestWire(const QString &sessionId, const QJsonObject &message)
{
    const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
    return "POST /message?session=" + sessionId.toUtf8()
           + " HTTP/1.1\r\n"
             "Host: 127.0.0.1\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: "
           + QByteArray::number(body.size()) + "\r\n\r\n" + body;
}

static QJsonObject ping(int id)
{
    return QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"method", "ping"}, {"params", QJsonObject{}}};
}

static QJsonObject callPendingTool(int id)
{
    return QJsonObject{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "tools/call"},
        {"params", QJsonObject{{"name", QString::fromLatin1(kPendingTool)}}}};
}

// Reads until the end of the response header block, which every answer here
// has; nothing is waited on that the server is not required to send. Returns
// an empty array if the exchange did not complete, which fails every check on
// it below. The timeout is the caller's, because an exchange retried inside a
// wait must not be able to spend that wait's whole budget on one round.
static QByteArray exchange(quint16 port, const QByteArray &wire, int timeoutMs = 5000)
{
    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    if (!client.waitForConnected(timeoutMs))
        return {};
    client.write(wire);

    QByteArray response;
    const auto answered = [&] { return response.append(client.readAll()).contains("\r\n\r\n"); };
    if (!QTest::qWaitFor(answered, timeoutMs))
        return {};
    return response;
}

class tst_McpSse : public QObject
{
    Q_OBJECT

private slots:
    void survivesASessionReclaimedFromInsideTheFanOut();
    void survivesASecondSessionReclaimedByTheFirstsCancellation();
};

// The stream's first event names the endpoint its session is to be driven
// through, so the session id is read off the wire rather than guessed.
static void openStream(quint16 port, QTcpSocket *stream, QByteArray *received, QString *sessionId)
{
    stream->connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(stream->waitForConnected());
    stream->write("GET /sse HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");

    // Waited on to its terminator, not just to the marker: the server writes
    // the event as one chunk, but TCP may hand it over split anywhere, and the
    // id itself is the part after the marker.
    const QByteArray marker = "data: /message?session=";
    const auto endpointArrived = [&] {
        received->append(stream->readAll());
        const qsizetype at = received->indexOf(marker);
        return at >= 0 && received->indexOf("\n\n", at + marker.size()) >= 0;
    };
    QTRY_VERIFY(endpointArrived());

    const qsizetype start = received->indexOf(marker) + marker.size();
    const qsizetype end = received->indexOf('\n', start);
    *sessionId = QString::fromUtf8(received->sliced(start, end - start).trimmed());
    QVERIFY(!sessionId->isEmpty());
}

// Writing to a stream whose peer is gone prunes it, and ~SseStream reclaims the
// session behind it, which cancels the calls left open on that session and
// writes their errors out through the very fan-out that is pruning.
void tst_McpSse::survivesASessionReclaimedFromInsideTheFanOut()
{
    TestServer test;
    QVERIFY(test.start());

    // Two streams, because pruning the first moves the second down over it, and
    // the entry the move leaves behind is what the write from inside
    // ~SseStream reaches.
    QTcpSocket first;
    QByteArray fromFirst;
    QString firstSession;
    openStream(test.port(), &first, &fromFirst, &firstSession);

    QTcpSocket second;
    QByteArray fromSecond;
    QString secondSession;
    openStream(test.port(), &second, &fromSecond, &secondSession);

    const QByteArray called
        = exchange(test.port(), requestWire(firstSession, callPendingTool(1)));
    QVERIFY(called.startsWith("HTTP/1.1 200"));
    QCOMPARE(test.pending.size(), size_t(1));

    // Dropped without notice: the server finds out the next time it writes to
    // the stream.
    first.abort();

    // That write is driven from another socket, whose request cannot be ordered
    // against the dropped connection, so it is retried until the teardown has
    // run - each round the real exchange whose answer reports it. A session
    // that has been reclaimed is refused, which is where a client sees it. One
    // round is given far less than the whole budget, so a slow one cannot
    // exhaust it and report the teardown as the thing that never happened.
    QByteArray refused;
    const auto sessionRefused = [&] {
        refused = exchange(test.port(), requestWire(firstSession, ping(2)), 500);
        return refused.startsWith("HTTP/1.1 404");
    };
    QVERIFY(QTest::qWaitFor(sessionRefused, 30000));

    // The stream that was moved down over the pruned one is still the one the
    // fan-out writes to.
    const QByteArray answered = exchange(test.port(), requestWire(secondSession, ping(3)));
    QVERIFY(answered.startsWith("HTTP/1.1 200"));
    QVERIFY(QTest::qWaitFor(
        [&] { return fromSecond.append(second.readAll()).contains("\"id\":3"); }));
}

// One reclaimed session can reclaim the next: cancelling the calls left open on
// the first writes their errors out, and that write prunes the second dead
// stream, whose teardown removes its own calls from the same map the first
// teardown is walking.
//
// Which entry that walk is left standing on follows the order the session ids
// sort in, and the ids are the server's to choose, so the pair is searched for
// rather than assumed: the session pruned first - the one whose stream was
// opened first - has to be the one that sorts first.
void tst_McpSse::survivesASecondSessionReclaimedByTheFirstsCancellation()
{
    TestServer test;
    QVERIFY(test.start());

    constexpr int maxStreams = 8;
    std::vector<std::unique_ptr<QTcpSocket>> streams;
    std::vector<std::unique_ptr<QByteArray>> received;
    QStringList sessions;
    int reclaimed = -1;
    int alongWithIt = -1;

    for (int opened = 0; opened < maxStreams && reclaimed < 0; ++opened) {
        streams.push_back(std::make_unique<QTcpSocket>());
        received.push_back(std::make_unique<QByteArray>());
        QString session;
        openStream(test.port(), streams.back().get(), received.back().get(), &session);
        QVERIFY(!session.isEmpty());
        sessions.append(session);

        for (int earlier = 0; earlier < opened; ++earlier) {
            if (sessions.at(earlier) < session) {
                reclaimed = earlier;
                alongWithIt = opened;
                break;
            }
        }
    }
    QVERIFY(reclaimed >= 0);

    // Only these two hold a pending call, so they are the only two entries in
    // the map the teardown walks, and they are adjacent in it.
    for (int id : {reclaimed, alongWithIt}) {
        const QByteArray called
            = exchange(test.port(), requestWire(sessions.at(id), callPendingTool(id + 1)));
        QVERIFY(called.startsWith("HTTP/1.1 200"));
    }
    QCOMPARE(test.pending.size(), size_t(2));

    streams.at(reclaimed)->abort();
    streams.at(alongWithIt)->abort();

    // Both teardowns run off this one write, the second from inside the first.
    // Reaching the refusal at all is the server having survived it; that the
    // second session is gone too is what says the nested teardown ran.
    QByteArray refused;
    const auto firstRefused = [&] {
        refused = exchange(test.port(), requestWire(sessions.at(reclaimed), ping(90)), 500);
        return refused.startsWith("HTTP/1.1 404");
    };
    QVERIFY(QTest::qWaitFor(firstRefused, 30000));

    const QByteArray second
        = exchange(test.port(), requestWire(sessions.at(alongWithIt), ping(91)));
    QVERIFY(second.startsWith("HTTP/1.1 404"));
}

QTEST_GUILESS_MAIN(tst_McpSse)

#include "tst_mcpsse.moc"
