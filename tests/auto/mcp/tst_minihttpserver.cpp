// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <mcp/server/minihttpserver.h>

#include <QEventLoop>
#include <QHostAddress>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>

using namespace MiniHttp;

using ParseResult = HttpRequest::ParseResult;

static ParseResult parse(const QByteArray &wire,
                         HttpRequest *request,
                         StatusCode *status,
                         qsizetype *bodyStart = nullptr,
                         qsizetype *contentLength = nullptr)
{
    qsizetype searchStart = 0;
    qsizetype start = -1;
    qsizetype length = 0;
    const ParseResult result
        = HttpRequest::parseHeaders(wire, request, &searchStart, &start, &length, status);
    if (bodyStart)
        *bodyStart = start;
    if (contentLength)
        *contentLength = length;
    return result;
}

// Parse the same wire one byte at a time, carrying the resume position the way
// a connection does. The result must not depend on how the bytes were split.
static ParseResult parseByteByByte(const QByteArray &wire,
                                   HttpRequest *request,
                                   StatusCode *status,
                                   qsizetype *bodyStart)
{
    QByteArray buffer;
    qsizetype searchStart = 0;
    qsizetype length = 0;
    ParseResult result = ParseResult::Incomplete;
    for (const char c : wire) {
        buffer += c;
        result = HttpRequest::parseHeaders(buffer, request, &searchStart, bodyStart, &length,
                                           status);
        if (result != ParseResult::Incomplete)
            break;
    }
    return result;
}

class tst_MiniHttpServer : public QObject
{
    Q_OBJECT

private slots:
    void parsesValidRequest();
    void needsMoreDataForIncompleteHeaderBlock();

    void rejectsQuantity_data();
    void rejectsQuantity();

    void rejectsMalformedHeaderBlock_data();
    void rejectsMalformedHeaderBlock();

    void rejectsMalformedRequestLine_data();
    void rejectsMalformedRequestLine();

    void rejectsOversizedHeaderBlock();
    void rejectsTooManyHeaderFields();
    void rejectsUnsupportedTransferCoding();

    void acceptsEveryHeaderBlockTerminator_data();
    void acceptsEveryHeaderBlockTerminator();

    void combinesRepeatedFields();
    void coalescesRequestLineWhitespace();

    void resumesHeaderSearchAcrossReads_data();
    void resumesHeaderSearchAcrossReads();

    void dispatchesPipelinedRequests();
    void dispatchesRequestArrivingDuringNestedEventLoop();
    void keepsSocketWhenPeerLeavesDuringNestedEventLoop();
    void keepsSocketWhileAnsweringFromNestedEventLoop();
    void answersAfterHandlerDeletesListener();
    void closesIdleConnectionWithListener();
    void doesNotWedgeOnRejectedRequest();
    void sanitizesResponseHeaderValues();
    void terminatesChunkedBody();
};

void tst_MiniHttpServer::parsesValidRequest()
{
    HttpRequest request;
    StatusCode status = StatusCode::Ok;
    qsizetype bodyStart = -1;
    qsizetype contentLength = -1;
    const QByteArray wire = "POST /message?session=abc HTTP/1.1\r\n"
                            "Host: 127.0.0.1\r\n"
                            "Content-Length: 2\r\n"
                            "\r\n"
                            "{}";

    QCOMPARE(parse(wire, &request, &status, &bodyStart, &contentLength), ParseResult::Complete);
    QCOMPARE(request.method(), HttpRequest::Method::Post);
    QCOMPARE(request.url().path(), QString("/message"));
    QCOMPARE(request.query().queryItemValue("session"), QString("abc"));
    QCOMPARE(request.headers().value("host"), QByteArray("127.0.0.1"));
    QCOMPARE(contentLength, qsizetype(2));
    QCOMPARE(wire.mid(bodyStart, contentLength), QByteArray("{}"));
}

