// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include <QCommandLineParser>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTest>
#include <QThread>
#include <QTimer>

class McpServerTest : public QObject
{
    Q_OBJECT

public:
    explicit McpServerTest(QObject *parent = nullptr)
        : QObject(parent)
    {}

private:
    struct HttpResponse
    {
        int statusCode = -1;
        QMap<QString, QString> headers;
        QByteArray body;
    };

    QString sendTcpRequest(const QString &request, int timeoutMs = 5000)
    {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, 3001);
        if (!socket.waitForConnected(timeoutMs)) {
            throw std::runtime_error(
                QStringLiteral("TCP connect failed: %1").arg(socket.errorString()).toStdString());
        }

        QByteArray payload = request.toUtf8() + '\n';
        if (socket.write(payload) != payload.size() || !socket.waitForBytesWritten(timeoutMs)) {
            throw std::runtime_error(
                QStringLiteral("TCP write failed: %1").arg(socket.errorString()).toStdString());
        }

        // Use a QEventLoop since waitForReadyRead() would cause issues
        QByteArray response;
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.setInterval(timeoutMs);

        QObject::connect(&socket, &QTcpSocket::readyRead, &loop, [&]() { loop.quit(); });
        QObject::connect(&socket, &QTcpSocket::disconnected, &loop, [&]() { loop.quit(); });
        QObject::connect(&socket, &QTcpSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
            loop.quit();
        });
        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() { loop.quit(); });

        while (true) {
            timeoutTimer.start();
            loop.exec();

            if (socket.bytesAvailable())
                response += socket.readAll();

            // Try to parse the accumulated bytes as JSON.
            QJsonParseError err;
            QJsonDocument::fromJson(response, &err);
            if (err.error == QJsonParseError::NoError)
                break; // full JSON received

            if (!timeoutTimer.isActive())
                throw std::runtime_error("TCP read timeout");

            if (socket.state() != QAbstractSocket::ConnectedState) {
                throw std::runtime_error(QStringLiteral("TCP socket closed / error: %1")
                                             .arg(socket.errorString())
                                             .toStdString());
            }
        }

        socket.disconnectFromHost();
        return QString::fromUtf8(response).trimmed();
    }

    QByteArray sendHttpRequest(
        const QString &method,
        const QString &path = QStringLiteral("/"),
        const QMap<QString, QString> &headers = {},
        const QByteArray &body = {},
        int timeoutMs = 5000)
    {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, 3001);
        if (!socket.waitForConnected(timeoutMs)) {
            throw std::runtime_error(
                QStringLiteral("HTTP connect failed: %1").arg(socket.errorString()).toStdString());
        }

        // Build the raw HTTP request
        QByteArray request;
        request += QStringLiteral("%1 %2 HTTP/1.1\r\n").arg(method, path).toUtf8();
        request += QByteArrayLiteral("Host: localhost\r\n");

        for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
            request += QStringLiteral("%1: %2\r\n").arg(it.key(), it.value()).toUtf8();

        if (!body.isEmpty())
            request += QStringLiteral("Content-Length: %1\r\n").arg(body.size()).toUtf8();

        request += "\r\n";
        request += body;

        if (socket.write(request) != request.size() || !socket.waitForBytesWritten(timeoutMs)) {
            throw std::runtime_error(
                QStringLiteral("HTTP write failed: %1").arg(socket.errorString()).toStdString());
        }

        // Read the answer using a local event loop
        QByteArray response;
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.setInterval(timeoutMs);

        QObject::connect(&socket, &QTcpSocket::readyRead, &loop, [&]() { loop.quit(); });
        QObject::connect(&socket, &QTcpSocket::disconnected, &loop, [&]() { loop.quit(); });
        QObject::connect(&socket, &QTcpSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
            loop.quit();
        });
        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() { loop.quit(); });

        // Helper lambda – does the “do we have the whole HTTP message?” check.
        auto isComplete = [&](const QByteArray &buf) -> bool {
            int headerEnd = buf.indexOf("\r\n\r\n");
            if (headerEnd == -1)
                return false; // still waiting for headers

            QByteArray headerPart = buf.left(headerEnd + 4);
            QRegularExpression
                re(QStringLiteral("Content-Length: (\\d+)"),
                   QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = re.match(QString::fromUtf8(headerPart));

            if (match.hasMatch()) {
                int contentLen = match.captured(1).toInt();
                int bodyStart = headerEnd + 4;
                int bodyNow = buf.size() - bodyStart;
                return bodyNow >= contentLen;
            }

            // No Content‑Length → treat as complete (e.g. 204/304)
            return true;
        };

        while (true) {
            timeoutTimer.start();
            loop.exec();

            if (socket.bytesAvailable())
                response += socket.readAll();

            if (isComplete(response))
                break; // success

            if (!timeoutTimer.isActive())
                throw std::runtime_error("HTTP read timeout");

            if (socket.state() != QAbstractSocket::ConnectedState)
                throw std::runtime_error(QStringLiteral("HTTP socket closed / error: %1")
                                             .arg(socket.errorString())
                                             .toStdString());
        }

        socket.disconnectFromHost();
        return response;
    }

    HttpResponse parseHttpResponse(const QByteArray &raw)
    {
        HttpResponse resp;

        QList<QByteArray> lines = raw.split('\n');
        if (lines.isEmpty())
            return resp;

        // ---- status line ------------------------------------------------
        QString statusLine = QString::fromUtf8(lines.takeFirst()).trimmed();
        QRegularExpression statusRx(QStringLiteral(R"(^HTTP\/\d\.\d (\d{3}) )"));
        QRegularExpressionMatch sm = statusRx.match(statusLine);
        if (sm.hasMatch())
            resp.statusCode = sm.captured(1).toInt();

        // ---- headers ----------------------------------------------------
        while (!lines.isEmpty()) {
            QByteArray line = lines.takeFirst();
            if (line.trimmed().isEmpty())
                break; // empty line → end of headers

            int colon = line.indexOf(':');
            if (colon > 0) {
                QString key = QString::fromUtf8(line.left(colon)).trimmed().toLower();
                QString value = QString::fromUtf8(line.mid(colon + 1)).trimmed();
                resp.headers.insert(key, value);
            }
        }

        // ---- body -------------------------------------------------------
        QByteArray body;
        for (const QByteArray &l : qAsConst(lines))
            body += l + '\n';
        resp.body = body.trimmed();
        return resp;
    }

    /*! Open a persistent SSE connection (GET /sse) and discard the initial
        “endpoint” event. Returns a socket that stays open for the duration of the
        test. */
    QTcpSocket *openSseSocket(int timeoutMs = 5000)
    {
        QTcpSocket *socket = new QTcpSocket(this);
        socket->connectToHost(QHostAddress::LocalHost, 3001);
        if (!socket->waitForConnected(timeoutMs)) {
            const QString errorString = socket->errorString();
            delete socket;
            throw std::runtime_error(
                QStringLiteral("SSE connect failed: %1").arg(errorString).toStdString());
        }

        // Send the GET request for the SSE stream
        QByteArray request;
        request += "GET /sse HTTP/1.1\r\n";
        request += "Host: localhost\r\n";
        request += "Accept: text/event-stream\r\n";
        request += "\r\n";

        if (socket->write(request) != request.size() || !socket->waitForBytesWritten(timeoutMs)) {
            const QString errorString = socket->errorString();
            delete socket;
            throw std::runtime_error(
                QStringLiteral("SSE write failed: %1").arg(errorString).toStdString());
        }

        // Wait for the HTTP status line + headers + the first “endpoint” event.
        QByteArray buffer;
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.setInterval(timeoutMs);

        QObject::connect(socket, &QTcpSocket::readyRead, &loop, [&]() { loop.quit(); });
        QObject::connect(socket, &QTcpSocket::disconnected, &loop, [&]() { loop.quit(); });
        QObject::connect(socket, &QTcpSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
            loop.quit();
        });
        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() { loop.quit(); });

        // Helper: have we received at least one complete SSE event?
        auto haveCompleteEvent = [&](const QByteArray &buf) -> bool {
            return buf.contains("\n\n"); // an empty line terminates an event
        };

        while (true) {
            timeoutTimer.start();
            loop.exec();

            if (socket->bytesAvailable())
                buffer += socket->readAll();

            if (haveCompleteEvent(buffer))
                break;

            if (!timeoutTimer.isActive())
                throw std::runtime_error("SSE connect timeout");

            if (socket->state() != QAbstractSocket::ConnectedState)
                throw std::runtime_error(QStringLiteral("SSE socket closed / error: %1")
                                             .arg(socket->errorString())
                                             .toStdString());
        }

        // Discard everything up to the first double‑newline (the “endpoint” event).
        int endPos = buffer.indexOf("\n\n");
        buffer.remove(0, endPos + 2);
        Q_UNUSED(buffer);

        return socket; // still open, ready to receive further events
    }

    /*! Read the next SSE *message* event from an already‑opened SSE socket.
        Returns the raw JSON payload (the content of the `data:` line). */
    QByteArray readSseMessage(QTcpSocket *socket, int timeoutMs = 5000)
    {
        QByteArray buffer;
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.setInterval(timeoutMs);

        QObject::connect(socket, &QTcpSocket::readyRead, &loop, [&]() { loop.quit(); });
        QObject::connect(socket, &QTcpSocket::disconnected, &loop, [&]() { loop.quit(); });
        QObject::connect(socket, &QTcpSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
            loop.quit();
        });
        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() { loop.quit(); });

        while (true) {
            timeoutTimer.start();
            loop.exec();

            if (socket->bytesAvailable())
                buffer += socket->readAll();

            // Look for an event that starts with “event: message”
            int eventPos = buffer.indexOf("event: message\n");
            if (eventPos != -1) {
                // Find the data line after the event line
                int dataPos = buffer.indexOf("data: ", eventPos);
                if (dataPos != -1) {
                    int lineEnd = buffer.indexOf('\n', dataPos);
                    if (lineEnd != -1) {
                        QByteArray json = buffer.mid(dataPos + 6, lineEnd - (dataPos + 6));
                        // Remove the processed part so the next call starts fresh
                        buffer.remove(0, lineEnd + 2); // also drop the empty line after the event
                        return json;
                    }
                }
            }

            if (!timeoutTimer.isActive())
                throw std::runtime_error("SSE read timeout");

            if (socket->state() != QAbstractSocket::ConnectedState)
                throw std::runtime_error(QStringLiteral("SSE socket closed / error: %1")
                                             .arg(socket->errorString())
                                             .toStdString());
        }
    }

    void assertJsonRpcInitialize(const QJsonObject &obj, const QString &testName)
    {
        QVERIFY2(
            obj.value("jsonrpc").toString() == QLatin1String("2.0"),
            qPrintable(testName + ": jsonrpc version mismatch"));
        QVERIFY2(obj.contains("result"), qPrintable(testName + ": missing result field"));
        QJsonObject result = obj.value("result").toObject();
        QVERIFY2(result.contains("serverInfo"), qPrintable(testName + ": missing serverInfo"));
        QJsonObject serverInfo = result.value("serverInfo").toObject();
        QVERIFY2(
            serverInfo.value("name").toString() == QLatin1String("Qt Creator MCP Server"),
            qPrintable(testName + ": unexpected server name"));
    }

