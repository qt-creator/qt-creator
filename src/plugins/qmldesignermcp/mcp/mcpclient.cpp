// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "mcpclient.h"

#include <utils/hostosinfo.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QTimer>

namespace {

QString jsonToCompact(const QJsonObject &o)
{
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

} // namespace

namespace QmlDesigner {

McpClient::McpClient(QObject *parent)
    : QObject(parent)
{}

McpClient::~McpClient()
{
    stop();
}

/*!
 *  Start an MCP server as a subprocess (stdio transport).
 *  Example: command="python", args={"qmlDesignerMcpServer","/projectPath","PATH_TO_THE_PROJECT"}
 *
 *  \param command
 *     Path or name of the executable to launch. Can be absolute, relative,
 *     or resolved via \p env / system PATH depending on platform.
 *  \param args
 *     List of arguments passed to the process in order.
 *  \param workingDir
 *     Working directory for the new process.
 *  \param env
 *     Environment variables for the new process.
 *  \return
 *     \c true if the process was successfully started; otherwise \c false.

 */
bool McpClient::startProcess(
    const QString &command,
    const QStringList &args,
    const QString &workingDir,
    const QProcessEnvironment &env)
{
    if (m_process)
        stop();

    m_process = new QProcess(this);
    m_process->setProcessEnvironment(env);
    if (!workingDir.isEmpty())
        m_process->setWorkingDirectory(workingDir);

    if (Utils::HostOsInfo::isWindowsHost()) {
        // Ensure no extra console window if embedding in a GUI product.
        m_process->setCreateProcessArgumentsModifier(
            [](QProcess::CreateProcessArguments *args) { Q_UNUSED(args); });
    }

    connect(m_process, &QProcess::readyReadStandardOutput, this, &McpClient::onStdoutReady);
    connect(m_process, &QProcess::readyReadStandardError, this, &McpClient::onStderrReady);
    connect(m_process, &QProcess::started, this, &McpClient::onProcessStarted);
    connect(
        m_process,
        qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        this,
        &McpClient::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &McpClient::onProcessError);

    m_process->start(command, args, QIODevice::ReadWrite | QIODevice::Unbuffered);
    if (!m_process->waitForStarted(8000)) {
        emit errorOccurred(tr("Failed to start MCP server: %1").arg(m_process->errorString()));
        return false;
    }
    return true;
}

void McpClient::stop(int killTimeoutMs)
{
    if (!m_process)
        return;

    if (m_process->state() != QProcess::NotRunning) {
        m_process->closeWriteChannel();
        m_process->terminate();
        if (!m_process->waitForFinished(killTimeoutMs)) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
    }

    m_process->deleteLater();
    m_process = nullptr;
    m_stdoutBuffer.clear();
    m_pendingResponses.clear();
    m_nextId = 1;
    m_initializedConfirmed = false;
}

bool McpClient::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

/*!
 * \brief Kick off handshake. Emits connected(serverInfo) on success.
 * \param protocolVersion defaults to a recent MCP spec revision
 * \return request id
 *  For each MCP spec: client sends "initialize" then server responds, then client sends "initialized".
 *  Include capability hints; these can be expanded as your host adds features.
 */
qint64 McpClient::initialize(
    const QString &protocolVersion, const QString &clientName, const QString &clientVersion)
{
    QJsonObject params{
        {"protocolVersion", protocolVersion},
        {"capabilities",
         QJsonObject{
             {"roots", QJsonObject{{"listChanged", true}}},
             {"sampling", QJsonObject{}}, // TODO
         }},
        {"clientInfo",
         QJsonObject{
             {"name", clientName},
             {"version", clientVersion},
         }},
    };

    return sendRequest(
        QStringLiteral("initialize"),
        params,
        [this](int, const QJsonObject &result, const QJsonObject &error) {
            if (!error.isEmpty()) {
                emit errorOccurred(
                    tr("initialize failed: %1").arg(error.value("message").toString()));
                return;
            }

            m_serverInfo = {
                .name = result.value("serverInfo").toObject().value("name").toString(),
                .version = result.value("serverInfo").toObject().value("version").toString(),
                .protocolVersion = result.value("protocolVersion").toString(),
                // TODO: save server capabilities
            };

            emit connected(m_serverInfo);
            QTimer::singleShot(0, this, &McpClient::confirmClientInitialized);
        });
}

qint64 McpClient::listTools()
{
    return sendRequest(
        QStringLiteral("tools/list"),
        QJsonObject(),
        [this](int requestId, const QJsonObject &result, const QJsonObject &error) {
            if (!error.isEmpty()) {
                emit errorOccurred(
                    tr("tools/list failed: %1").arg(error.value("message").toString()));
                return;
            }
            QList<McpTool> mcpTools;
            const QJsonArray toolsArray = result.value("tools").toArray();
            mcpTools.reserve(toolsArray.size());
            for (const QJsonValue &v : toolsArray) {
                const QJsonObject obj = v.toObject();
                mcpTools.append({
                    .name = obj.value("name").toString(),
                    .description = obj.value("description").toString(),
                    .inputSchema = obj.value("inputSchema").toObject(),
                });
            }
            emit toolsListed(mcpTools, requestId);
        });
}

qint64 McpClient::callTool(const QString &toolName, const QJsonObject &arguments)
{
    QJsonObject params{{"name", toolName}, {"arguments", arguments}};
    return sendRequest(
        QStringLiteral("tools/call"),
        params,
        [this](int requestId, const QJsonObject &result, const QJsonObject &error) {
            if (error.isEmpty())
                emit toolCallSucceeded(result, requestId);
            else
                emit toolCallFailed(error.value("message").toString(), error, requestId);
        });
}

qint64 McpClient::listResources()
{
    return sendRequest(
        QStringLiteral("resources/list"),
        QJsonObject(),
        [this](int requestId, const QJsonObject &result, const QJsonObject &error) {
            if (!error.isEmpty()) {
                emit errorOccurred(
                    tr("resources/list failed: %1").arg(error.value("message").toString()));
                return;
            }
            QList<McpResource> mcpResource;
            const QJsonArray resourcesArray = result.value("resources").toArray();
            mcpResource.reserve(resourcesArray.size());
            for (const QJsonValue &v : resourcesArray) {
                const QJsonObject obj = v.toObject();
                mcpResource.append({
                    .name = obj.value("name").toString(),
                    .description = obj.value("description").toString(),
                    .uri = obj.value("uri").toString(),
                    .mimeType = obj.value("mimeType").toString(),
                });
            }
            emit resourcesListed(mcpResource, requestId);
        });
}

qint64 McpClient::readResource(const QString &uri)
{
    QJsonObject params{{"uri", uri}};
    return sendRequest(
        QStringLiteral("resources/read"),
        params,
        [this](int requestId, const QJsonObject &result, const QJsonObject &error) {
            if (error.isEmpty())
                emit resourceReadSucceeded(result, requestId);
            else
                emit resourceReadFailed(error.value("message").toString(), error, requestId);
        });
}

// Returns request id. If callback is empty, you will only get signals.
qint64 McpClient::sendRequest(
    const QString &method, const QJsonObject &params, ResponseHandler callback)
{
    if (!m_process || m_process->state() != QProcess::Running) {
        emit errorOccurred(tr("sendRequest called but process not running"));
        return -1;
    }
    const qint64 id = m_nextId++;
    QJsonObject req{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}};
    sendRpcToServer(req);