void tst_MiniHttpServer::needsMoreDataForIncompleteHeaderBlock()
{
    HttpRequest request;
    StatusCode status = StatusCode::Ok;
    QCOMPARE(parse("GET /sse HTTP/1.1\r\nHost: h\r\n", &request, &status),
             ParseResult::Incomplete);
}

void tst_MiniHttpServer::rejectsQuantity_data()
{
    QTest::addColumn<QByteArray>("length");
    QTest::addColumn<int>("status");

    QTest::newRow("negative") << QByteArray("-1") << int(StatusCode::BadRequest);
    QTest::newRow("large negative") << QByteArray("-2000000000") << int(StatusCode::BadRequest);
    QTest::newRow("not a number") << QByteArray("abc") << int(StatusCode::BadRequest);
    QTest::newRow("trailing garbage") << QByteArray("5abc") << int(StatusCode::BadRequest);
    QTest::newRow("hex") << QByteArray("0x10") << int(StatusCode::BadRequest);
    QTest::newRow("empty") << QByteArray("") << int(StatusCode::BadRequest);
    QTest::newRow("above body limit")
        << QByteArray::number(qint64(MaxBodySize) + 1) << int(StatusCode::PayloadTooLarge);
}

void tst_MiniHttpServer::rejectsQuantity()
{
    QFETCH(QByteArray, length);
    QFETCH(int, status);

    HttpRequest request;
    StatusCode actual = StatusCode::Ok;
    const QByteArray wire = "POST / HTTP/1.1\r\nContent-Length: " + length + "\r\n\r\nBODY";

    QCOMPARE(parse(wire, &request, &actual), ParseResult::Rejected);
    QCOMPARE(int(actual), status);
}

void tst_MiniHttpServer::rejectsMalformedHeaderBlock_data()
{
    QTest::addColumn<QByteArray>("wire");

    QTest::newRow("space before colon")
        << QByteArray("GET / HTTP/1.1\r\nContent-Length : 4\r\n\r\nBODY");
    QTest::newRow("tab before colon")
        << QByteArray("GET / HTTP/1.1\r\nContent-Length\t: 4\r\n\r\nBODY");
    QTest::newRow("obs-fold")
        << QByteArray("GET / HTTP/1.1\r\nX-Long: a\r\n b\r\n\r\n");
    QTest::newRow("no colon") << QByteArray("GET / HTTP/1.1\r\nBroken\r\n\r\n");
    QTest::newRow("empty field name") << QByteArray("GET / HTTP/1.1\r\n: value\r\n\r\n");
    QTest::newRow("separator in field name")
        << QByteArray("GET / HTTP/1.1\r\nBad(Name): v\r\n\r\n");
}

void tst_MiniHttpServer::rejectsMalformedHeaderBlock()
{
    QFETCH(QByteArray, wire);

    HttpRequest request;
    StatusCode status = StatusCode::Ok;
    QCOMPARE(parse(wire, &request, &status), ParseResult::Rejected);
    QCOMPARE(status, StatusCode::BadRequest);
}

void tst_MiniHttpServer::rejectsMalformedRequestLine_data()
{
    QTest::addColumn<QByteArray>("wire");

    QTest::newRow("no version") << QByteArray("GET /sse\r\n\r\n");
    QTest::newRow("unknown protocol") << QByteArray("get /sse SPDY/9.9\r\n\r\n");
    QTest::newRow("one token") << QByteArray("GET\r\n\r\n");
    QTest::newRow("four tokens") << QByteArray("GET /sse HTTP/1.1 extra\r\n\r\n");
    QTest::newRow("empty request line") << QByteArray("\r\n\r\n");
}

void tst_MiniHttpServer::rejectsMalformedRequestLine()
{
    QFETCH(QByteArray, wire);

    HttpRequest request;
    StatusCode status = StatusCode::Ok;
    QCOMPARE(parse(wire, &request, &status), ParseResult::Rejected);
    QCOMPARE(status, StatusCode::BadRequest);
}

