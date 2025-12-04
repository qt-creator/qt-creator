// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpserver.h"

#include <utils/threadutils.h>

#include <QCoreApplication>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

// Define logging category for MCP server
Q_LOGGING_CATEGORY(mcpServer, "qtc.mcpserver", QtWarningMsg)

namespace Mcp::Internal {

template<class Callable>
static auto runOnGuiThread(Callable &&c) -> std::invoke_result_t<Callable>
{
    using Ret = std::invoke_result_t<Callable>;

    // Decide which connection type to use.
    Qt::ConnectionType connectionType = Utils::isMainThread() ? Qt::DirectConnection
                                                              : Qt::BlockingQueuedConnection;
    if constexpr (std::is_void_v<Ret>) {
        // No return value – just invoke and wait.
        QMetaObject::invokeMethod(
            QCoreApplication::instance(), std::forward<Callable>(c), connectionType);
    } else {
        // Callable returns something – capture it in a local variable.
        Ret ret{};
        QMetaObject::invokeMethod(QCoreApplication::instance(), [&] { ret = c(); }, connectionType);
        return ret;
    }
}

static QHttpHeaders commonCorsHeaders()
{
    QHttpHeaders h;
    h.append(QHttpHeaders::WellKnownHeader::ContentType, QByteArrayLiteral("application/json"));
    h.append(QHttpHeaders::WellKnownHeader::AccessControlAllowOrigin, "*");
    h.append(QHttpHeaders::WellKnownHeader::AccessControlAllowMethods, "GET, POST, OPTIONS");
    h.append(QHttpHeaders::WellKnownHeader::AccessControlAllowHeaders, "Content-Type, Accept");
    return h;
}

static QHttpServerResponse createCorsJsonResponse(
    const QByteArray &json,
    QHttpServerResponder::StatusCode httpStatus = QHttpServerResponder::StatusCode::Ok)
{
    QHttpServerResponse response(json, httpStatus);
    response.setHeaders(commonCorsHeaders());
    return response;
}

static QHttpServerResponse createCorsEmptyResponse(QHttpServerResponder::StatusCode httpStatus)
{
    QHttpServerResponse response(httpStatus);
    response.setHeaders(commonCorsHeaders());
    return response;
}

McpServer::McpServer(QObject *parent)
    : QObject(parent)
    , m_tcpServerP(new QTcpServer(this))
    , m_httpServerP(new QHttpServer(this))
    , m_commandsP(new McpCommands(this))
    , m_port(3001)
{
    // Set up TCP server connections
    connect(m_tcpServerP, &QTcpServer::newConnection, this, &McpServer::handleNewConnection);

    m_httpServerP->route("/", QHttpServerRequest::Method::Get, this, &McpServer::onHttpGet);
    m_httpServerP->route("/", QHttpServerRequest::Method::Post, this, &McpServer::onHttpPost);
    m_httpServerP->route("/", QHttpServerRequest::Method::Options, this, &McpServer::onHttpOptions);

    // Optional generic 404 for any other path
    m_httpServerP
        ->setMissingHandler(this, [](const QHttpServerRequest &, QHttpServerResponder &resp) {
            resp.write(QHttpServerResponder::StatusCode::NotFound);
        });

    auto addMethod = [&](const QString &name, MethodHandler handler) {
        m_methods.insert(name, std::move(handler));
    };

    addMethod("initialize", [this](const QJsonObject &, const QJsonValue &id) {
        QJsonObject capabilities;
        QJsonObject toolsCapability;
        toolsCapability["listChanged"] = false;
        capabilities["tools"] = toolsCapability;

        QJsonObject serverInfo;
        serverInfo["name"] = "Qt Creator MCP Server";
        serverInfo["version"] = m_commandsP->getVersion();

        QJsonObject initResult;
        initResult["protocolVersion"] = "2024-11-05";
        initResult["capabilities"] = capabilities;
        initResult["serverInfo"] = serverInfo;

        return createSuccessResponse(initResult, id);
    });

    addMethod("tools/list", [this](const QJsonObject &, const QJsonValue &id) {
        return createSuccessResponse(QJsonObject{{"tools", m_toolList}}, id);
    });

    addMethod("tools/call", [this](const QJsonObject &params, const QJsonValue &id) {
        const QString toolName = params.value("name").toString();
        const QJsonObject arguments = params.value("arguments").toObject();

        auto it = m_toolHandlers.find(toolName);
        if (it == m_toolHandlers.end())
            return createErrorResponse(-32601, QStringLiteral("Unknown tool: %1").arg(toolName), id);

        QJsonObject rawResult = it.value()(arguments);

        // Build the CallToolResult payload as required by the spec.
        QJsonObject result;
        QJsonObject textBlock{
            {"type", "text"},
            {"text", QString::fromUtf8(QJsonDocument(rawResult).toJson(QJsonDocument::Compact))}};
        result.insert("content", QJsonArray{textBlock});
        result.insert("isError", false);

        return createSuccessResponse(result, id);
    });

    auto addTool = [&](const QJsonObject &tool, ToolHandler handler) {
        m_toolList.append(tool);
        const QString name = tool.value("name").toString();
        Q_ASSERT(!name.isEmpty());
        m_toolHandlers.insert(name, std::move(handler));
    };

    addTool(
        {{"name", "build"},
         {"title", "Build the current project"},
         {"description", "Compile the current Qt Creator project"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            bool ok = runOnGuiThread([this] { return m_commandsP->build(); });
            return QJsonObject{{"success", ok}};
        });

    addTool(
        {{"name", "get_build_status"},
         {"title", "Get current build status"},
         {"description", "Get current build progress and status"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"result", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"result"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            QString status = runOnGuiThread([this] { return m_commandsP->getBuildStatus(); });
            return QJsonObject{{"result", status}};
        });

    addTool(
        {{"name", "debug"},
         {"title", "Start debugging the current project"},
         {"description", "Start debugging the current project"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"result", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"result"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            QString result = runOnGuiThread([this] { return m_commandsP->debug(); });
            return QJsonObject{{"result", result}};
        });

    addTool(
        {{"name", "open_file"},
         {"title", "Open a file in Qt Creator"},
         {"description", "Open a file in Qt Creator"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"path",
                    QJsonObject{
                        {"type", "string"},
                        {"format", "uri"},
                        {"description", "Absolute path of the file to open"}}}}},
              {"required", QJsonArray{"path"}}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            bool ok = runOnGuiThread([this, path] { return m_commandsP->openFile(path); });
            return QJsonObject{{"success", ok}};
        });

    addTool(
        {{"name", "list_projects"},
         {"title", "List all available projects"},
         {"description", "List all available projects"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"projects",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"projects"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            const QStringList projects = runOnGuiThread(
                [this] { return m_commandsP->listProjects(); });
            QJsonArray arr;
            for (const QString &pr : projects)
                arr.append(pr);
            return QJsonObject{{"projects", arr}};
        });

    addTool(
        {{"name", "list_build_configs"},
         {"title", "List available build configurations"},
         {"description", "List available build configurations"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"buildConfigs",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"buildConfigs"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            const QStringList configs = runOnGuiThread(
                [this] { return m_commandsP->listBuildConfigs(); });
            QJsonArray arr;
            for (const QString &c : configs)
                arr.append(c);
            return QJsonObject{{"buildConfigs", arr}};
        });

    addTool(
        {{"name", "switch_build_config"},
         {"title", "Switch to a specific build configuration"},
         {"description", "Switch to a specific build configuration"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"name",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Name of the build configuration to switch to"}}}}},
              {"required", QJsonArray{"name"}}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            const QString name = p.value("name").toString();
            bool ok = runOnGuiThread(
                [this, name] { return m_commandsP->switchToBuildConfig(name); });
            return QJsonObject{{"success", ok}};
        });

    addTool(
        {{"name", "run_project"},
         {"title", "Run the current project"},
         {"description", "Run the current project"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            bool ok = runOnGuiThread([this] { return m_commandsP->runProject(); });
            return QJsonObject{{"success", ok}};
        });

    addTool(
        {{"name", "clean_project"},
         {"title", "Clean the current project"},
         {"description", "Clean the current project"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            bool ok = runOnGuiThread([this] { return m_commandsP->cleanProject(); });
            return QJsonObject{{"success", ok}};
        });

    addTool(
        {{"name", "list_open_files"},
         {"title", "List currently open files"},
         {"description", "List currently open files"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"openFiles",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"openFiles"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            const QStringList files = runOnGuiThread(
                [this] { return m_commandsP->listOpenFiles(); });
            QJsonArray arr;
            for (const QString &f : files)
                arr.append(f);
            return QJsonObject{{"openFiles", arr}};
        });

    addTool(
        {{"name", "list_sessions"},
         {"title", "List available sessions"},
         {"description", "List available sessions"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"sessions",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"sessions"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            const QStringList sessions = runOnGuiThread(
                [this] { return m_commandsP->listSessions(); });
            QJsonArray arr;
            for (const QString &s : sessions)
                arr.append(s);
            return QJsonObject{{"sessions", arr}};
        });

    addTool(
        {{"name", "load_session"},
         {"title", "Load a specific session"},
         {"description", "Load a specific session"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"sessionName",
                    QJsonObject{{"type", "string"}, {"description", "Name of the session to load"}}}}},
              {"required", QJsonArray{"sessionName"}}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            const QString name = p.value("sessionName").toString();
            bool ok = runOnGuiThread([this, name] { return m_commandsP->loadSession(name); });
            return QJsonObject{{"success", ok}};
        });

    addTool(
        {{"name", "list_issues"},
         {"title", "List current issues (warnings and errors)"},
         {"description", "List current issues (warnings and errors)"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"issues",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"issues"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            const QStringList issues = runOnGuiThread([this] { return m_commandsP->listIssues(); });
            QJsonArray arr;
            for (const QString &i : issues)
                arr.append(i);
            return QJsonObject{{"issues", arr}};
        });

    addTool(
        {{"name", "quit"},
         {"title", "Quit Qt Creator"},
         {"description", "Quit Qt Creator"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            bool ok = runOnGuiThread([this] { return m_commandsP->quit(); });
            return QJsonObject{{"success", ok}};
        });

    addTool(
        {{"name", "get_current_project"},
         {"title", "Get the currently active project"},
         {"description", "Get the currently active project"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"project", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"project"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            QString proj = runOnGuiThread([this] { return m_commandsP->getCurrentProject(); });
            return QJsonObject{{"project", proj}};
        });

    addTool(
        {{"name", "get_current_build_config"},
         {"title", "Get the currently active build configuration"},
         {"description", "Get the currently active build configuration"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"buildConfig", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"buildConfig"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            QString cfg = runOnGuiThread([this] { return m_commandsP->getCurrentBuildConfig(); });
            return QJsonObject{{"buildConfig", cfg}};
        });

    addTool(
        {{"name", "get_current_session"},
         {"title", "Get the currently active session"},
         {"description", "Get the currently active session"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"session", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"session"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            QString sess = runOnGuiThread([this] { return m_commandsP->getCurrentSession(); });
            return QJsonObject{{"session", sess}};
        });

    addTool(
        {{"name", "save_session"},
         {"title", "Save the current session"},
         {"description", "Save the current session"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p) {
            Q_UNUSED(p);
            bool ok = runOnGuiThread([this] { return m_commandsP->saveSession(); });
            return QJsonObject{{"success", ok}};
        });
}

QHttpServerResponse McpServer::onHttpGet(const QHttpServerRequest &req)
{
    Q_UNUSED(req);
    // Simple GET request - return server info
    QJsonObject serverInfo;
    serverInfo["name"] = "Qt Creator MCP Server";
    serverInfo["version"] = "1.0.0";
    serverInfo["transport"] = "HTTP";
    serverInfo["protocol"] = "MCP";

    QJsonDocument doc(serverInfo);
    return createCorsJsonResponse(doc.toJson());
}

QHttpServerResponse McpServer::onHttpPost(const QHttpServerRequest &req)
{
    const QByteArrayView ct = req.headers().value("Content-Type");
    if (!ct.startsWith("application/json")) {
        return createErrorResponse(
            int(QHttpServerResponder::StatusCode::BadRequest),
            "Content-Type must be application/json");
    }

    // Handle MCP JSON-RPC requests
    const QByteArray payload = req.body();
    if (payload.isEmpty())
        return createErrorResponse(-32700, "Empty request body");

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError)
        return createErrorResponse(-32600, QStringLiteral("Invalid JSON: %1").arg(err.errorString()));

    if (!doc.isObject())
        return createErrorResponse(-32600, "Invalid Request");

    const QJsonObject obj = doc.object();
    const QString method = obj.value("method").toString();
    const QJsonValue id = obj.value("id");
    const QJsonObject params = obj.value("params").toObject();

    QString jsonrpc = obj.value("jsonrpc").toString();
    if (jsonrpc != "2.0")
        return createErrorResponse(-32600, "Invalid Request: jsonrpc must be '2.0'", id);

    const QJsonObject reply = callMCPMethod(method, params, id);
    return createCorsJsonResponse(QJsonDocument(reply).toJson());
}

QHttpServerResponse McpServer::onHttpOptions(const QHttpServerRequest &)
{
    // No body, just CORS headers and a 204 status
    return createCorsEmptyResponse(QHttpServerResponder::StatusCode::NoContent);
}

McpServer::~McpServer()
{
    stop();
    delete m_commandsP;
}

bool McpServer::start(quint16 port)
{
    m_port = port;
    qCDebug(mcpServer) << "Starting MCP HTTP server on port" << m_port;
    qCDebug(mcpServer) << "Qt version:" << QT_VERSION_STR;
    qCDebug(mcpServer) << "Build type:" <<
#ifdef QT_NO_DEBUG
        "Release"
#else
        "Debug"
#endif
        ;

    // Try to start TCP server on the requested port first
    if (!m_tcpServerP->listen(QHostAddress::LocalHost, m_port)) {
        qCCritical(mcpServer) << "Failed to start MCP TCP server on port" << m_port << ":"
                              << m_tcpServerP->errorString();
        qCWarning(mcpServer) << "Port" << m_port
                             << "is in use, trying to find an available port...";

        // Try ports from 3001 to 3010
        for (quint16 tryPort = 3001; tryPort <= 3010; ++tryPort) {
            if (m_tcpServerP->listen(QHostAddress::LocalHost, tryPort)) {
                m_port = tryPort;
                qCInfo(mcpServer) << "MCP TCP Server started on port" << m_port << "(port" << port
                                  << "was busy)";
                break;
            }
        }

        if (!m_tcpServerP->isListening()) {
            qCCritical(mcpServer) << "Failed to start MCP TCP server on any port from 3001-3010";
            return false;
        }
    }

    quint16 httpPort = m_port; // start with the same number
    bool httpBound = false;

    while (!httpBound && httpPort <= 3010) {
        // Create a brand‑new QTcpServer that will be owned by the QHttpServer
        // after a successful bind().
        QTcpServer *httpTcp = new QTcpServer(this);
        if (!httpTcp->listen(QHostAddress::LocalHost, httpPort)) {
            delete httpTcp;
            ++httpPort;
            continue; // try next port
        }

        // Bind the listening socket to the QHttpServer.
        // If bind() fails we destroy the socket and try the next port.
        if (m_httpServerP->bind(httpTcp)) {
            httpBound = true;
            m_httpTcpServerP = httpTcp;
            qCInfo(mcpServer) << "HTTP server bound to port" << httpPort;
        } else {
            delete httpTcp;
            ++httpPort;
        }
    }

    if (!httpBound) {
        qCCritical(mcpServer) << "Failed to bind HTTP listener on any port 3001‑3010";
        // Clean up the raw‑TCP listener we already opened
        m_tcpServerP->close();
        return false;
    }

    qCDebug(mcpServer) << "MCP server startup complete - TCP listening:"
                       << m_tcpServerP->isListening();
    qCDebug(mcpServer) << "MCP server startup complete - HTTP listening:"
                       << m_httpTcpServerP->isListening();

    qCInfo(mcpServer) << "MCP TCP Server started successfully on port"
                      << m_tcpServerP->serverPort();
    qCInfo(mcpServer) << "MCP HTTP Server started successfully on port"
                      << m_httpTcpServerP->serverPort();
    return true;
}

void McpServer::stop()
{
    if (m_tcpServerP && m_tcpServerP->isListening()) {
        m_tcpServerP->close();
        qCDebug(mcpServer) << "MCP TCP Server stopped";
    }
    if (m_httpTcpServerP && m_httpTcpServerP->isListening()) {
        m_httpTcpServerP->close();
        qCDebug(mcpServer) << "MCP HTTP Server stopped";
    }
}

bool McpServer::isRunning() const
{
    return (m_tcpServerP && m_tcpServerP->isListening())
           || (m_httpTcpServerP && m_httpTcpServerP->isListening());
}

quint16 McpServer::getPort() const
{
    return m_port;
}

QJsonObject McpServer::callMCPMethod(
    const QString &method, const QJsonObject &params, const QJsonValue &id)
{
    if (method.isEmpty())
        return createErrorResponse(-32600, "Invalid Request: method is required", id);

    // Find the handler
    auto it = m_methods.find(method);
    QJsonObject reply;
    if (it == m_methods.end())
        reply = createErrorResponse(-32601, QStringLiteral("Method not found: %1").arg(method), id);
    else {
        try {
            reply = it.value()(params, id);
        } catch (const std::exception &e) {
            reply = createErrorResponse(-32603, QString::fromStdString(e.what()), id);
        }
    }

    return reply;
}

QJsonObject McpServer::createErrorResponse(int code, const QString &message, const QJsonValue &id)
{
    QJsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;

    QJsonObject error;
    error["code"] = code;
    error["message"] = message;
    response["error"] = error;

    return response;
}

QJsonObject McpServer::createSuccessResponse(const QJsonValue &result, const QJsonValue &id)
{
    QJsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;
    response["result"] = result;

    return response;
}

void McpServer::handleNewConnection()
{
    QTcpSocket *client = m_tcpServerP->nextPendingConnection();
    if (!client)
        return;

    m_clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &McpServer::handleClientData);
    connect(client, &QTcpSocket::disconnected, this, &McpServer::handleClientDisconnected);

    qCDebug(mcpServer) << "New TCP client connected, total clients:" << m_clients.size();
}

void McpServer::handleClientData()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client)
        return;

    QByteArray data = client->readAll();
    qCDebug(mcpServer) << "Received data, size:" << data.size();

    // Handle as TCP/JSON-RPC request (original behavior)
    qCDebug(mcpServer) << "Handling as TCP/JSON-RPC request";

    // Parse JSON-RPC request
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qCDebug(mcpServer) << "JSON parse error:" << error.errorString();
        QJsonObject errorResponse = createErrorResponse(-32700, "Parse error");
        sendResponse(client, errorResponse);
        return;
    }

    if (!doc.isObject()) {
        qCDebug(mcpServer) << "Invalid JSON-RPC message: not an object";
        QJsonObject errorResponse = createErrorResponse(-32600, "Invalid Request");
        sendResponse(client, errorResponse);
        return;
    }

    const QJsonObject obj = doc.object();

    // The JSON‑RPC 2.0 request
    const QString method = obj.value("method").toString();
    const QJsonValue id = obj.value("id");
    const QJsonObject params = obj.value("params").toObject();

    const QString jsonrpc = obj.value("jsonrpc").toString();
    if (jsonrpc != "2.0") {
        qCDebug(mcpServer) << "Invalid JSON-RPC version";
        QJsonObject errorResponse
            = createErrorResponse(-32600, "Invalid Request: jsonrpc must be '2.0'", id);
        sendResponse(client, errorResponse);
        return;
    }

    const QJsonObject response = callMCPMethod(method, params, id);
    sendResponse(client, response);
}

void McpServer::handleClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client)
        return;

    m_clients.removeAll(client);
    client->deleteLater();

    qCDebug(mcpServer) << "TCP client disconnected, remaining clients:" << m_clients.size();
}

void McpServer::sendResponse(QTcpSocket *client, const QJsonObject &response)
{
    if (!client)
        return;

    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    qCDebug(mcpServer) << "Sending TCP response:" << data;
    client->write(data);
    client->flush();
}

} // namespace Mcp::Internal