    PendingResponse pendingResponse;
    pendingResponse.method = method;
    if (callback)
        pendingResponse.callback = std::move(callback);
    m_pendingResponses.insert(id, std::move(pendingResponse));
    return id;
}

void McpClient::onStdoutReady()
{
    // MCP stdio transport: messages are newline-delimited JSON, no embedded newlines.
    // We'll split by '\n'. If the server ever prints partial lines, we buffer.
    m_stdoutBuffer.append(m_process->readAllStandardOutput());
    int idx = -1;
    while ((idx = m_stdoutBuffer.indexOf('\n')) != -1) {
        const QByteArray line = m_stdoutBuffer.left(idx);
        m_stdoutBuffer.remove(0, idx + 1);
        if (!line.trimmed().isEmpty())
            handleIncomingLine(line);
    }
}

void McpClient::onStderrReady()
{
    const QByteArray data = m_process->readAllStandardError();
    const QStringList lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
        emit logMessage(line);
}

void McpClient::onProcessStarted()
{
    emit started();
}

void McpClient::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    emit exited(exitCode, status);
}

void McpClient::onProcessError(QProcess::ProcessError e)
{
    emit errorOccurred(tr("Process error: %1").arg(int(e)));
}

void McpClient::sendRpcToServer(const QJsonObject &obj)
{
    const QByteArray line = QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
    qint64 n = m_process->write(line);
    if (n != line.size())
        emit errorOccurred(tr("Failed to write full JSON line to server"));
}

void McpClient::handleIncomingLine(const QByteArray &line)
{
    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(line, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred(tr("Invalid JSON from server: %1").arg(QString::fromUtf8(line)));
        return;
    }
    const QJsonObject obj = doc.object();

    // Distinguishes between JSON‑RPC responses and notifications based on
    // the presence of an "id" or "method" field. Messages with an "id"
    // are treated as responses, those with a "method" as notifications.
    if (obj.contains(QStringLiteral("id")))
        handleResponse(obj);
    else if (obj.contains(QStringLiteral("method")))
        handleNotification(obj);
    else
        emit errorOccurred(tr("Unknown MCP frame: %1").arg(jsonToCompact(obj)));
}

void McpClient::handleResponse(const QJsonObject &obj)
{
    const qint64 id = obj.value("id").toInteger();
    const auto it = m_pendingResponses.find(id);
    const QJsonObject result = obj.value("result").toObject();
    const QJsonObject error = obj.value("error").toObject();

    if (it != m_pendingResponses.end()) {
        const QString method = it->method;
        auto callback = std::move(it->callback);
        m_pendingResponses.erase(it);
        if (callback)
            callback(id, result, error);
    } else if (!error.isEmpty()) {
        emit errorOccurred(
            tr("Untracked response id=%1: %2").arg(id).arg(error.value("message").toString()));
    }
}

void McpClient::handleNotification(const QJsonObject &obj)
{
    const QString method = obj.value("method").toString();
    const QJsonObject params = obj.value("params").toObject();
    emit notificationReceived(method, params);
}

void McpClient::confirmClientInitialized()
{
    if (m_initializedConfirmed)
        return;

    m_initializedConfirmed = true;
    QJsonObject notif{
        {"jsonrpc", "2.0"},
        {"method", "notifications/initialized"},
    };
    sendRpcToServer(notif);
}

} // namespace QmlDesigner