void tst_MiniHttpServer::rejectsOversizedHeaderBlock()
{
    HttpRequest request;
    StatusCode status = StatusCode::Ok;

    QByteArray wire = "GET / HTTP/1.1\r\n";
    while (wire.size() <= MaxHeaderBlockSize)
        wire += "X-Pad: 0123456789012345678901234567890123456789\r\n";

    // Rejected while still incomplete: the ceiling must not wait for a terminator.
    QCOMPARE(parse(wire, &request, &status), ParseResult::Rejected);
    QCOMPARE(status, StatusCode::RequestHeaderFieldsTooLarge);

    wire += "\r\n";
    QCOMPARE(parse(wire, &request, &status), ParseResult::Rejected);
    QCOMPARE(status, StatusCode::RequestHeaderFieldsTooLarge);
}

void tst_MiniHttpServer::rejectsTooManyHeaderFields()
{
    HttpRequest request;
    StatusCode status = StatusCode::Ok;

    QByteArray wire = "GET / HTTP/1.1\r\n";
    for (int i = 0; i <= MaxHeaderCount; ++i)
        wire += "X" + QByteArray::number(i) + ": v\r\n";
    wire += "\r\n";

    QCOMPARE(parse(wire, &request, &status), ParseResult::Rejected);
    QCOMPARE(status, StatusCode::RequestHeaderFieldsTooLarge);
}

void tst_MiniHttpServer::rejectsUnsupportedTransferCoding()
{
    HttpRequest request;
    StatusCode status = StatusCode::Ok;
    const QByteArray wire = "GET /sse HTTP/1.1\r\n"
                            "Content-Length: 4\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "\r\n"
                            "5a\r\nPOST /message HTTP/1.1\r\nContent-Length: 2\r\n\r\n{}";

    QCOMPARE(parse(wire, &request, &status), ParseResult::Rejected);
    QCOMPARE(status, StatusCode::NotImplemented);
}

void tst_MiniHttpServer::acceptsEveryHeaderBlockTerminator_data()
{
    QTest::addColumn<QByteArray>("terminator");

    QTest::newRow("CRLFCRLF") << QByteArray("\r\n\r\n");
    QTest::newRow("CRLFLF") << QByteArray("\r\n\n");
    QTest::newRow("LFCRLF") << QByteArray("\n\r\n");
    QTest::newRow("LFLF") << QByteArray("\n\n");
}

void tst_MiniHttpServer::acceptsEveryHeaderBlockTerminator()
{
    QFETCH(QByteArray, terminator);

    HttpRequest request;
    StatusCode status = StatusCode::Ok;
    qsizetype bodyStart = -1;

    // The body must stay the body: a field-looking line after the terminator
    // must not end up in the header map of this request.
    const QByteArray wire = "POST / HTTP/1.1\nHost: 127.0.0.1\nAccept: */*" + terminator
                            + "mcp-session-id: FROM-BODY\r\nAccept: application/json\r\n\r\n";

    QCOMPARE(parse(wire, &request, &status, &bodyStart), ParseResult::Complete);
    QCOMPARE(request.headers().value("accept"), QByteArray("*/*"));
    QVERIFY(!request.headers().contains("mcp-session-id"));
    QCOMPARE(wire.mid(bodyStart, 17), QByteArray("mcp-session-id: F"));
}

void tst_MiniHttpServer::combinesRepeatedFields()
{
    HttpRequest request;
    StatusCode status = StatusCode::Ok;
    const QByteArray wire = "GET / HTTP/1.1\r\n"
                            "Origin: http://localhost\r\n"
                            "Origin: http://evil.example\r\n"
                            "\r\n";

    QCOMPARE(parse(wire, &request, &status), ParseResult::Complete);
    QCOMPARE(request.headers().value("origin"),
             QByteArray("http://localhost, http://evil.example"));

    // A field that must be single-valued therefore cannot be smuggled past a
    // check by repeating it: it stops parsing as a number.
    HttpRequest other;
    const QByteArray duplicateLength = "POST / HTTP/1.1\r\n"
                                       "Content-Length: 5\r\n"
                                       "Content-Length: 0\r\n"
                                       "\r\nHELLO";
    QCOMPARE(parse(duplicateLength, &other, &status), ParseResult::Rejected);
    QCOMPARE(status, StatusCode::BadRequest);
}

