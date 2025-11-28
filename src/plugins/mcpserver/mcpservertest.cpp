// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include <QCommandLineParser>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTest>
#include <QThread>

class MCPServerTest : public QObject
{
    Q_OBJECT

public:
    explicit MCPServerTest(QObject *parent = nullptr)
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

        // Use a QEventLopp since waitForReadyRead() would cause issues
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
            // Start (or restart) the timeout timer for each iteration.
            timeoutTimer.start();

            // Run the loop – it will return as soon as readyRead() (or any of the
            // other signals) fires.
            loop.exec();

            if (socket.bytesAvailable()) {
                response += socket.readAll();

                // Try to parse the accumulated bytes as JSON. If it succeeds we
                // have received the whole response and can break.
                QJsonParseError err;
                QJsonDocument::fromJson(response, &err);
                if (err.error == QJsonParseError::NoError)
                    break; // success – full JSON received
            }

            if (!timeoutTimer.isActive()) {
                // The timer has expired – we give up.
                throw std::runtime_error("TCP read timeout");
            }

            if (socket.state() != QAbstractSocket::ConnectedState) {
                throw std::runtime_error(QStringLiteral("TCP socket closed / error: %1")
                                             .arg(socket.errorString())
                                             .toStdString());
            }
        }

        socket.disconnectFromHost(); // clean shutdown
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
        socket.connectToHost(QHostAddress::LocalHost, 3002);
        if (!socket.waitForConnected(timeoutMs)) {
            throw std::runtime_error(
                QStringLiteral("HTTP connect failed: %1").arg(socket.errorString()).toStdString());
        }

        // Build the raw HTTP request (identical to the original script)
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
            // 1) Find the end of the header block.
            int headerEnd = buf.indexOf("\r\n\r\n");
            if (headerEnd == -1)
                return false; // still waiting for headers

            // 2) Look for a Content‑Length header.
            QByteArray headerPart = buf.left(headerEnd + 4);
            QRegularExpression
                re(QStringLiteral("Content-Length: (\\d+)"),
                   QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = re.match(QString::fromUtf8(headerPart));

            if (match.hasMatch()) {
                int contentLen = match.captured(1).toInt();
                int bodyStart = headerEnd + 4;
                int bodyNow = buf.size() - bodyStart;
                return bodyNow >= contentLen; // true when we have the whole body
            }

            // No Content‑Length → most likely a 204/304 or a response that ends
            // with the header block. Treat it as complete.
            return true;
        };

        while (true) {
            timeoutTimer.start();
            loop.exec(); // returns on readyRead / error / timeout

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

            // Otherwise loop again – timer has been restarted and we wait for the
            // next readyRead().
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

    void test_httpGetInfo()
    {
        QByteArray raw = sendHttpRequest(QStringLiteral("GET"), QStringLiteral("/"));
        HttpResponse resp = parseHttpResponse(raw);
        QCOMPARE(resp.statusCode, 200);
    }

    void test_httpInitialize()
    {
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

        QByteArray raw = sendHttpRequest(QStringLiteral("POST"), QStringLiteral("/"), hdr, payload);
        HttpResponse resp = parseHttpResponse(raw);
        QCOMPARE(resp.statusCode, 200);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(resp.body, &err);
        QVERIFY2(
            err.error == QJsonParseError::NoError,
            qPrintable("HTTP initialize JSON parse error: " + err.errorString()));
        assertJsonRpcInitialize(doc.object(), "HTTP Initialize");
    }

    void test_httpToolsList()
    {
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["method"] = "tools/list";
        req["id"] = 4;

        QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);
        QMap<QString, QString> hdr;
        hdr.insert(QStringLiteral("Content-Type"), QStringLiteral("application/json"));

        QByteArray raw = sendHttpRequest(QStringLiteral("POST"), QStringLiteral("/"), hdr, payload);
        HttpResponse resp = parseHttpResponse(raw);
        QCOMPARE(resp.statusCode, 200);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(resp.body, &err);
        QVERIFY2(
            err.error == QJsonParseError::NoError,
            qPrintable("HTTP tools/list JSON parse error: " + err.errorString()));

        QJsonObject obj = doc.object();
        QVERIFY2(
            obj.value("jsonrpc").toString() == QLatin1String("2.0"),
            "HTTP tools/list: wrong jsonrpc version");
        QJsonObject result = obj.value("result").toObject();
        QVERIFY2(result.contains("tools"), "HTTP tools/list: missing tools array");
        QJsonArray tools = result.value("tools").toArray();
        QVERIFY2(!tools.isEmpty(), "HTTP tools/list: tools array is empty");
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

        // ---- HTTP -------------------------------------------------
        QMap<QString, QString> hdr;
        hdr.insert(QStringLiteral("Content-Type"), QStringLiteral("application/json"));
        QByteArray rawHttp
            = sendHttpRequest(QStringLiteral("POST"), QStringLiteral("/"), hdr, payload);
        HttpResponse httpResp = parseHttpResponse(rawHttp);
        bool httpOk = false;
        if (httpResp.statusCode == 200) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(httpResp.body, &err);
            if (err.error == QJsonParseError::NoError) {
                QJsonObject obj = doc.object();
                httpOk = (obj.value("id").toInt() == testId);
            }
        }

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
    return new MCPServerTest();
}

#include "mcpservertest.moc"
#endif