private slots:
    void test_serverConnectivity()
    {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, 3001);
        bool ok = socket.waitForConnected(3000);
        socket.disconnectFromHost();
        QVERIFY2(ok, "Unable to connect to localhost:3001");
    }

    void test_tcpInitialize()
    {
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["method"] = "initialize";
        req["id"] = 1;

        QJsonObject params;
        params["protocolVersion"] = "2024-11-05";
        params["capabilities"] = QJsonObject();
        QJsonObject clientInfo;
        clientInfo["name"] = "TCP Test Client";
        clientInfo["version"] = "1.0.0";
        params["clientInfo"] = clientInfo;

        req["params"] = params;

        QString request = QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
        QString responseText = sendTcpRequest(request);
        QVERIFY2(!responseText.isEmpty(), "Empty TCP response");

        QJsonParseError err;
        QJsonDocument respDoc = QJsonDocument::fromJson(responseText.toUtf8(), &err);
        QVERIFY2(
            err.error == QJsonParseError::NoError,
            qPrintable("TCP JSON parse error: " + err.errorString()));

        QJsonObject respObj = respDoc.object();
        assertJsonRpcInitialize(respObj, "TCP Initialize");
    }

    void test_tcpToolsList()
    {
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["method"] = "tools/list";
        req["id"] = 2;
        QString request = QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
        QString responseText = sendTcpRequest(request);
        QVERIFY2(!responseText.isEmpty(), "Empty TCP tools/list response");

        QJsonParseError err;
        QJsonDocument respDoc = QJsonDocument::fromJson(responseText.toUtf8(), &err);
        QVERIFY2(
            err.error == QJsonParseError::NoError,
            qPrintable("TCP tools/list JSON parse error: " + err.errorString()));

        QJsonObject respObj = respDoc.object();
        QVERIFY2(
            respObj.value("jsonrpc").toString() == QLatin1String("2.0"),
            "TCP tools/list: wrong jsonrpc version");

        QJsonObject result = respObj.value("result").toObject();
        QVERIFY2(result.contains("tools"), "TCP tools/list: missing tools array");
        QJsonArray tools = result.value("tools").toArray();
        QVERIFY2(!tools.isEmpty(), "TCP tools/list: tools array is empty");
    }

    // HTTP SSE tests
    void test_httpGetInfo()
    {
        // This request is still a plain HTTP GET (no SSE). It should return a 200.
        QByteArray raw = sendHttpRequest(QStringLiteral("GET"), QStringLiteral("/"));
        HttpResponse resp = parseHttpResponse(raw);
        QCOMPARE(resp.statusCode, 200);
    }

    void test_httpInitialize()
    {
        // Open an SSE stream first.
        QTcpSocket *sseSocket = openSseSocket();

        // Send the JSON‑RPC request as a POST to the SSE endpoint.
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["method"] = "initialize";
        req["id"] = 3;

        QJsonObject params;
        params["protocolVersion"] = "2024-11-05";
        params["capabilities"] = QJsonObject();
        QJsonObject clientInfo;
        clientInfo["name"] = "HTTP Test Client";
        clientInfo["version"] = "1.0.0";
        params["clientInfo"] = clientInfo;
        req["params"] = params;

        QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);
        QMap<QString, QString> hdr;
        hdr.insert(QStringLiteral("Content-Type"), QStringLiteral("application/json"));

        // POST to /sse – we expect a 204 No Content response.
        QByteArray raw
            = sendHttpRequest(QStringLiteral("POST"), QStringLiteral("/sse"), hdr, payload);
        HttpResponse resp = parseHttpResponse(raw);
        QCOMPARE(resp.statusCode, 204);

        // The real JSON‑RPC reply arrives via SSE.
        QByteArray sseJson = readSseMessage(sseSocket);
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(sseJson, &err);
        QVERIFY2(
            err.error == QJsonParseError::NoError,
            qPrintable(QStringLiteral("SSE JSON parse error: %1").arg(err.errorString())));
        assertJsonRpcInitialize(doc.object(), "HTTP Initialize (SSE)");

        sseSocket->disconnectFromHost();
        sseSocket->deleteLater();
    }

    void test_httpToolsList()
    {
        QTcpSocket *sseSocket = openSseSocket();

        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["method"] = "tools/list";
        req["id"] = 4;

        QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);
        QMap<QString, QString> hdr;
        hdr.insert(QStringLiteral("Content-Type"), QStringLiteral("application/json"));

        QByteArray raw
            = sendHttpRequest(QStringLiteral("POST"), QStringLiteral("/sse"), hdr, payload);
        HttpResponse resp = parseHttpResponse(raw);
        QCOMPARE(resp.statusCode, 204);

        QByteArray sseJson = readSseMessage(sseSocket);
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(sseJson, &err);
        QVERIFY2(
            err.error == QJsonParseError::NoError,
            qPrintable(QStringLiteral("SSE JSON parse error: %1").arg(err.errorString())));

        QJsonObject obj = doc.object();
        QVERIFY2(
            obj.value("jsonrpc").toString() == QLatin1String("2.0"),
            "HTTP tools/list: wrong jsonrpc version");
        QJsonObject result = obj.value("result").toObject();
        QVERIFY2(result.contains("tools"), "HTTP tools/list: missing tools array");
        QJsonArray tools = result.value("tools").toArray();
        QVERIFY2(!tools.isEmpty(), "HTTP tools/list: tools array is empty");

        sseSocket->disconnectFromHost();
        sseSocket->deleteLater();
    }

    void test_httpCors()
    {
        QMap<QString, QString> hdr;
        hdr.insert(QStringLiteral("Access-Control-Request-Method"), QStringLiteral("POST"));
        hdr.insert(QStringLiteral("Access-Control-Request-Headers"), QStringLiteral("Content-Type"));

        QByteArray raw = sendHttpRequest(QStringLiteral("OPTIONS"), QStringLiteral("/"), hdr);
        HttpResponse resp = parseHttpResponse(raw);

        // The Python test expects a 204 No Content and the three CORS headers.
        QCOMPARE(resp.statusCode, 204);

        QStringList required
            = {QStringLiteral("access-control-allow-origin"),
               QStringLiteral("access-control-allow-methods"),
               QStringLiteral("access-control-allow-headers")};
        for (const QString &h : required) {
            QVERIFY2(resp.headers.contains(h), qPrintable("Missing CORS header: " + h));
        }
    }

    void test_protocolDetection()
    {
        QByteArray raw = sendHttpRequest(QStringLiteral("GET"), QStringLiteral("/"));
        // If the server answered with an HTTP status line we know it is HTTP.
        QVERIFY2(raw.startsWith("HTTP/1.1"), "Response does not start with HTTP/1.1");
    }

    void test_protocolConsistency()
    {
        // Build request with a unique id
        const int testId = 99;
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["method"] = "initialize";
        req["id"] = testId;

        QJsonObject params;
        params["protocolVersion"] = "2024-11-05";
        params["capabilities"] = QJsonObject();
        QJsonObject clientInfo;
        clientInfo["name"] = "Consistency Test Client";
        clientInfo["version"] = "1.0.0";
        params["clientInfo"] = clientInfo;
        req["params"] = params;

        QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);

        // ---- HTTP (SSE) -------------------------------------------------
        QTcpSocket *sseSocket = openSseSocket();

        QMap<QString, QString> hdr;
        hdr.insert(QStringLiteral("Content-Type"), QStringLiteral("application/json"));
        QByteArray rawHttp
            = sendHttpRequest(QStringLiteral("POST"), QStringLiteral("/sse"), hdr, payload);
        HttpResponse httpResp = parseHttpResponse(rawHttp);
        bool httpOk = false;
        if (httpResp.statusCode == 204) {
            QByteArray sseJson = readSseMessage(sseSocket);
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(sseJson, &err);
            if (err.error == QJsonParseError::NoError) {
                QJsonObject obj = doc.object();
                httpOk = (obj.value("id").toInt() == testId);
            }
        }
        sseSocket->disconnectFromHost();
        sseSocket->deleteLater();

        // ---- TCP --------------------------------------------------
        bool tcpOk = false;
        QString rawTcp = sendTcpRequest(QString::fromUtf8(payload));
        QJsonParseError errTcp;
        QJsonDocument docTcp = QJsonDocument::fromJson(rawTcp.toUtf8(), &errTcp);
        if (errTcp.error == QJsonParseError::NoError) {
            QJsonObject obj = docTcp.object();
            tcpOk = (obj.value("id").toInt() == testId);
        }

        QVERIFY2(
            httpOk && tcpOk,
            qPrintable(QString("Protocol consistency failed – HTTP ok: %1, TCP ok: %2")
                           .arg(httpOk ? QStringLiteral("true") : "false")
                           .arg(tcpOk ? QStringLiteral("true") : "false")));
    }

    void test_sseMultipleClients()
    {
        // Open two independent SSE streams
        QTcpSocket *sse1 = openSseSocket();
        QTcpSocket *sse2 = openSseSocket();

        // Send a simple request (tools/list) via POST to the SSE endpoint
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["method"] = "tools/list";
        req["id"] = 12345;

        QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);
        QMap<QString, QString> hdr;
        hdr.insert(QStringLiteral("Content-Type"), QStringLiteral("application/json"));
        QByteArray raw
            = sendHttpRequest(QStringLiteral("POST"), QStringLiteral("/sse"), hdr, payload);
        HttpResponse resp = parseHttpResponse(raw);
        QCOMPARE(resp.statusCode, 204); // POST itself should be 204

        // Both SSE connections should receive the same JSON‑RPC response
        QByteArray json1 = readSseMessage(sse1);
        QByteArray json2 = readSseMessage(sse2);
        QCOMPARE(json1, json2); // identical payloads

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(json1, &err);
        QVERIFY2(
            err.error == QJsonParseError::NoError,
            qPrintable(QStringLiteral("SSE JSON parse error: %1").arg(err.errorString())));

        QJsonObject obj = doc.object();
        QVERIFY2(
            obj.value("jsonrpc").toString() == QLatin1String("2.0"),
            "SSE multiple clients: wrong jsonrpc version");
        QVERIFY2(obj.value("id").toInt() == 12345, "SSE multiple clients: wrong id");

        sse1->disconnectFromHost();
        sse2->disconnectFromHost();
        sse1->deleteLater();
        sse2->deleteLater();
    }

    void test_pluginVersion()
    {
        // Re‑use the TCP initialize request, but we only care about version string.
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["method"] = "initialize";
        req["id"] = 100;

        QJsonObject params;
        params["protocolVersion"] = "2024-11-05";
        params["capabilities"] = QJsonObject();
        QJsonObject clientInfo;
        clientInfo["name"] = "Version Test Client";
        clientInfo["version"] = "1.0.0";
        params["clientInfo"] = clientInfo;
        req["params"] = params;

        QString request = QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact));
        QString responseText = sendTcpRequest(request);
        QVERIFY2(!responseText.isEmpty(), "Empty response for version test");

        QJsonParseError err;
        QJsonDocument respDoc = QJsonDocument::fromJson(responseText.toUtf8(), &err);
        QVERIFY2(
            err.error == QJsonParseError::NoError,
            qPrintable("Version test JSON parse error: " + err.errorString()));

        QJsonObject respObj = respDoc.object();
        QJsonObject serverInfo = respObj.value("result").toObject().value("serverInfo").toObject();

        QString name = serverInfo.value("name").toString();
        QString version = serverInfo.value("version").toString();

        QVERIFY2(
            name.contains("Qt Creator MCP Server"), qPrintable("Unexpected server name: " + name));
        QVERIFY2(
            version.contains(QCoreApplication::applicationVersion()),
            qPrintable("Version does not contain expected substring: " + version));
    }
};

QObject *setupMCPServerTest()
{
    return new McpServerTest();
}

#include "mcpservertest.moc"
#endif