void tst_MiniHttpServer::coalescesRequestLineWhitespace()
{
    HttpRequest request;
    StatusCode status = StatusCode::Ok;
    QCOMPARE(parse("GET  /sse  HTTP/1.1\r\n\r\n", &request, &status), ParseResult::Complete);
    QCOMPARE(request.url().path(), QString("/sse"));
}

void tst_MiniHttpServer::resumesHeaderSearchAcrossReads_data()
{
    acceptsEveryHeaderBlockTerminator_data();
}

// Resuming must not skip a terminator that straddles the boundary between two
// reads, which is the whole risk of not restarting the search at byte 0.
void tst_MiniHttpServer::resumesHeaderSearchAcrossReads()
{
    QFETCH(QByteArray, terminator);

    const QByteArray wire = "POST /message HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 2"
                            + terminator + "{}";

    HttpRequest wholeRequest;
    StatusCode wholeStatus = StatusCode::Ok;
    qsizetype wholeBodyStart = -1;
    QCOMPARE(parse(wire, &wholeRequest, &wholeStatus, &wholeBodyStart), ParseResult::Complete);

    HttpRequest dripRequest;
    StatusCode dripStatus = StatusCode::Ok;
    qsizetype dripBodyStart = -1;
    QCOMPARE(parseByteByByte(wire, &dripRequest, &dripStatus, &dripBodyStart),
             ParseResult::Complete);
    QCOMPARE(dripBodyStart, wholeBodyStart);
    QCOMPARE(dripRequest.headers().value("host"), QByteArray("127.0.0.1"));
}

class TestServer
{
public:
    bool start() { return tcpServer.listen(QHostAddress::LocalHost) && server.bind(&tcpServer); }

    quint16 port() const { return tcpServer.serverPort(); }

    QTcpServer tcpServer;
    HttpServer server;
};

void tst_MiniHttpServer::dispatchesPipelinedRequests()
{
    TestServer test;
    QVERIFY(test.start());
    QStringList seen;
    test.server.route("/a", HttpRequest::Method::Get,
                      [&seen](const HttpRequest &req, HttpResponder &) {
                          seen << req.url().path();
                      });
    test.server.route("/b", HttpRequest::Method::Get,
                      [&seen](const HttpRequest &req, HttpResponder &) {
                          seen << req.url().path();
                      });

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, test.port());
    QVERIFY(client.waitForConnected());
    client.write("GET /a HTTP/1.1\r\n\r\nGET /b HTTP/1.1\r\n\r\n");

    // Both requests arrive in one segment; neither handler closes the socket,
    // so the second must not wait for further input.
    QTRY_COMPARE(seen, QStringList({"/a", "/b"}));
}

// A handler may run a nested event loop - an MCP tool opening a modal dialog
// does exactly that. QAbstractSocket does not emit readyRead while one is
// already being handled, so a request that arrives meanwhile is only in the
// socket, and nothing else will arrive to fetch it.
void tst_MiniHttpServer::dispatchesRequestArrivingDuringNestedEventLoop()
{
    TestServer test;
    QVERIFY(test.start());

    QTcpSocket client;
    QStringList seen;

    test.server.route("/a", HttpRequest::Method::Get,
                      [&](const HttpRequest &req, HttpResponder &) {
                          seen << req.url().path();
                          QEventLoop loop;
                          QTimer::singleShot(0, &client, [&client] {
                              client.write("GET /b HTTP/1.1\r\n\r\n");
                          });
                          QTimer::singleShot(100, &loop, &QEventLoop::quit);
                          loop.exec();
                      });
    test.server.route("/b", HttpRequest::Method::Get,
                      [&seen](const HttpRequest &req, HttpResponder &) {
                          seen << req.url().path();
                      });

    client.connectToHost(QHostAddress::LocalHost, test.port());
    QVERIFY(client.waitForConnected());
    client.write("GET /a HTTP/1.1\r\n\r\n");

    QTRY_COMPARE(seen, QStringList({"/a", "/b"}));
}

