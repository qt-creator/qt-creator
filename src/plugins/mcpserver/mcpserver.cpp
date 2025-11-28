// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpserver.h"

#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QLoggingCategory>
#include <QTcpServer>
#include <QTcpSocket>

// Define logging category for MCP server
Q_LOGGING_CATEGORY(mcpServer, "qtc.mcpserver", QtWarningMsg)

namespace MCP {
namespace Internal {

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

MCPServer::MCPServer(QObject *parent)
    : QObject(parent)
    , m_tcpServerP(new QTcpServer(this))
    , m_httpServerP(new QHttpServer(this))
    , m_commandsP(new MCPCommands(this))
    , m_port(3001)
{
    // Set up TCP server connections
    connect(m_tcpServerP, &QTcpServer::newConnection, this, &MCPServer::handleNewConnection);

    m_httpServerP->route("/", QHttpServerRequest::Method::Get, this, &MCPServer::onHttpGet);
    m_httpServerP->route("/", QHttpServerRequest::Method::Post, this, &MCPServer::onHttpPost);
    m_httpServerP->route("/", QHttpServerRequest::Method::Options, this, &MCPServer::onHttpOptions);

    // Optional generic 404 for any other path
    m_httpServerP
        ->setMissingHandler(this, [](const QHttpServerRequest &, QHttpServerResponder &resp) {
            resp.write(QHttpServerResponder::StatusCode::NotFound);
        });
}

QHttpServerResponse MCPServer::onHttpGet(const QHttpServerRequest &req)
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

QHttpServerResponse MCPServer::onHttpPost(const QHttpServerRequest &req)
{
    const QByteArrayView ct = req.headers().value("Content-Type");
    if (!ct.startsWith("application/json")) {
        return createErrorResponse(
            int(QHttpServerResponder::StatusCode::BadRequest),
            "Content-Type must be application/json");
    }

    // Handle MCP JSON-RPC requests
    const QByteArray payload = req.body();
    if (payload.isEmpty()) {
        return createErrorResponse(
            int(QHttpServerResponder::StatusCode::BadRequest), "Empty request body");
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError) {
        return createErrorResponse(
            int(QHttpServerResponder::StatusCode::BadRequest),
            QStringLiteral("Invalid JSON: %1").arg(err.errorString()));
    }
    if (!doc.isObject()) {
        return createErrorResponse(
            int(QHttpServerResponder::StatusCode::BadRequest), "Invalid JSON‑RPC: not an object");
    }

    QJsonObject reply = processRequest(doc.object());

    QJsonDocument replyDoc(reply);
    return createCorsJsonResponse(replyDoc.toJson());
}

QHttpServerResponse MCPServer::onHttpOptions(const QHttpServerRequest &)
{
    // No body, just CORS headers and a 204 status
    return createCorsEmptyResponse(QHttpServerResponder::StatusCode::NoContent);
}

MCPServer::~MCPServer()
{
    stop();
    delete m_commandsP;
}

bool MCPServer::start(quint16 port)
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

void MCPServer::stop()
{
    if (m_tcpServerP && m_tcpServerP->isListening()) {
        m_tcpServerP->close();
        qCDebug(mcpServer) << "MCP TCP Server stopped";
    }
    if (m_httpTcpServerP && m_httpTcpServerP->isListening()) {
        m_tcpServerP->close();
        qCDebug(mcpServer) << "MCP HTTP Server stopped";
    }
}

bool MCPServer::isRunning() const
{
    return (m_tcpServerP && m_tcpServerP->isListening())
           || (m_httpTcpServerP && m_httpTcpServerP->isListening());
}

quint16 MCPServer::getPort() const
{
    return m_port;
}

QJsonObject MCPServer::processRequest(const QJsonObject &request)
{
    // Extract method and parameters
    QString method = request.value("method").toString();
    QJsonValue params = request.value("params");
    QJsonValue id = request.value("id");

    // Validate JSON-RPC version
    QString jsonrpc = request.value("jsonrpc").toString();
    if (jsonrpc != "2.0") {
        return createErrorResponse(-32600, "Invalid Request: jsonrpc must be '2.0'", id);
    }

    if (method.isEmpty()) {
        return createErrorResponse(-32600, "Invalid Request: method is required", id);
    }

    qCDebug(mcpServer) << "Processing MCP request:" << method << "with id:" << id;

    QJsonValue result;
    QString errorMessage;

    // Handle standard MCP protocol methods only
    if (method == "initialize") {
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
        result = initResult;
    } else if (method == "tools/list") {
        QJsonArray tools;

        // Build tool
        QJsonObject buildTool;
        buildTool["name"] = "build";
        buildTool["description"] = "Build the current Qt Creator project";
        QJsonObject buildInputSchema;
        buildInputSchema["type"] = "object";
        buildInputSchema["properties"] = QJsonObject();
        buildTool["inputSchema"] = buildInputSchema;
        tools.append(buildTool);

        // Get build status tool
        QJsonObject getBuildStatusTool;
        getBuildStatusTool["name"] = "getBuildStatus";
        getBuildStatusTool["description"] = "Get current build progress and status";
        QJsonObject getBuildStatusInputSchema;
        getBuildStatusInputSchema["type"] = "object";
        getBuildStatusInputSchema["properties"] = QJsonObject();
        getBuildStatusTool["inputSchema"] = getBuildStatusInputSchema;
        tools.append(getBuildStatusTool);

        // Debug tool
        QJsonObject debugTool;
        debugTool["name"] = "debug";
        debugTool["description"] = "Start debugging the current project";
        QJsonObject debugInputSchema;
        debugInputSchema["type"] = "object";
        debugInputSchema["properties"] = QJsonObject();
        debugTool["inputSchema"] = debugInputSchema;
        tools.append(debugTool);

        // Open file tool
        QJsonObject openFileTool;
        openFileTool["name"] = "openFile";
        openFileTool["description"] = "Open a file in Qt Creator";
        QJsonObject openFileInputSchema;
        openFileInputSchema["type"] = "object";
        QJsonObject openFileProperties;
        openFileProperties["path"]
            = QJsonObject{{"type", "string"}, {"description", "Path to the file to open"}};
        openFileInputSchema["properties"] = openFileProperties;
        openFileInputSchema["required"] = QJsonArray{"path"};
        openFileTool["inputSchema"] = openFileInputSchema;
        tools.append(openFileTool);

        // List projects tool
        QJsonObject listProjectsTool;
        listProjectsTool["name"] = "listProjects";
        listProjectsTool["description"] = "List all available projects";
        QJsonObject listProjectsInputSchema;
        listProjectsInputSchema["type"] = "object";
        listProjectsInputSchema["properties"] = QJsonObject();
        listProjectsTool["inputSchema"] = listProjectsInputSchema;
        tools.append(listProjectsTool);

        // List build configs tool
        QJsonObject listBuildConfigsTool;
        listBuildConfigsTool["name"] = "listBuildConfigs";
        listBuildConfigsTool["description"] = "List available build configurations";
        QJsonObject listBuildConfigsInputSchema;
        listBuildConfigsInputSchema["type"] = "object";
        listBuildConfigsInputSchema["properties"] = QJsonObject();
        listBuildConfigsTool["inputSchema"] = listBuildConfigsInputSchema;
        tools.append(listBuildConfigsTool);

        // Switch build config tool
        QJsonObject switchBuildConfigTool;
        switchBuildConfigTool["name"] = "switchBuildConfig";
        switchBuildConfigTool["description"] = "Switch to a specific build configuration";
        QJsonObject switchBuildConfigInputSchema;
        switchBuildConfigInputSchema["type"] = "object";
        QJsonObject switchBuildConfigProperties;
        switchBuildConfigProperties["name"] = QJsonObject{
            {"type", "string"}, {"description", "Name of the build configuration to switch to"}};
        switchBuildConfigInputSchema["properties"] = switchBuildConfigProperties;
        switchBuildConfigInputSchema["required"] = QJsonArray{"name"};
        switchBuildConfigTool["inputSchema"] = switchBuildConfigInputSchema;
        tools.append(switchBuildConfigTool);

        // Run project tool
        QJsonObject runProjectTool;
        runProjectTool["name"] = "runProject";
        runProjectTool["description"] = "Run the current project";
        QJsonObject runProjectInputSchema;
        runProjectInputSchema["type"] = "object";
        runProjectInputSchema["properties"] = QJsonObject();
        runProjectTool["inputSchema"] = runProjectInputSchema;
        tools.append(runProjectTool);

        // Clean project tool
        QJsonObject cleanProjectTool;
        cleanProjectTool["name"] = "cleanProject";
        cleanProjectTool["description"] = "Clean the current project";
        QJsonObject cleanProjectInputSchema;
        cleanProjectInputSchema["type"] = "object";
        cleanProjectInputSchema["properties"] = QJsonObject();
        cleanProjectTool["inputSchema"] = cleanProjectInputSchema;
        tools.append(cleanProjectTool);

        // List open files tool
        QJsonObject listOpenFilesTool;
        listOpenFilesTool["name"] = "listOpenFiles";
        listOpenFilesTool["description"] = "List currently open files";
        QJsonObject listOpenFilesInputSchema;
        listOpenFilesInputSchema["type"] = "object";
        listOpenFilesInputSchema["properties"] = QJsonObject();
        listOpenFilesTool["inputSchema"] = listOpenFilesInputSchema;
        tools.append(listOpenFilesTool);

        // List sessions tool
        QJsonObject listSessionsTool;
        listSessionsTool["name"] = "listSessions";
        listSessionsTool["description"] = "List available sessions";
        QJsonObject listSessionsInputSchema;
        listSessionsInputSchema["type"] = "object";
        listSessionsInputSchema["properties"] = QJsonObject();
        listSessionsTool["inputSchema"] = listSessionsInputSchema;
        tools.append(listSessionsTool);

        // Load session tool
        QJsonObject loadSessionTool;
        loadSessionTool["name"] = "loadSession";
        loadSessionTool["description"] = "Load a specific session";
        QJsonObject loadSessionInputSchema;
        loadSessionInputSchema["type"] = "object";
        QJsonObject loadSessionProperties;
        loadSessionProperties["sessionName"]
            = QJsonObject{{"type", "string"}, {"description", "Name of the session to load"}};
        loadSessionInputSchema["properties"] = loadSessionProperties;
        loadSessionInputSchema["required"] = QJsonArray{"sessionName"};
        loadSessionTool["inputSchema"] = loadSessionInputSchema;
        tools.append(loadSessionTool);

        // List issues tool
        QJsonObject listIssuesTool;
        listIssuesTool["name"] = "listIssues";
        listIssuesTool["description"] = "List current issues (warnings and errors)";
        QJsonObject listIssuesInputSchema;
        listIssuesInputSchema["type"] = "object";
        listIssuesInputSchema["properties"] = QJsonObject();
        listIssuesTool["inputSchema"] = listIssuesInputSchema;
        tools.append(listIssuesTool);

        // Quit tool
        QJsonObject quitTool;
        quitTool["name"] = "quit";
        quitTool["description"] = "Quit Qt Creator";
        QJsonObject quitInputSchema;
        quitInputSchema["type"] = "object";
        quitInputSchema["properties"] = QJsonObject();
        quitTool["inputSchema"] = quitInputSchema;
        tools.append(quitTool);

        // Get current project tool
        QJsonObject getCurrentProjectTool;
        getCurrentProjectTool["name"] = "getCurrentProject";
        getCurrentProjectTool["description"] = "Get the currently active project";
        QJsonObject getCurrentProjectInputSchema;
        getCurrentProjectInputSchema["type"] = "object";
        getCurrentProjectInputSchema["properties"] = QJsonObject();
        getCurrentProjectTool["inputSchema"] = getCurrentProjectInputSchema;
        tools.append(getCurrentProjectTool);

        // Get current build config tool
        QJsonObject getCurrentBuildConfigTool;
        getCurrentBuildConfigTool["name"] = "getCurrentBuildConfig";
        getCurrentBuildConfigTool["description"] = "Get the currently active build configuration";
        QJsonObject getCurrentBuildConfigInputSchema;
        getCurrentBuildConfigInputSchema["type"] = "object";
        getCurrentBuildConfigInputSchema["properties"] = QJsonObject();
        getCurrentBuildConfigTool["inputSchema"] = getCurrentBuildConfigInputSchema;
        tools.append(getCurrentBuildConfigTool);

        // Get current session tool
        QJsonObject getCurrentSessionTool;
        getCurrentSessionTool["name"] = "getCurrentSession";
        getCurrentSessionTool["description"] = "Get the currently active session";
        QJsonObject getCurrentSessionInputSchema;
        getCurrentSessionInputSchema["type"] = "object";
        getCurrentSessionInputSchema["properties"] = QJsonObject();
        getCurrentSessionTool["inputSchema"] = getCurrentSessionInputSchema;
        tools.append(getCurrentSessionTool);

        // Save session tool
        QJsonObject saveSessionTool;
        saveSessionTool["name"] = "saveSession";
        saveSessionTool["description"] = "Save the current session";
        QJsonObject saveSessionInputSchema;
        saveSessionInputSchema["type"] = "object";
        saveSessionInputSchema["properties"] = QJsonObject();
        saveSessionTool["inputSchema"] = saveSessionInputSchema;
        tools.append(saveSessionTool);

        QJsonObject toolsResult;
        toolsResult["tools"] = tools;
        result = toolsResult;
    } else if (method == "tools/call") {
        if (!params.isObject()) {
            errorMessage = "Invalid parameters for tools/call";
        } else {
            QJsonObject paramsObj = params.toObject();
            QString toolName = paramsObj.value("name").toString();
            QJsonValue arguments = paramsObj.value("arguments");

            // Route to appropriate command based on tool name
            if (toolName == "build") {
                bool successB = m_commandsP->build();
                result = QJsonObject{{"success", successB}};
            } else if (toolName == "getBuildStatus") {
                QString buildStatusResult = m_commandsP->getBuildStatus();
                result = QJsonObject{{"result", buildStatusResult}};
            } else if (toolName == "debug") {
                QString debugResult = m_commandsP->debug();
                result = QJsonObject{{"result", debugResult}};
            } else if (toolName == "openFile") {
                if (arguments.isObject()) {
                    QString path = arguments.toObject().value("path").toString();
                    bool successB = m_commandsP->openFile(path);
                    result = QJsonObject{{"success", successB}};
                } else {
                    errorMessage = "Invalid arguments for openFile";
                }
            } else if (toolName == "listProjects") {
                QStringList projects = m_commandsP->listProjects();
                QJsonArray projectArray;
                for (const QString &project : projects) {
                    projectArray.append(project);
                }
                result = QJsonObject{{"projects", projectArray}};
            } else if (toolName == "listBuildConfigs") {
                QStringList configs = m_commandsP->listBuildConfigs();
                QJsonArray configArray;
                for (const QString &config : configs) {
                    configArray.append(config);
                }
                result = QJsonObject{{"buildConfigs", configArray}};
            } else if (toolName == "switchBuildConfig") {
                if (arguments.isObject()) {
                    QString name = arguments.toObject().value("name").toString();
                    bool successB = m_commandsP->switchToBuildConfig(name);
                    result = QJsonObject{{"success", successB}};
                } else {
                    errorMessage = "Invalid arguments for switchBuildConfig";
                }
            } else if (toolName == "runProject") {
                bool successB = m_commandsP->runProject();
                result = QJsonObject{{"success", successB}};
            } else if (toolName == "cleanProject") {
                bool successB = m_commandsP->cleanProject();
                result = QJsonObject{{"success", successB}};
            } else if (toolName == "listOpenFiles") {
                QStringList files = m_commandsP->listOpenFiles();
                QJsonArray fileArray;
                for (const QString &file : files) {
                    fileArray.append(file);
                }
                result = QJsonObject{{"openFiles", fileArray}};
            } else if (toolName == "listSessions") {
                QStringList sessions = m_commandsP->listSessions();
                QJsonArray sessionArray;
                for (const QString &session : sessions) {
                    sessionArray.append(session);
                }
                result = QJsonObject{{"sessions", sessionArray}};
            } else if (toolName == "loadSession") {
                if (arguments.isObject()) {
                    QString sessionName = arguments.toObject().value("sessionName").toString();
                    bool successB = m_commandsP->loadSession(sessionName);
                    result = QJsonObject{{"success", successB}};
                } else {
                    errorMessage = "Invalid arguments for loadSession";
                }
            } else if (toolName == "listIssues") {
                QStringList issues = m_commandsP->listIssues();
                QJsonArray issueArray;
                for (const QString &issue : issues) {
                    issueArray.append(issue);
                }
                result = QJsonObject{{"issues", issueArray}};
            } else if (toolName == "quit") {
                bool successB = m_commandsP->quit();
                result = QJsonObject{{"success", successB}};
            } else if (toolName == "getCurrentProject") {
                QString project = m_commandsP->getCurrentProject();
                result = QJsonObject{{"project", project}};
            } else if (toolName == "getCurrentBuildConfig") {
                QString config = m_commandsP->getCurrentBuildConfig();
                result = QJsonObject{{"buildConfig", config}};
            } else if (toolName == "getCurrentSession") {
                QString session = m_commandsP->getCurrentSession();
                result = QJsonObject{{"session", session}};
            } else if (toolName == "saveSession") {
                bool successB = m_commandsP->saveSession();
                result = QJsonObject{{"success", successB}};
            } else {
                // Provide helpful suggestions for common typos
                QString suggestion = "";
                if (toolName == "setBuildConfiguration") {
                    suggestion = " (did you mean 'switchBuildConfig'?)";
                } else if (toolName == "setBuildConfig") {
                    suggestion = " (did you mean 'switchBuildConfig'?)";
                } else if (toolName == "switchBuildConfiguration") {
                    suggestion = " (did you mean 'switchBuildConfig'?)";
                } else if (toolName == "getVersion") {
                    suggestion = " (use 'tools/list' to see available tools)";
                }
                errorMessage = "Unknown tool: " + toolName + suggestion;
            }
        }
    } else {
        errorMessage = QString("Unknown method: %1").arg(method);
    }

    if (!errorMessage.isEmpty()) {
        return createErrorResponse(-32601, errorMessage, id);
    } else {
        return createSuccessResponse(result, id);
    }
}

QJsonObject MCPServer::createErrorResponse(int code, const QString &message, const QJsonValue &id)
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

QJsonObject MCPServer::createSuccessResponse(const QJsonValue &result, const QJsonValue &id)
{
    QJsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;
    response["result"] = result;

    return response;
}

QJsonObject MCPServer::callMCPMethod(const QString &method, const QJsonValue &params)
{
    // Create a fake request object to reuse the existing processRequest logic
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["method"] = method;
    request["id"] = 1;

    if (!params.isUndefined()) {
        request["params"] = params;
    }

    // Create a fake response object to capture the result
    QJsonObject response;
    response["id"] = 1;

    // Process the request (this will populate the response)
    QString errorMessage;
    QJsonValue result;

    // Handle standard MCP protocol methods
    if (method == "initialize") {
        QJsonObject capabilities;
        QJsonObject toolsCapability;
        toolsCapability["listChanged"] = false;
        capabilities["tools"] = toolsCapability;

        QJsonObject serverInfo;
        serverInfo["name"] = "Qt MCP Plugin";
        serverInfo["version"] = m_commandsP->getVersion();

        QJsonObject initResult;
        initResult["protocolVersion"] = "2024-11-05";
        initResult["capabilities"] = capabilities;
        initResult["serverInfo"] = serverInfo;
        result = initResult;
    } else if (method == "tools/list") {
        QJsonArray tools;

        // Build tool
        QJsonObject buildTool;
        buildTool["name"] = "build";
        buildTool["description"] = "Build the current Qt Creator project";
        QJsonObject buildInputSchema;
        buildInputSchema["type"] = "object";
        buildInputSchema["properties"] = QJsonObject();
        buildTool["inputSchema"] = buildInputSchema;
        tools.append(buildTool);

        // Get build status tool
        QJsonObject getBuildStatusTool;
        getBuildStatusTool["name"] = "getBuildStatus";
        getBuildStatusTool["description"] = "Get current build progress and status";
        QJsonObject getBuildStatusInputSchema;
        getBuildStatusInputSchema["type"] = "object";
        getBuildStatusInputSchema["properties"] = QJsonObject();
        getBuildStatusTool["inputSchema"] = getBuildStatusInputSchema;
        tools.append(getBuildStatusTool);

        // Debug tool
        QJsonObject debugTool;
        debugTool["name"] = "debug";
        debugTool["description"] = "Start debugging the current project";
        QJsonObject debugInputSchema;
        debugInputSchema["type"] = "object";
        debugInputSchema["properties"] = QJsonObject();
        debugTool["inputSchema"] = debugInputSchema;
        tools.append(debugTool);

        // Open file tool
        QJsonObject openFileTool;
        openFileTool["name"] = "openFile";
        openFileTool["description"] = "Open a file in Qt Creator";
        QJsonObject openFileInputSchema;
        openFileInputSchema["type"] = "object";
        QJsonObject openFileProperties;
        openFileProperties["path"]
            = QJsonObject{{"type", "string"}, {"description", "Path to the file to open"}};
        openFileInputSchema["properties"] = openFileProperties;
        openFileInputSchema["required"] = QJsonArray{"path"};
        openFileTool["inputSchema"] = openFileInputSchema;
        tools.append(openFileTool);

        // List projects tool
        QJsonObject listProjectsTool;
        listProjectsTool["name"] = "listProjects";
        listProjectsTool["description"] = "List all available projects";
        QJsonObject listProjectsInputSchema;
        listProjectsInputSchema["type"] = "object";
        listProjectsInputSchema["properties"] = QJsonObject();
        listProjectsTool["inputSchema"] = listProjectsInputSchema;
        tools.append(listProjectsTool);

        // List build configs tool
        QJsonObject listBuildConfigsTool;
        listBuildConfigsTool["name"] = "listBuildConfigs";
        listBuildConfigsTool["description"] = "List available build configurations";
        QJsonObject listBuildConfigsInputSchema;
        listBuildConfigsInputSchema["type"] = "object";
        listBuildConfigsInputSchema["properties"] = QJsonObject();
        listBuildConfigsTool["inputSchema"] = listBuildConfigsInputSchema;
        tools.append(listBuildConfigsTool);

        // Switch build config tool
        QJsonObject switchBuildConfigTool;
        switchBuildConfigTool["name"] = "switchBuildConfig";
        switchBuildConfigTool["description"] = "Switch to a specific build configuration";
        QJsonObject switchBuildConfigInputSchema;
        switchBuildConfigInputSchema["type"] = "object";
        QJsonObject switchBuildConfigProperties;
        switchBuildConfigProperties["name"] = QJsonObject{
            {"type", "string"}, {"description", "Name of the build configuration to switch to"}};
        switchBuildConfigInputSchema["properties"] = switchBuildConfigProperties;
        switchBuildConfigInputSchema["required"] = QJsonArray{"name"};
        switchBuildConfigTool["inputSchema"] = switchBuildConfigInputSchema;
        tools.append(switchBuildConfigTool);

        // Run project tool
        QJsonObject runProjectTool;
        runProjectTool["name"] = "runProject";
        runProjectTool["description"] = "Run the current project";
        QJsonObject runProjectInputSchema;
        runProjectInputSchema["type"] = "object";
        runProjectInputSchema["properties"] = QJsonObject();
        runProjectTool["inputSchema"] = runProjectInputSchema;
        tools.append(runProjectTool);

        // Clean project tool
        QJsonObject cleanProjectTool;
        cleanProjectTool["name"] = "cleanProject";
        cleanProjectTool["description"] = "Clean the current project";
        QJsonObject cleanProjectInputSchema;
        cleanProjectInputSchema["type"] = "object";
        cleanProjectInputSchema["properties"] = QJsonObject();
        cleanProjectTool["inputSchema"] = cleanProjectInputSchema;
        tools.append(cleanProjectTool);

        // List open files tool
        QJsonObject listOpenFilesTool;
        listOpenFilesTool["name"] = "listOpenFiles";
        listOpenFilesTool["description"] = "List currently open files";
        QJsonObject listOpenFilesInputSchema;
        listOpenFilesInputSchema["type"] = "object";
        listOpenFilesInputSchema["properties"] = QJsonObject();
        listOpenFilesTool["inputSchema"] = listOpenFilesInputSchema;
        tools.append(listOpenFilesTool);

        // List sessions tool
        QJsonObject listSessionsTool;
        listSessionsTool["name"] = "listSessions";
        listSessionsTool["description"] = "List available sessions";
        QJsonObject listSessionsInputSchema;
        listSessionsInputSchema["type"] = "object";
        listSessionsInputSchema["properties"] = QJsonObject();
        listSessionsTool["inputSchema"] = listSessionsInputSchema;
        tools.append(listSessionsTool);

        // Load session tool
        QJsonObject loadSessionTool;
        loadSessionTool["name"] = "loadSession";
        loadSessionTool["description"] = "Load a specific session";
        QJsonObject loadSessionInputSchema;
        loadSessionInputSchema["type"] = "object";
        QJsonObject loadSessionProperties;
        loadSessionProperties["sessionName"]
            = QJsonObject{{"type", "string"}, {"description", "Name of the session to load"}};
        loadSessionInputSchema["properties"] = loadSessionProperties;
        loadSessionInputSchema["required"] = QJsonArray{"sessionName"};
        loadSessionTool["inputSchema"] = loadSessionInputSchema;
        tools.append(loadSessionTool);

        // List issues tool
        QJsonObject listIssuesTool;
        listIssuesTool["name"] = "listIssues";
        listIssuesTool["description"] = "List current issues (warnings and errors)";
        QJsonObject listIssuesInputSchema;
        listIssuesInputSchema["type"] = "object";
        listIssuesInputSchema["properties"] = QJsonObject();
        listIssuesTool["inputSchema"] = listIssuesInputSchema;
        tools.append(listIssuesTool);

        // Quit tool
        QJsonObject quitTool;
        quitTool["name"] = "quit";
        quitTool["description"] = "Quit Qt Creator";
        QJsonObject quitInputSchema;
        quitInputSchema["type"] = "object";
        quitInputSchema["properties"] = QJsonObject();
        quitTool["inputSchema"] = quitInputSchema;
        tools.append(quitTool);

        QJsonObject toolsResult;
        toolsResult["tools"] = tools;
        result = toolsResult;
    } else {
        // Unknown method
        response["jsonrpc"] = "2.0";
        response["id"] = 1;
        QJsonObject error;
        error["code"] = -32601;
        error["message"] = QJsonValue(QString("Unknown method: ") + method);
        response["error"] = error;
        return response;
    }

    // Create success response
    response["jsonrpc"] = "2.0";
    response["id"] = 1;
    response["result"] = result;

    return response;
}

void MCPServer::handleNewConnection()
{
    QTcpSocket *client = m_tcpServerP->nextPendingConnection();
    if (!client)
        return;

    m_clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &MCPServer::handleClientData);
    connect(client, &QTcpSocket::disconnected, this, &MCPServer::handleClientDisconnected);

    qCDebug(mcpServer) << "New TCP client connected, total clients:" << m_clients.size();
}

void MCPServer::handleClientData()
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

    // Process the MCP request
    QJsonObject response = processRequest(doc.object());
    sendResponse(client, response);
}

void MCPServer::handleClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client)
        return;

    m_clients.removeAll(client);
    client->deleteLater();

    qCDebug(mcpServer) << "TCP client disconnected, remaining clients:" << m_clients.size();
}

void MCPServer::sendResponse(QTcpSocket *client, const QJsonObject &response)
{
    if (!client)
        return;

    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    qCDebug(mcpServer) << "Sending TCP response:" << data;
    client->write(data);
    client->flush();
}

} // namespace Internal
} // namespace MCP