// The client giving up while a handler waits in a nested event loop must not
// delete the socket under the readyRead emission that is still on the stack.
void tst_MiniHttpServer::keepsSocketWhenPeerLeavesDuringNestedEventLoop()
{
    TestServer test;
    QVERIFY(test.start());

    QTcpSocket client;
    QPointer<QTcpSocket> socket;
    bool handled = false;
    bool aliveAfterLoop = false;

    test.server.route("/a", HttpRequest::Method::Get,
                      [&](const HttpRequest &, HttpResponder &responder) {
                          socket = responder.socket();
                          QEventLoop loop;
                          // Queued behind the deferred delete that the
                          // disconnect may post, so that it had its chance.
                          connect(socket, &QTcpSocket::disconnected, &loop, [&loop] {
                              QMetaObject::invokeMethod(&loop, &QEventLoop::quit,
                                                        Qt::QueuedConnection);
                          });
                          QTimer::singleShot(0, &client, [&client] { client.abort(); });
                          loop.exec();
                          aliveAfterLoop = !socket.isNull();
                          handled = true;
                      });

    client.connectToHost(QHostAddress::LocalHost, test.port());
    QVERIFY(client.waitForConnected());
    client.write("GET /a HTTP/1.1\r\n\r\n");

    QTRY_VERIFY(handled);
    QVERIFY(aliveAfterLoop);
    QTRY_VERIFY(socket.isNull());
}

// An MCP tool answers from inside a menu's or dialog's event loop. The answer
// closes the connection, and the socket must survive that loop: its readyRead
// emission is still on the stack below it.
void tst_MiniHttpServer::keepsSocketWhileAnsweringFromNestedEventLoop()
{
    TestServer test;
    QVERIFY(test.start());

    QTcpSocket client;
    bool aliveInLoop = false;
    QPointer<QTcpSocket> serverSide;

    test.server.route("/a", HttpRequest::Method::Get,
                      [&](const HttpRequest &, HttpResponder &responder) {
                          serverSide = test.server.findChild<QTcpSocket *>();
                          QEventLoop loop;
                          connect(&client, &QTcpSocket::disconnected,
                                  &loop, &QEventLoop::quit);
                          QTimer::singleShot(0, &loop, [&responder] {
                              responder.write(StatusCode::Ok);
                          });
                          loop.exec();
                          aliveInLoop = serverSide;
                      });

    client.connectToHost(QHostAddress::LocalHost, test.port());
    QVERIFY(client.waitForConnected());
    client.write("GET /a HTTP/1.1\r\n\r\n");

    QTRY_VERIFY(client.state() == QAbstractSocket::UnconnectedState);
    QVERIFY(aliveInLoop);
    QTRY_VERIFY(!serverSide);
}

// An MCP tool that changes the server settings restarts the server, which
// deletes the listener the request came in on.
void tst_MiniHttpServer::answersAfterHandlerDeletesListener()
{
    HttpServer server;
    auto listener = new QTcpServer;
    QVERIFY(listener->listen(QHostAddress::LocalHost));
    QVERIFY(server.bind(listener));
    const quint16 port = listener->serverPort();

    server.route("/a", HttpRequest::Method::Get,
                 [&listener](const HttpRequest &, HttpResponder &responder) {
                     delete listener;
                     listener = nullptr;
                     responder.write("done", HttpHeaders{}, StatusCode::Ok);
                 });

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client.waitForConnected());
    client.write("GET /a HTTP/1.1\r\n\r\n");

    QByteArray response;
    QTRY_VERIFY((response.append(client.readAll()),
                 client.state() == QAbstractSocket::UnconnectedState));
    response.append(client.readAll());
    QVERIFY(!listener);
    QVERIFY(server.servers().isEmpty());
    QVERIFY(response.startsWith("HTTP/1.1 200"));
    QVERIFY(response.endsWith("done"));
}

// A server restart deletes the listeners, which must also end the open
// notification streams: a client holding a revoked token stays locked out.
void tst_MiniHttpServer::closesIdleConnectionWithListener()
{
    HttpServer server;
    auto listener = new QTcpServer;
    QVERIFY(listener->listen(QHostAddress::LocalHost));
    QVERIFY(server.bind(listener));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, listener->serverPort());
    QVERIFY(client.waitForConnected());
    QTRY_VERIFY(server.findChild<QTcpSocket *>());

    delete listener;
    QTRY_VERIFY(client.state() == QAbstractSocket::UnconnectedState);
    QTRY_VERIFY(!server.findChild<QTcpSocket *>());
}

void tst_MiniHttpServer::doesNotWedgeOnRejectedRequest()
{
    TestServer test;
    QVERIFY(test.start());
    int dispatched = 0;
    test.server.route("/a", HttpRequest::Method::Get,
                      [&dispatched](const HttpRequest &, HttpResponder &responder) {
                          ++dispatched;
                          responder.write(StatusCode::Ok);
                      });

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, test.port());
    QVERIFY(client.waitForConnected());
    client.write("GET /a HTTP/1.1\r\nContent-Length: -1\r\n\r\n");

    // The connection is answered and closed instead of being left parsing a
    // buffer that can never advance.
    QTRY_VERIFY(client.state() == QAbstractSocket::UnconnectedState);
    QCOMPARE(dispatched, 0);
    QVERIFY(client.readAll().startsWith("HTTP/1.1 400"));
}

void tst_MiniHttpServer::sanitizesResponseHeaderValues()
{
    TestServer test;
    QVERIFY(test.start());
    test.server.route("/a", HttpRequest::Method::Get,
                      [](const HttpRequest &, HttpResponder &responder) {
                          HttpHeaders headers;
                          headers.set("x-echo", "value\r\nInjected: yes");
                          responder.write("body", headers, StatusCode::Ok);
                      });

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, test.port());
    QVERIFY(client.waitForConnected());
    client.write("GET /a HTTP/1.1\r\n\r\n");

    QByteArray response;
    QTRY_VERIFY(response.append(client.readAll()).contains("\r\n\r\n"));
    QVERIFY(!response.contains("\r\nInjected:"));
    QVERIFY(response.contains("x-echo: valueInjected: yes"));
    QCOMPARE(response.count("content-length"), 1);
}

void tst_MiniHttpServer::terminatesChunkedBody()
{
    TestServer test;
    QVERIFY(test.start());
    test.server.route("/sse", HttpRequest::Method::Get,
                      [](const HttpRequest &, HttpResponder &responder) {
                          HttpHeaders headers;
                          headers.set("content-type", "text/event-stream");
                          responder.writeBeginChunked(headers, StatusCode::Ok);
                          responder.writeChunk("data: a\n\n");
                          responder.writeChunk({});
                          responder.writeEndChunked("\n\n");
                      });

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, test.port());
    QVERIFY(client.waitForConnected());
    client.write("GET /sse HTTP/1.1\r\n\r\n");

    QByteArray response;
    QTRY_VERIFY((response.append(client.readAll()),
                 client.state() == QAbstractSocket::UnconnectedState));
    response.append(client.readAll());

    const qsizetype headerEnd = response.indexOf("\r\n\r\n");
    QVERIFY(headerEnd > 0);
    QVERIFY(response.left(headerEnd).contains("\r\ntransfer-encoding: chunked"));
    QCOMPARE(response.mid(headerEnd + 4), "9\r\ndata: a\n\n\r\n2\r\n\n\n\r\n0\r\n\r\n");
}

QTEST_GUILESS_MAIN(tst_MiniHttpServer)

#include "tst_minihttpserver.moc"
