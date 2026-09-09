// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpserver.h"

#include "../schemas/schema_2026_07_28.h"

#include <utils/algorithm.h>
#include <utils/result.h>
#include <utils/utility.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QLoggingCategory>
#include <QTcpServer>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <map>

#ifdef MCP_SERVER_HAS_QT_HTTP_SERVER
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QHttpServerResponse>
#else
#include "minihttpserver.h"
// Bring fallback types into the global namespace so the rest of the file
// compiles unchanged whether or not Qt::HttpServer is present.
using QHttpServer = MiniHttp::HttpServer;
using QHttpServerRequest = MiniHttp::HttpRequest;
using QHttpServerResponder = MiniHttp::HttpResponder;
using QHttpServerResponse = MiniHttp::HttpResponse;
using QHttpHeaders = MiniHttp::HttpHeaders;
#endif

static Q_LOGGING_CATEGORY(mcpServerLog, "mcp.server", QtWarningMsg)
static Q_LOGGING_CATEGORY(mcpServerIOLog, "mcp.server.io", QtWarningMsg)

// Accept carries a list, and its entries may have parameters. Comparing the
// header as a whole rejects every client that sends more than one type - which
// the protocol requires them to do on POST. Media types are case-insensitive
// and may be given as the "*/*" and "type/*" wildcards.
static bool accepts(const QHttpServerRequest &request, QByteArrayView mediaType)
{
    // value() is a QByteArrayView with QHttpHeaders and a QByteArray with
    // MiniHttp; only the latter has split().
    const QByteArray accept = QByteArrayView(request.headers().value("Accept")).toByteArray();
    const qsizetype slash = mediaType.indexOf('/');
    const QByteArrayView type = slash < 0 ? QByteArrayView() : mediaType.first(slash);
    for (const QByteArray &entry : accept.split(',')) {
        const QList<QByteArray> parts = entry.split(';');
        // "q=0" refuses that type rather than accepting it.
        bool refused = false;
        for (qsizetype i = 1; i < parts.size() && !refused; ++i) {
            const QByteArray param = parts.at(i).trimmed();
            refused = param.startsWith("q=") && param.mid(2).trimmed().toDouble() == 0.0;
        }
        if (refused)
            continue;
        const QByteArray value = parts.constFirst().trimmed();
        if (value == "*/*" || value.compare(mediaType, Qt::CaseInsensitive) == 0)
            return true;
        // A media type given without a subtype has no "type/*" to match.
        if (!type.isEmpty() && value.endsWith("/*")
            && QByteArrayView(value).chopped(2).compare(type, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

using UniqueDeleteLaterTimer = std::unique_ptr<QTimer, QScopedPointerObjectDeleteLater<QTimer>>;

using namespace Utils;

namespace Mcp {

static constexpr int s_maxPageSize = 200;

enum ErrorCodes {
    // Defined by JSON RPC
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    serverErrorStart = -32099,
    serverErrorEnd = -32000,
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001,
    // Implementation-defined. The -32000..-32099 range is nominally the
    // server's, but the specification allocates inside it - the schemas here
    // carry -32020, -32021 and -32022 for 2026-07-28 and -32042 for
    // 2025-11-25 - so these two sit at the far end of it, where the allocated
    // codes are furthest away, starting one in from -32099, which
    // serverErrorStart above already names. A revision that reaches this far
    // would collide silently, which is a property of the range rather than of
    // the choice.
    TooManySubscriptions = -32098,
    SubscriptionIdInUse = -32097,
    RequestCancelled = -32800,
    // Reserved for the MCP specification since 2026-07-28. -32021
    // (MissingRequiredClientCapability) has nowhere to be raised: a capability
    // the client lacks only shows up once a tool asks for something, and is
    // answered there as a tool error.
    HeaderMismatch = -32020,
    UnsupportedProtocolVersion = -32022,
};

namespace V2026 = Generated::Schema::_2026_07_28;

// The 2026-07-28 revision drops the initialize handshake and the session
// header: each request instead carries its protocol version, the client's
// capabilities and the client's identity in these reserved _meta keys.
namespace MetaKey {
constexpr QLatin1String protocolVersion{"io.modelcontextprotocol/protocolVersion"};
constexpr QLatin1String clientCapabilities{"io.modelcontextprotocol/clientCapabilities"};
constexpr QLatin1String clientInfo{"io.modelcontextprotocol/clientInfo"};
constexpr QLatin1String serverInfo{"io.modelcontextprotocol/serverInfo"};
constexpr QLatin1String subscriptionId{"io.modelcontextprotocol/subscriptionId"};
} // namespace MetaKey

constexpr QLatin1String kVersion2026{"2026-07-28"};

// Keep this list sorted in descending order of version, so that the first entry
// is the latest supported version. Used to negotiate in the initialize request
// body (onInitialize) and to validate the MCP-Protocol-Version header on all
// subsequent HTTP requests.
static const QStringList kSupportedProtocolVersions
    = {"2026-07-28", "2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05"};

// Freshness hint for the list/read results that 2026-07-28 requires to be
// cacheable. Scoped private: the tool set is specific to this Qt Creator
// instance, so shared intermediaries must not serve it to anyone else.
//
// A minute is what a client may serve a stale list for, and it is a ceiling
// rather than a schedule: a list that changes sends list_changed, and a client
// that listens for it re-reads at once. One that does not listen is the one
// this bounds.
static constexpr int kCacheTtlMs = 60000;
constexpr QLatin1String kCacheScopePrivate{"private"};
constexpr QLatin1String kResultTypeComplete{"complete"};
constexpr QLatin1String kResultTypeInputRequired{"input_required"};
constexpr QLatin1String kResultTypeTask{"task"};

// Tasks left core in 2026-07-28 and are negotiated as an extension: the client
// names it in its per-request capabilities, the server in server/discover.
constexpr QLatin1String kTasksExtension{"io.modelcontextprotocol/tasks"};

// Polling is how a 2026-07-28 client follows a task, so a handle that carried no
// interval would leave it guessing.
static constexpr int kDefaultTaskPollIntervalMs = 1000;

// How long a request parked on input_required waits for the retry that would
// resume it. Nothing obliges a client to come back, and the parked continuation
// owns the suspended tool call, so an entry that never expired would pin it for
// the lifetime of the process. Long enough for a person to be asked a question
// and answer it.
static constexpr int kPendingInputTtlMs = 300000;

// Synthetic session identifiers for 2026-07-28 clients carry this prefix, which
// is how code further down recognises which revision it is answering.
constexpr QLatin1String kSession2026Prefix{"2026-07-28:"};

static bool is2026Session(const QString &sessionId)
{
    return sessionId.startsWith(kSession2026Prefix);
}

static QJsonValue toJsonValue(const Schema::RequestId &id)
{
    return std::visit([](const auto &v) { return QJsonValue(v); }, id);
}

static Schema::RequestId toRequestId(const QJsonValue &id)
{
    if (id.isString())
        return id.toString();
    return id.toInt();
}

// Methods a 2026-07-28 client may call. initialize, ping, logging/setLevel and
// resources/subscribe|unsubscribe were removed in this revision, so they must
// not be reachable through it. tasks/* stopped being core as well, but comes
// back as the io.modelcontextprotocol/tasks extension and is served here in
// that role.
static bool isKnown2026Method(const QString &method)
{
    static const QSet<QString> methods{
        "server/discover",
        "subscriptions/listen",
        "tools/list",
        "tools/call",
        "prompts/list",
        "prompts/get",
        "resources/list",
        "resources/read",
        "resources/templates/list",
        "completion/complete",
        "tasks/get",
        "tasks/update",
        "tasks/cancel",
    };
    return methods.contains(method);
}

// Results of these methods carry the CacheableResult fields (ttlMs, cacheScope).
static bool is2026CacheableMethod(const QString &method)
{
    static const QSet<QString> methods{
        "tools/list",
        "prompts/list",
        "resources/list",
        "resources/read",
        "resources/templates/list",
    };
    return methods.contains(method);
}

static QJsonObject requestMeta(const QJsonObject &message)
{
    return message.value("params").toObject().value("_meta").toObject();
}

Dialect dialectFor(const QJsonObject &message, const QString &versionHeader)
{
    const QString version = requestMeta(message).value(MetaKey::protocolVersion).toString();

    // A 2026-07-28 client states the revision in both places; an older one
    // states it in the header alone and leaves the body silent. So it is a
    // header naming 2026-07-28 that is held to the body's word, in both
    // directions, and a body with no header behind it is taken at its word.
    if (!versionHeader.isEmpty() && (versionHeader == kVersion2026) != (version == kVersion2026))
        return Dialect::Mismatch;

    if (version == kVersion2026)
        return Dialect::Revision2026;

    // It is the value that decides and not the presence of the key: an older
    // client that states its own version is still an older client, and belongs
    // with the handlers that speak to it.
    if (version.isEmpty() || kSupportedProtocolVersions.contains(version))
        return Dialect::Legacy;

    return Dialect::Unsupported;
}

// The tasks extension names the same fields as the retired core tasks did, but
// spells the two durations in milliseconds. ttlMs is required and nullable.
static QJsonObject toExtensionTask(const Schema::Task &task)
{
    QJsonObject obj{
        {"taskId", task.taskId()},
        {"status", Schema::toString(task.status())},
        {"createdAt", task.createdAt()},
        {"lastUpdatedAt", task.lastUpdatedAt()},
        {"ttlMs", task.ttl() ? QJsonValue(*task.ttl()) : QJsonValue(QJsonValue::Null)},
    };
    if (task.pollInterval())
        obj.insert("pollIntervalMs", *task.pollInterval());
    if (task.statusMessage())
        obj.insert("statusMessage", *task.statusMessage());
    return obj;
}

static bool declaresTasksExtension(const QJsonObject &clientCapabilities)
{
    return clientCapabilities.value("extensions").toObject().contains(kTasksExtension);
}

// 2026-07-28 dropped the per-request `task` opt-in from elicitation and
// sampling params; the surrounding task carries that meaning instead.
static QJsonObject withoutLegacyTask(QJsonObject params)
{
    params.remove("task");
    return params;
}

class SseStream : public QObject
{
public:
    SseStream(const QHttpHeaders &headers, QHttpServerResponder &&_responder)
        : responder(std::make_shared<QHttpServerResponder>(std::move(_responder)))
        , sessionId(QString::fromUtf8(headers.value("mcp-session-id")))
    {
        initStream(headers);
    }

    SseStream(const QHttpHeaders &headers, const std::shared_ptr<QHttpServerResponder> &_responder)
        : responder(_responder)
        , sessionId(QString::fromUtf8(headers.value("mcp-session-id")))
    {
        initStream(headers);
    }

    ~SseStream() { responder->writeEndChunked({"\n\n"}); }

    void initStream(QHttpHeaders headers)
    {
        qCDebug(mcpServerLog) << "Starting SSE stream for session"
                              << headers.value("mcp-session-id");
        headers.append("Content-type", "text/event-stream");

        responder->writeBeginChunked(headers, QHttpServerResponder::StatusCode::Ok);
    }

    bool sendData(const QByteArray &data, const QString &sId)
    {
        if (responder->isResponseCanceled())
            return false;

        if (!sId.isEmpty() && sessionId != sId)
            return true; // Not for this stream

        QByteArray event = "data: " + data + "\n\n";
        responder->writeChunk(event);
        return true;
    }

    bool sendEndpoint(const QByteArray &endpoint)
    {
        if (responder->isResponseCanceled())
            return false;

        QByteArray event = "event: endpoint\n" + QByteArray("data: ") + endpoint + "\n\n";
        responder->writeChunk(event);
        return true;
    }

    bool isCanceled() const { return responder->isResponseCanceled(); }

    // Send an SSE comment to keep the TCP connection alive through idle-timeout
    // firewalls/NAT. Returns false if the stream has been canceled.
    bool sendPing()
    {
        if (responder->isResponseCanceled())
            return false;
        responder->writeChunk(": heartbeat\n\n");
        return true;
    }

    QString sessionIdValue() const { return sessionId; }

private:
    std::shared_ptr<QHttpServerResponder> responder;
    const QString sessionId;
};

static QJsonObject makeResponse(Schema::RequestId id, const Schema::ServerResult &result)
{
    if (std::holds_alternative<Schema::CallToolResult>(result)) {
        auto callToolResult = std::get<Schema::CallToolResult>(result);
        if (callToolResult.structuredContent().has_value()) {
            // Copy structured content into content for backwards compatibility
            // with clients that don't support structured content.
            QJsonDocument doc(callToolResult.structuredContentAsObject());
            QByteArray json = doc.toJson(QJsonDocument::Compact);
            callToolResult.addContent(Schema::TextContent().text(QString::fromUtf8(json)));

            return Schema::toJson(
                Schema::JSONRPCResultResponse().id(id).result(
                    Schema::Result().additionalProperties(Schema::toJson(callToolResult))));
        }
    }

    return Schema::toJson(
        Schema::JSONRPCResultResponse().id(id).result(
            Schema::Result().additionalProperties(Schema::toJson(result))));
};

struct Responder
{
    std::function<void(QJsonDocument)> write;
    std::function<void(QHttpServerResponder::StatusCode)> writeStatus;
    std::function<void(const QByteArray &, const char *, QHttpServerResponse::StatusCode)> writeData;
    std::function<void(const QByteArray &)> writeSSE;
    std::function<bool()> isCanceled;
};

static QJsonObject jsonRpcError(const QJsonValue &id, int code, const QString &message)
{
    return QJsonObject{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", QJsonObject{{"code", code}, {"message", message}}}};
}

// The errors 2026-07-28 reserves for a malformed request - a header that
// contradicts the body, an unsupported version - are the ones it also requires
// to carry 400 Bad Request. Errors that are merely the answer to a well-formed
// call stay on the 200 that JSON-RPC expects.
static void writeError(
    const Responder &responder, const QJsonValue &id, int code, const QString &message)
{
    responder.writeData(
        QJsonDocument(jsonRpcError(id, code, message)).toJson(QJsonDocument::Compact),
        "application/json",
        QHttpServerResponse::StatusCode::BadRequest);
}

// The request body, or nothing once the malformed one has been answered.
static std::optional<QJsonObject> parseRequestBody(const QByteArray &data,
                                                   const Responder &responder)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        responder.writeData(
            "Invalid JSON body", "text/plain", QHttpServerResponse::StatusCode::BadRequest);
        return {};
    }
    return document.object();
}

// Lifts a response produced by the shared handlers to what 2026-07-28 requires:
// every result is tagged with its resultType, list and read results carry the
// caching hints, and the server identifies itself in the result's _meta.
static QJsonObject to2026Response(
    QJsonObject response, const QString &method, const Schema::Implementation &serverInfo)
{
    if (!response.contains("result"))
        return response;

    QJsonObject result = response.value("result").toObject();
    // An interim input_required result already states its type; only ordinary
    // results need tagging as complete.
    if (!result.contains("resultType"))
        result.insert("resultType", QString(kResultTypeComplete));

    if (is2026CacheableMethod(method) && !result.contains("ttlMs")) {
        result.insert("ttlMs", kCacheTtlMs);
        result.insert("cacheScope", QString(kCacheScopePrivate));
    }

    QJsonObject meta = result.value("_meta").toObject();
    meta.insert(MetaKey::serverInfo, Schema::toJson(serverInfo));
    result.insert("_meta", meta);

    response.insert("result", result);
    return response;
}

struct ToolInterfacePrivate;

static QString jsonKind(const QJsonValue &value)
{
    switch (value.type()) {
    case QJsonValue::Null: return "null";
    case QJsonValue::Bool: return "a boolean";
    case QJsonValue::Double: return "a number";
    case QJsonValue::String: return "a string";
    case QJsonValue::Array: return "an array";
    case QJsonValue::Object: return "an object";
    case QJsonValue::Undefined: break;
    }
    return "nothing";
}

static QString jsonText(const QJsonValue &value)
{
    switch (value.type()) {
    case QJsonValue::Null: return "null";
    case QJsonValue::Bool: return value.toBool() ? QLatin1String("true") : QLatin1String("false");
    case QJsonValue::Double:
        return QString::number(value.toDouble(), 'g', QLocale::FloatingPointShortest);
    case QJsonValue::String: return '"' + value.toString() + '"';
    case QJsonValue::Array:
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    case QJsonValue::Object:
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    case QJsonValue::Undefined: break;
    }
    return "nothing";
}

static Utils::Result<> validateValue(const QString &toolName, const QString &argumentName,
                                     const QJsonObject &property, const QJsonValue &value)
{
    const QJsonArray allowed = property.value("enum").toArray();
    if (!allowed.isEmpty()) {
        if (!allowed.contains(value)) {
            QStringList names;
            for (const QJsonValue &one : allowed)
                names += jsonText(one);
            return Utils::ResultError(
                QString("Invalid value %1 for argument \"%2\" of tool \"%3\". "
                        "Expected one of: %4")
                    .arg(jsonText(value), argumentName, toolName, names.join(", ")));
        }
        return Utils::ResultOk;
    }

    const QString wanted = property.value("type").toString();
    if (wanted.isEmpty())
        return Utils::ResultOk;
    const auto wrongKind = [&](const QString &got) {
        const QLatin1String article = QLatin1String("aeiou").contains(wanted.at(0))
                                          ? QLatin1String("an") : QLatin1String("a");
        return Utils::ResultError(
            QString("Argument \"%1\" of tool \"%2\" wants %3 %4, got %5")
                .arg(argumentName, toolName, article, wanted, got));
    };
    if (wanted == "string" && !value.isString())
        return wrongKind(jsonKind(value));
    // JSON has one number type, so an integer is a number whose value has
    // no fraction - which is what a caller sending 3 rather than 3.5 means.
    if ((wanted == "number" || wanted == "integer") && !value.isDouble())
        return wrongKind(jsonKind(value));
    if (wanted == "integer" && value.isDouble()
        && value.toDouble() != std::floor(value.toDouble())) {
        return wrongKind("a fractional number");
    }
    if (wanted == "boolean" && !value.isBool())
        return wrongKind(jsonKind(value));
    if (wanted == "array" && !value.isArray())
        return wrongKind(jsonKind(value));
    if (wanted == "object" && !value.isObject())
        return wrongKind(jsonKind(value));
    return Utils::ResultOk;
}

// This is deliberately not a full JSON Schema implementation. It checks
// the four things a caller gets wrong: a name that is not there, a
// missing required one, a value of the wrong kind, and a value outside an
// enum. Anything a schema says beyond that is left alone.
Utils::Result<> validateToolArguments(const Schema::Tool &tool,
                                      const Schema::CallToolRequestParams &params)
{
    static const QMap<QString, QJsonObject> noProperties;
    const Schema::Tool::InputSchema &schema = tool.inputSchema();
    const QMap<QString, QJsonObject> &properties = schema.properties() ? *schema.properties()
                                                                      : noProperties;
    const QJsonObject arguments = params.argumentsAsObject();

    // A name that is not declared comes first, because a misspelled argument
    // would otherwise be reported as the required one it failed to supply -
    // true, but silent about the actual mistake.
    for (auto it = arguments.constBegin(); it != arguments.constEnd(); ++it) {
        const QString &name = it.key();
        const auto property = properties.constFind(name);
        if (property == properties.constEnd()) {
            if (properties.isEmpty()) {
                return Utils::ResultError(
                    QString("Tool \"%1\" takes no arguments, but \"%2\" was passed")
                        .arg(tool.name(), name));
            }
            QStringList known = properties.keys();
            known.sort();
            return Utils::ResultError(
                QString("Unknown argument \"%1\" for tool \"%2\". Known: %3")
                    .arg(name, tool.name(), known.join(", ")));
        }
        if (const Utils::Result<> ok = validateValue(tool.name(), name, *property, it.value());
            !ok) {
            return ok;
        }
    }

    for (const QString &name : schema.required().value_or(QStringList{})) {
        if (!arguments.contains(name)) {
            return Utils::ResultError(
                QString("Missing required argument \"%1\" for tool \"%2\"")
                    .arg(name, tool.name()));
        }
    }
    return Utils::ResultOk;
}

class ServerPrivate : public std::enable_shared_from_this<ServerPrivate>
{
    Schema::Implementation serverInfo;

public:
    QString instructions;

    // 2026-07-28 removed the initialize handshake, so it can never be the
    // outcome of one: a client that gets here speaks an older revision.
    static constexpr QLatin1String kLatestHandshakeVersion{"2025-11-25"};

    // A stream opened by subscriptions/listen, with the notification types it
    // opted in to. 2026-07-28 has no session header, so these cannot be
    // reached through the session-keyed SSE fan-out.
    struct Listener
    {
        std::function<void(const QByteArray &)> send;
        std::function<bool()> isCanceled;
        V2026::SubscriptionFilter filter;
        QJsonValue subscriptionId;
        // The subscription id is the client's own JSON-RPC id, so it says
        // nothing about who opened the subscription. Only the session it was
        // opened for may end it.
        QString session;
    };

    ServerPrivate(Schema::Implementation serverInfo)
        : serverInfo(serverInfo)
    {
        // Send a heartbeat ping every 30 s to keep TCP connections alive through
        // idle-timeout firewalls/NAT. Prune any dead streams detected during the ping.
        m_heartbeatTimer.setInterval(std::chrono::seconds(30));
        m_heartbeatTimer.setSingleShot(false);
        QObject::connect(&m_heartbeatTimer, &QTimer::timeout, [this]() {
            m_sseStreams.erase(
                std::remove_if(
                    m_sseStreams.begin(),
                    m_sseStreams.end(),
                    [](const std::unique_ptr<SseStream> &stream) {
                        if (stream->sendPing())
                            return false;
                        qCDebug(mcpServerLog)
                            << "Heartbeat pruned dead stream for session"
                            << stream->sessionIdValue();
                        return true;
                    }),
                m_sseStreams.end());
        });
        m_heartbeatTimer.start();
    }

    bool bind(QTcpServer *server) { return m_server.bind(server); }

    QHttpHeaders corsHeaders(const QString &sessionId) const
    {
        QHttpHeaders headers;

        if (enableCors) {
            headers.append("Access-Control-Allow-Origin", "*");
            headers.append("Access-Control-Allow-Methods", "GET, POST, OPTIONS, DELETE");
            headers.append(
                "access-control-expose-headers",
                "mcp-session-id, last-event-id, mcp-protocol-version");
            headers.append(
                "Access-Control-Allow-Headers",
                "Authorization, Content-Type, mcp-session-id, last-event-id, "
                "mcp-protocol-version");
        }

        if (!sessionId.isNull())
            headers.append("mcp-session-id", sessionId.toUtf8());

        return headers;
    }

    static bool isLoopbackHost(const QString &host)
    {
        if (host.compare("localhost", Qt::CaseInsensitive) == 0)
            return true;
        const QHostAddress address(host);
        return !address.isNull() && address.isLoopback();
    }

    // Compared over the full length whatever the input, so that the time a
    // rejection takes does not tell a peer how much of the token it guessed.
    static bool tokenMatches(const QByteArray &expected, const QByteArray &actual)
    {
        if (expected.size() != actual.size())
            return false;
        quint8 differing = 0;
        for (qsizetype i = 0; i < expected.size(); ++i)
            differing |= quint8(expected.at(i)) ^ quint8(actual.at(i));
        return differing == 0;
    }

    // Loopback only says the peer is on this machine, which every other
    // process of every logged-in user is too. A token says it is the peer the
    // developer handed it to.
    Result<void> validateAuthorization(const QHttpServerRequest &req) const
    {
        if (authToken.isEmpty())
            return {};

        const QByteArray authorization
            = QByteArrayView(req.headers().value("Authorization")).toByteArray();
        const QByteArray scheme = authorization.left(7);
        if (scheme.compare("bearer ", Qt::CaseInsensitive) != 0)
            return ResultError(QString("No bearer token"));
        if (!tokenMatches(authToken, authorization.mid(scheme.size()).trimmed()))
            return ResultError(QString("Bearer token not accepted"));

        return {};
    }

    // A peer that reaches the port is not necessarily the one the developer
    // meant to serve. A browser can be made to send requests here by any page
    // it renders, and a DNS name rebound to 127.0.0.1 makes a remote page
    // same-origin with this server. Requiring the Origin, when there is one,
    // to be loopback refuses the first; refusing a Host that names anything
    // but an address or localhost refuses the second, since rebinding needs a
    // name. Clients that are not browsers send no Origin and are unaffected.
    Result<void> validateRequest(const QHttpServerRequest &req)
    {
        if (req.headers().contains("Host")) {
            const QString host
                = QUrl("http://" + QString::fromUtf8(req.headers().value("Host"))).host();
            if (QHostAddress(host).isNull() && !isLoopbackHost(host))
                return ResultError(QString("Host not allowed: %1").arg(host));
        }

        if (!req.headers().contains("Origin"))
            return {};

        const QString originHeader = QString::fromUtf8(req.headers().value("Origin"));
        if (originHeader.isEmpty())
            return ResultError("Empty origin header");

        const QUrl origin(originHeader);
        if (!origin.isValid())
            return ResultError(QString("Invalid Origin header: %1").arg(originHeader));

        if (!isLoopbackHost(origin.host()))
            return ResultError(QString("Origin not allowed: %1").arg(origin.toString()));

        return {};
    }

    // Routes registered through here run only for requests that passed the
    // transport-level checks, so a new route cannot forget them.
    template<typename Path>
    void routeChecked(
        const Path &path,
        QHttpServerRequest::Method method,
        std::function<void(const QHttpServerRequest &, QHttpServerResponder &)> handler)
    {
        m_server.route(
            path,
            method,
            [this, method, handler = std::move(handler)](
                const QHttpServerRequest &req, QHttpServerResponder &responder) {
                // A CORS preflight carries no Authorization header, so demanding
                // one there would refuse the request that asks whether the real
                // one may be sent. It reaches no handler and reads nothing.
                if (method != QHttpServerRequest::Method::Options) {
                    if (const Result<void> allowed = validateAuthorization(req); !allowed) {
                        qCWarning(mcpServerLog) << "Rejected request:" << allowed.error();
                        QHttpHeaders headers = corsHeaders({});
                        headers.append("content-type", "text/plain");
                        headers.append("WWW-Authenticate", "Bearer");
                        responder.write(
                            "Unauthorized",
                            headers,
                            QHttpServerResponse::StatusCode::Unauthorized);
                        return;
                    }
                }
                if (const Result<void> valid = validateRequest(req); !valid) {
                    qCWarning(mcpServerLog) << "Rejected request:" << valid.error();
                    QHttpHeaders headers = corsHeaders({});
                    headers.append("content-type", "text/plain");
                    responder.write(
                        "Request rejected", headers, QHttpServerResponse::StatusCode::BadRequest);
                    return;
                }
                handler(req, responder);
            });
    }

    // A session whose peer walked away without DELETE would keep its slot for
    // the rest of the run, so the cap alone would trade an unbounded leak for a
    // permanent refusal. lastSeen is stamped on every request, so a session
    // that is still in use is never the one reclaimed here - except for the one
    // client that sends none: a 2026-07-28 client that opened a subscription
    // and listens. That one is reclaimed too, rather than pinning a slot for
    // the rest of the run, and deleteSession() ends its subscriptions where it
    // can hear it: a stream that goes quiet on its own is the failure this is
    // the other side of.
    void reclaimIdleSessions()
    {
        pruneCanceledListeners();

        const QDateTime now = QDateTime::currentDateTime();
        QStringList idle;
        for (auto it = m_sessions.cbegin(); it != m_sessions.cend(); ++it) {
            if (it.value() && it.value()->lastSeen.secsTo(now) > kSessionIdleTimeoutSecs)
                idle.append(it.key());
        }
        for (const QString &sessionId : std::as_const(idle)) {
            qCInfo(mcpServerLog) << "Reclaiming idle session" << sessionId;
            deleteSession(sessionId);
        }
    }

    bool sessionLimitReached()
    {
        if (m_sessions.size() < kMaxSessions)
            return false;
        reclaimIdleSessions();
        return m_sessions.size() >= kMaxSessions;
    }

    void sendDataTo(const QByteArray &data, const QString &sessionId)
    {
        for (auto it = m_sseStreams.begin(); it != m_sseStreams.end();) {
            if (!(*it)->sendData(data, sessionId))
                it = m_sseStreams.erase(it);
            else
                ++it;
        }
    }

    void sendNotification(const Schema::ServerNotification &notification, const QString &sessionId)
    {
        const QJsonObject json = toJson(notification);
        auto data = QJsonDocument(json).toJson(QJsonDocument::Compact);

        if (m_ioOutputHandler)
            m_ioOutputHandler(data);

        // 2026-07-28 listeners cannot be reached through the session-keyed
        // fan-out below, and receive only what their filter named.
        deliverTo2026Listeners(json, sessionId);

        for (auto it = m_sseStreams.begin(); it != m_sseStreams.end();) {
            if (!(*it)->sendData(data, sessionId))
                it = m_sseStreams.erase(it);
            else
                ++it;
        }
    }

    void sendServerRequest(
        Schema::ServerRequest request,
        const QString &sessionId,
        std::function<void(Schema::JSONRPCResponse)> onResponse = {})
    {
        const int requestId = m_serverRequests.isEmpty() ? 1 : (m_serverRequests.lastKey() + 1);
        std::visit([requestId](auto &r) { r.id(requestId); }, request);

        m_serverRequests[requestId] = onResponse;

        auto data = QJsonDocument(toJson(request)).toJson(QJsonDocument::Compact);

        if (m_ioOutputHandler) {
            m_ioOutputHandler(data);
        }

        for (auto it = m_sseStreams.begin(); it != m_sseStreams.end();) {
            if (!(*it)->sendData(data, sessionId))
                it = m_sseStreams.erase(it);
            else
                ++it;
        }
    }

    // Returns true if the session is initialized (or was just auto-initialized) and
    // processing should continue. Sends an error and returns false for unknown sessions.
    bool validateSessionInitialized(
        Schema::RequestId id,
        const Schema::ClientRequest &request,
        const Responder &responder,
        const QString &sessionId)
    {
        if (m_sessions.value(sessionId).has_value())
            return true;

        if (m_sessions.contains(sessionId)) {
            // Session exists but was never fully initialized (SSE connected,
            // initialize never completed). Auto-initialize with empty capabilities
            // so the client can continue working rather than getting stuck in a
            // reconnect loop. Close-zombie causes a loop: erasing the stream triggers
            // reconnect, which creates another zombie and retries the same request.
            qCWarning(mcpServerLog)
                << "Received" << Schema::dispatchValue(request)
                << "on uninitialized session" << sessionId
                << "— auto-initializing to allow client to continue"
                << "(active sessions:" << m_sessions.keys() << ")";
            m_sessions.insert(sessionId, Client{});
            return true;
        }

        qCWarning(mcpServerLog)
            << "Received" << Schema::dispatchValue(request)
            << "with unknown session ID:" << sessionId
            << "— known sessions:" << m_sessions.keys();
        responder.write(QJsonDocument(toJson(
            Schema::JSONRPCErrorResponse()
                .error(Schema::Error()
                           .code(InvalidRequest)
                           .message("Session not initialized. Please reconnect and send initialize."))
                .id(id))));
        return false;
    }

    void onRequest(
        Schema::RequestId id,
        const Schema::ClientRequest &request,
        const Responder &responder,
        QString sessionId)
    {
        qCDebug(mcpServerLog) << "Received JSONRPCRequest:" << Schema::dispatchValue(request);

        if (std::holds_alternative<Schema::InitializeRequest>(request)) {
            if (m_sessions.contains(sessionId) && m_sessions[sessionId]) {
                qCWarning(mcpServerLog)
                    << "Received initialize request with already assigned session ID" << sessionId
                    << ", rejecting";
                responder.writeStatus(QHttpServerResponder::StatusCode::BadRequest);
                return;
            }

            onInitialize(id, std::get<Schema::InitializeRequest>(request), responder, sessionId);
            return;
        }

        if (!validateSessionInitialized(id, request, responder, sessionId))
            return;

        if (std::holds_alternative<Schema::CallToolRequest>(request)) {
            onToolCall(id, std::get<Schema::CallToolRequest>(request), responder, sessionId);
            return;
        } else if (std::holds_alternative<Schema::ListToolsRequest>(request)) {
            onToolsList(id, std::get<Schema::ListToolsRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ListPromptsRequest>(request)) {
            onPromptsList(id, std::get<Schema::ListPromptsRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::GetPromptRequest>(request)) {
            onGetPrompt(id, std::get<Schema::GetPromptRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ListResourcesRequest>(request)) {
            onResourcesList(id, std::get<Schema::ListResourcesRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ReadResourceRequest>(request)) {
            onReadResource(id, std::get<Schema::ReadResourceRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ListResourceTemplatesRequest>(request)) {
            onListResourceTemplates(
                id, std::get<Schema::ListResourceTemplatesRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::CompleteRequest>(request)) {
            onComplete(id, std::get<Schema::CompleteRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::GetTaskRequest>(request)) {
            onGetTask(id, std::get<Schema::GetTaskRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ListTasksRequest>(request)) {
            onListTasks(id, std::get<Schema::ListTasksRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::CancelTaskRequest>(request)) {
            onCancelTask(id, std::get<Schema::CancelTaskRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::GetTaskPayloadRequest>(request)) {
            onGetTaskPayload(id, std::get<Schema::GetTaskPayloadRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::PingRequest>(request)) {
            onPing(id, std::get<Schema::PingRequest>(request), responder);
            return;
        }

        responder.write(QJsonDocument(
            Schema::toJson(
                Schema::JSONRPCErrorResponse()
                    .error(
                        Schema::Error()
                            .code(MethodNotFound)
                            .message(QString("Method \"%1\" not implemented")
                                         .arg(Schema::dispatchValue(request))))
                    .id(id))));

        return;
    }

    void onInitialize(
        Schema::RequestId id,
        const Schema::InitializeRequest &request,
        const Responder &responder,
        const QString &sessionId)
    {
        // ** Version Negotiation
        // * In the initialize request, the client MUST send a protocol version it supports.
        //   This SHOULD be the latest version supported by the client.
        // * If the server supports the requested protocol version, it MUST respond with the same
        //   version. Otherwise, the server MUST respond with another protocol version it supports.
        //   This SHOULD be the latest version supported by the server.
        // * If the client does not support the version in the server's response,
        //   it SHOULD disconnect.
        // Latest supported version by the server.
        QString negotiatedVersion = kLatestHandshakeVersion;
        if (kSupportedProtocolVersions.contains(request.params().protocolVersion())
            && request.params().protocolVersion() != kVersion2026) {
            negotiatedVersion = request.params().protocolVersion();
        }

        qCDebug(mcpServerLog).noquote()
            << "Client initialized with protocol version" << Schema::toJson(request.params());

        auto caps = Schema::ServerCapabilities()
                        .prompts(Schema::ServerCapabilities::Prompts{}.listChanged(true))
                        .tools(Schema::ServerCapabilities::Tools().listChanged(true))
                        .resources(Schema::ServerCapabilities::Resources{}.listChanged(true))
                        .tasks(
                            Schema::ServerCapabilities::Tasks()
                                .list(QJsonObject{})
                                .cancel(QJsonObject{})
                                .requests(
                                    Schema::ServerCapabilities::Tasks::Requests().tools(
                                        Schema::ServerCapabilities::Tasks::Requests::Tools().call(
                                            QJsonObject{}))));

        if (m_completionCallback)
            caps = caps.completions(QJsonObject());

        auto initResult = Schema::InitializeResult()
                              .protocolVersion(negotiatedVersion)
                              .serverInfo(serverInfo)
                              .capabilities(caps);

        if (!instructions.isEmpty())
            initResult.instructions(instructions);

        qCDebug(mcpServerLog) << "Assigning session ID" << sessionId << "to new client";
        m_sessions.insert(
            sessionId, Client{request.params().capabilities(), request.params().clientInfo()});

        responder.write(QJsonDocument(makeResponse(id, initResult)));
    }

    QString createNewSessionId()
    {
        QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        qCDebug(mcpServerLog()) << "Generated new session ID" << sessionId;
        if (m_inspector)
            m_inspector->onSessionStarted(sessionId);
        return sessionId;
    }

    V2026::Implementation serverInfo2026() const
    {
        const auto converted = V2026::fromJson<V2026::Implementation>(Schema::toJson(serverInfo));
        return converted ? *converted : V2026::Implementation{};
    }

    struct Client
    {
        Schema::ClientCapabilities capabilities;
        Schema::Implementation info;
        QDateTime lastSeen = QDateTime::currentDateTime();
        // Declared per request via the 2026-07-28 extensions capability. A task
        // must never be handed to a client that did not ask for one.
        bool supportsTasks = false;
    };

    // 2026-07-28 has no session identifier. The shared handlers still need a
    // session to hang client capabilities off, so requests are grouped by the
    // client's self-reported identity and the entry is rewritten on every
    // request: the revision forbids inferring capabilities from earlier ones.
    //
    // The grouping is bookkeeping, not authority: nothing obliges a client to
    // name itself, so unrelated clients land on the same entry and overwrite
    // each other. Read what a request needs while it is being dispatched;
    // anything that outlives it has to capture it first. Where the identity
    // guards something - ending a subscription is the one place - it can only
    // tell differently named clients apart, and has to answer for the rest by
    // refusing rather than by guessing.
    //
    // Empty when a new entry would pass kMaxSessions.
    QString sessionFor2026(const QJsonObject &meta, const Client &client)
    {
        const QJsonObject info = meta.value(MetaKey::clientInfo).toObject();
        const QString sessionId = QString("%1%2/%3")
                                      .arg(kSession2026Prefix,
                                           info.value("name").toString("anonymous"),
                                           info.value("version").toString("unknown"));

        if (!m_sessions.contains(sessionId)) {
            if (sessionLimitReached())
                return {};
            if (m_inspector)
                m_inspector->onSessionStarted(sessionId);
        }

        m_sessions.insert(sessionId, client);

        return sessionId;
    }

    // The client a 2026-07-28 request declares in its _meta. Both revisions
    // describe the same JSON for the capability subset this server cares
    // about, so the generated codecs are bridged through JSON rather than
    // field by field. Unknown members - the revision's own `extensions`, the
    // earlier one's `tasks` - drop out; a capabilities object that cannot be
    // read at all does not, since a client that declares nothing and a client
    // whose declaration is broken have to be told apart.
    Result<Client> clientFor2026(const QJsonObject &meta) const
    {
        const QJsonValue declared = meta.value(MetaKey::clientCapabilities);
        if (!declared.isUndefined() && !declared.isObject())
            return ResultError(QString("%1 is not an object").arg(MetaKey::clientCapabilities));

        const QJsonObject caps = declared.toObject();
        const Result<Schema::ClientCapabilities> capabilities
            = Schema::fromJson<Schema::ClientCapabilities>(caps);
        if (!capabilities)
            return ResultError(capabilities.error());

        Client client;
        client.capabilities = *capabilities;
        client.supportsTasks = declaresTasksExtension(caps);
        if (const auto impl = Schema::fromJson<Schema::Implementation>(
                meta.value(MetaKey::clientInfo).toObject())) {
            client.info = *impl;
        }
        return client;
    }

    void on2026Discover(const QJsonValue &id, const Responder &responder)
    {
        auto caps = V2026::ServerCapabilities()
                        .prompts(V2026::ServerCapabilities::Prompts{}.listChanged(true))
                        .tools(V2026::ServerCapabilities::Tools{}.listChanged(true))
                        .resources(V2026::ServerCapabilities::Resources{}
                                       .listChanged(true)
                                       .subscribe(true))
                        .extensions(QMap<QString, QJsonObject>{{kTasksExtension, QJsonObject{}}});

        if (m_completionCallback)
            caps.completions(QJsonObject{});

        auto result = V2026::DiscoverResult()
                          .resultType(kResultTypeComplete)
                          .supportedVersions(kSupportedProtocolVersions)
                          .capabilities(caps)
                          .ttlMs(kCacheTtlMs)
                          .cacheScope(V2026::DiscoverResult::CacheScope::private_)
                          ._meta(V2026::ResultMetaObject()
                                     .iodotmodelcontextprotocolslashserverInfo(serverInfo2026()));

        if (!instructions.isEmpty())
            result.instructions(instructions);

        responder.write(QJsonDocument(QJsonObject{
            {"jsonrpc", "2.0"}, {"id", id}, {"result", V2026::toJson(result)}}));
    }

    // Entry point for 2026-07-28 traffic. Only the version-specific RPCs are
    // served here; the payloads the two revisions share are handed to
    // onRequest() and their responses lifted on the way out.
    // The header and the body have been held to each other by onData(), so the
    // version this acts on is the body's.
    void on2026Data(const Responder &responder, const QJsonObject &message)
    {
        const QString method = message.value("method").toString();
        const QJsonValue id = message.value("id");
        const QJsonObject meta = requestMeta(message);
        const bool isNotification = !message.contains("id");

        const QString version = meta.value(MetaKey::protocolVersion).toString();

        if (version != kVersion2026) {
            if (isNotification) {
                qCWarning(mcpServerLog)
                    << "Discarding" << method << "sent with protocol version" << version;
                responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
                return;
            }
            writeError(
                responder,
                id,
                UnsupportedProtocolVersion,
                QString("Unsupported protocol version \"%1\"").arg(version));
            return;
        }

        if (!isNotification && !isKnown2026Method(method)) {
            responder.write(QJsonDocument(jsonRpcError(
                id,
                MethodNotFound,
                QString("Method \"%1\" is not part of protocol version %2")
                    .arg(method, kVersion2026))));
            return;
        }

        if (method == "server/discover") {
            on2026Discover(id, responder);
            return;
        }

        const Result<Client> client = clientFor2026(meta);
        if (!client) {
            if (isNotification) {
                qCWarning(mcpServerLog)
                    << "Discarding" << method << "with unreadable _meta:" << client.error();
                responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
                return;
            }
            writeError(responder, id, InvalidParams, client.error());
            return;
        }

        const QString sessionId = sessionFor2026(meta, *client);
        if (sessionId.isEmpty()) {
            qCWarning(mcpServerLog)
                << "Refusing new session, session limit reached:" << m_sessions.size();
            responder.writeData("Too many sessions",
                                "text/plain",
                                QHttpServerResponse::StatusCode::ServiceUnavailable);
            return;
        }

        if (isNotification) {
            on2026Notification(message, responder, sessionId);
            return;
        }

        if (method == "subscriptions/listen") {
            on2026Listen(id, message, responder, sessionId);
            return;
        }

        Responder lifted = responder;
        lifted.write = [responder, method, info = serverInfo](QJsonDocument doc) {
            responder.write(QJsonDocument(to2026Response(doc.object(), method, info)));
        };
        lifted.writeSSE = [responder, method, info = serverInfo](const QByteArray &raw) {
            const QJsonObject obj = QJsonDocument::fromJson(raw).object();
            responder.writeSSE(QJsonDocument(to2026Response(obj, method, info))
                                   .toJson(QJsonDocument::Compact));
        };

        const QJsonObject params = message.value("params").toObject();

        if (method == "tasks/get") {
            on2026TaskGet(id, params.value("taskId").toString(), lifted);
            return;
        }
        if (method == "tasks/update") {
            on2026TaskUpdate(
                id,
                params.value("taskId").toString(),
                params.value("inputResponses").toObject(),
                lifted);
            return;
        }
        if (method == "tasks/cancel") {
            on2026TaskCancel(id, params.value("taskId").toString(), lifted);
            return;
        }

        // A retry carrying requestState resumes the call that asked for input
        // rather than starting a new one.
        const QString requestState = params.value("requestState").toString();
        if (!requestState.isEmpty()) {
            const auto pending = m_pendingInput.find(requestState);
            if (pending == m_pendingInput.end()) {
                responder.write(QJsonDocument(jsonRpcError(
                    id,
                    InvalidParams,
                    QString("Unknown or expired requestState \"%1\"").arg(requestState))));
                return;
            }
            const ResumeInput resume = pending->second.resume;
            m_pendingInput.erase(pending);
            resume(message.value("params").toObject().value("inputResponses").toObject(), lifted, id);
            return;
        }

        onLegacyData(QJsonDocument(message), lifted, sessionId);
    }

    // subscriptions/listen replaces the HTTP GET stream and
    // resources/subscribe: one long-lived response stream carrying only the
    // change notifications the client opted in to.
    void on2026Listen(
        const QJsonValue &id,
        const QJsonObject &message,
        const Responder &responder,
        const QString &sessionId)
    {
        // A client that opens a stream and disappears is only noticed when
        // something is delivered, which on a quiet server may be never.
        pruneCanceledListeners();

        // The pair (session, id) is all a cancel has to go on, so the server
        // keeps it unique rather than accepting a second subscription that
        // neither it nor the client could then end. Refused rather than
        // replaced: two instances of one client report one identity, and the
        // one that listened first must not lose its stream to the one that
        // listened second.
        const auto sameSubscription = [&id, &sessionId](const Listener &l) {
            return l.subscriptionId == id && l.session == sessionId;
        };
        if (Utils::anyOf(m_listeners, sameSubscription)) {
            qCWarning(mcpServerLog) << "Refusing subscription" << id << "of session" << sessionId
                                    << ": that id is already open";
            writeError(
                responder,
                id,
                SubscriptionIdInUse,
                "A subscription under this request id is already open for this session; "
                "cancel it before listening again");
            return;
        }

        // Nothing obliges a client to end a subscription before opening
        // another, and a stream that cannot report itself closed is never
        // pruned, so the count is capped rather than left to the client.
        const int held = listenerCount(sessionId);
        if (held >= kMaxListenersPerSession) {
            qCWarning(mcpServerLog) << "Refusing subscription" << id << "of session" << sessionId
                                    << ": the session already holds" << held;
            writeError(
                responder,
                id,
                TooManySubscriptions,
                QString(
                    "This session already holds %1 subscriptions; end one before "
                    "opening another")
                    .arg(kMaxListenersPerSession));
            return;
        }

        const QJsonObject filterJson
            = message.value("params").toObject().value("notifications").toObject();
        const auto parsedFilter = V2026::fromJson<V2026::SubscriptionFilter>(filterJson);
        const V2026::SubscriptionFilter filter = parsedFilter ? *parsedFilter
                                                              : V2026::SubscriptionFilter{};

        // The acknowledgement opens the stream and must be the first message
        // carrying the subscription's ID. A result would instead tell the
        // client the subscription has ended, so none is written here.
        const auto ack = V2026::SubscriptionsAcknowledgedNotification().params(
            V2026::SubscriptionsAcknowledgedNotificationParams().notifications(filter));

        QJsonObject ackJson = V2026::toJson(ack);
        QJsonObject ackParams = ackJson.value("params").toObject();
        QJsonObject ackMeta = ackParams.value("_meta").toObject();
        ackMeta.insert(MetaKey::subscriptionId, id);
        ackParams.insert("_meta", ackMeta);
        ackJson.insert("params", ackParams);
        responder.writeSSE(QJsonDocument(ackJson).toJson(QJsonDocument::Compact));

        m_listeners.push_back(
            Listener{responder.writeSSE, responder.isCanceled, filter, id, sessionId});
    }

    // notifications/cancelled is the only notification a 2026-07-28 client
    // sends. Against a listen request it ends the subscription; against
    // anything else it cancels a call the shared handlers are running.
    void on2026Notification(
        const QJsonObject &message, const Responder &responder, const QString &sessionId)
    {
        if (message.value("method").toString() != "notifications/cancelled") {
            // The revision removed the rest, and the shared handlers still
            // answer to some of their names.
            qCWarning(mcpServerLog) << "Discarding notification a 2026-07-28 client cannot send:"
                                    << message.value("method").toString();
            responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
            return;
        }

        // A subscription is named by the client's own request id, and the
        // session is the identity the client reported. The pair is not a proof
        // of ownership - two instances of one client share it - but on2026Listen()
        // keeps it unique, so it singles out at most one subscription and the
        // cancel cannot end a second client's by mistake.
        const QJsonValue requestId = message.value("params").toObject().value("requestId");
        const auto listener = std::find_if(m_listeners.begin(),
                                           m_listeners.end(),
                                           [&requestId, &sessionId](const Listener &l) {
                                               return l.subscriptionId == requestId
                                                      && l.session == sessionId;
                                           });

        if (listener == m_listeners.end()) {
            onLegacyData(QJsonDocument(message), responder, sessionId);
            return;
        }

        m_listeners.erase(listener);
        responder.writeStatus(QHttpServerResponse::StatusCode::Accepted);
    }

    // Reports the current state of a task. Terminal states carry the payload the
    // original request would have returned synchronously.
    void on2026TaskGet(const QJsonValue &id, const QString &taskId, const Responder &responder)
    {
        const auto it = m_tasks.find(taskId);
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(jsonRpcError(
                id, InvalidParams, QString("Task with ID \"%1\" not found").arg(taskId))));
            return;
        }

        // While the task waits on input its status is ours, not the tool's.
        if (it->second.inputRequests.isEmpty())
            it->second.update(it->second.callbacks.updateTask(it->second.task), shared_from_this());

        QJsonObject result = toExtensionTask(it->second.task);
        result.insert("resultType", QString(kResultTypeComplete));

        switch (it->second.task.status()) {
        case Schema::TaskStatus::completed: {
            const Result<Schema::CallToolResult> r = it->second.callbacks.result();
            if (r) {
                QJsonObject payload = Schema::toJson(*r);
                payload.insert("resultType", QString(kResultTypeComplete));
                result.insert("result", payload);
            } else {
                result.insert("status", Schema::toString(Schema::TaskStatus::failed));
                result.insert(
                    "error",
                    QJsonObject{{"code", InternalError}, {"message", r.error()}});
            }
            break;
        }
        case Schema::TaskStatus::failed: {
            const Result<Schema::CallToolResult> r = it->second.callbacks.result();
            result.insert(
                "error",
                QJsonObject{
                    {"code", InternalError},
                    {"message", r ? QString("Task failed") : r.error()}});
            break;
        }
        case Schema::TaskStatus::input_required:
            result.insert("inputRequests", it->second.inputRequests);
            break;
        default:
            break;
        }

        responder.write(
            QJsonDocument(QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}}));
    }

    // Delivers the answers a task asked for and lets it continue.
    void on2026TaskUpdate(
        const QJsonValue &id,
        const QString &taskId,
        const QJsonObject &inputResponses,
        const Responder &responder)
    {
        const auto it = m_tasks.find(taskId);
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(jsonRpcError(
                id, InvalidParams, QString("Task with ID \"%1\" not found").arg(taskId))));
            return;
        }

        if (it->second.inputHandlers.empty()) {
            responder.write(QJsonDocument(jsonRpcError(
                id,
                InvalidParams,
                QString("Task with ID \"%1\" is not waiting for input").arg(taskId))));
            return;
        }

        // Only the requests this answers are done with; a tool that asked
        // twice keeps waiting for the other one. Everything is taken off the
        // task before any continuation runs, since one may finish the task or
        // ask again.
        std::vector<std::pair<QJsonObject, std::function<void(const QJsonObject &)>>> answered;
        for (const QString &key : inputResponses.keys()) {
            const auto handler = it->second.inputHandlers.find(key);
            if (handler == it->second.inputHandlers.end())
                continue;
            answered.emplace_back(inputResponses.value(key).toObject(), handler->second);
            it->second.inputHandlers.erase(handler);
            it->second.inputRequests.remove(key);
        }

        if (answered.empty()) {
            responder.write(QJsonDocument(jsonRpcError(
                id,
                InvalidParams,
                QString("None of \"%1\" answers a request task \"%2\" is waiting for: \"%3\"")
                    .arg(inputResponses.keys().join("\", \""),
                         taskId,
                         it->second.inputRequests.keys().join("\", \"")))));
            return;
        }

        // Acknowledge only once the answers are known to reach a handler, and
        // before running them: the tool may finish the task inline.
        responder.write(QJsonDocument(QJsonObject{
            {"jsonrpc", "2.0"},
            {"id", id},
            {"result", QJsonObject{{"resultType", QString(kResultTypeComplete)}}}}));

        if (it->second.inputHandlers.empty())
            it->second.task.status(Schema::TaskStatus::working);

        for (const auto &[response, handler] : answered)
            handler(response);
    }

    void on2026TaskCancel(const QJsonValue &id, const QString &taskId, const Responder &responder)
    {
        const auto it = m_tasks.find(taskId);
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(jsonRpcError(
                id, InvalidParams, QString("Task with ID \"%1\" not found").arg(taskId))));
            return;
        }

        // Cancellation is cooperative: acknowledge the intent either way.
        if (it->second.callbacks.cancelTask)
            (*it->second.callbacks.cancelTask)();
        it->second.task.status(Schema::TaskStatus::cancelled);

        responder.write(QJsonDocument(QJsonObject{
            {"jsonrpc", "2.0"},
            {"id", id},
            {"result", QJsonObject{{"resultType", QString(kResultTypeComplete)}}}}));
    }

    // True when a listener asked for this notification. Request-scoped
    // notifications never travel here: they belong on the response stream of
    // the request that caused them.
    static bool wantsNotification(const V2026::SubscriptionFilter &filter, const QJsonObject &n)
    {
        const QString method = n.value("method").toString();

        if (method == "notifications/tools/list_changed")
            return filter.toolsListChanged().value_or(false);
        if (method == "notifications/prompts/list_changed")
            return filter.promptsListChanged().value_or(false);
        if (method == "notifications/resources/list_changed")
            return filter.resourcesListChanged().value_or(false);
        if (method == "notifications/resources/updated") {
            const QString uri = n.value("params").toObject().value("uri").toString();
            return filter.resourceSubscriptions().value_or(QStringList{}).contains(uri);
        }
        return false;
    }

    void pruneCanceledListeners()
    {
        std::erase_if(m_listeners, [](const Listener &listener) {
            return listener.isCanceled && listener.isCanceled();
        });
    }

    int listenerCount(const QString &sessionId) const
    {
        return Utils::count(m_listeners, [&sessionId](const Listener &l) {
            return l.session == sessionId;
        });
    }

    void deliverTo2026Listeners(const QJsonObject &notification, const QString &sessionId)
    {
        pruneCanceledListeners();

        // Named before anything is sent, and looked up again for each: a send
        // may reach back into m_listeners - a session deleted from under it,
        // another listen - which would erase from or reallocate the vector
        // while it is walked, and would leave a subscription that is gone by
        // then being sent to.
        QList<std::pair<QString, QJsonValue>> targets;
        for (const Listener &listener : m_listeners) {
            // A notification naming a session is for that one alone, as it is
            // for the SSE streams.
            if (!sessionId.isEmpty() && listener.session != sessionId)
                continue;
            if (wantsNotification(listener.filter, notification))
                targets.append({listener.session, listener.subscriptionId});
        }

        for (const auto &[session, subscriptionId] : targets) {
            const auto still = std::find_if(
                m_listeners.cbegin(),
                m_listeners.cend(),
                [&session, &subscriptionId](const Listener &l) {
                    return l.session == session && l.subscriptionId == subscriptionId;
                });
            if (still == m_listeners.cend())
                continue;

            QJsonObject tagged = notification;
            QJsonObject params = tagged.value("params").toObject();
            QJsonObject meta = params.value("_meta").toObject();
            meta.insert(MetaKey::subscriptionId, subscriptionId);
            params.insert("_meta", meta);
            tagged.insert("params", params);
            still->send(QJsonDocument(tagged).toJson(QJsonDocument::Compact));
        }
    }

    void deleteSession(const QString &sessionId);

    void onPing(Schema::RequestId id, const Schema::PingRequest &request, const Responder &responder)
    {
        Q_UNUSED(request);
        responder.write(QJsonDocument(Schema::toJson(Schema::JSONRPCResultResponse().id(id))));
    }

    void onGetTaskPayload(
        Schema::RequestId id,
        const Schema::GetTaskPayloadRequest &request,
        const Responder &responder)
    {
        const auto it = m_tasks.find(request.params().taskId());
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(QString("Task with ID \"%1\" not found")
                                             .arg(request.params().taskId())))
                        .id(id))));
            return;
        }

        Result<Schema::CallToolResult> r = it->second.callbacks.result();

        if (!r) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InternalError)
                                .message(QString("Unknown Error: %1").arg(r.error())))
                        .id(id))));
            return;
        }

        Schema::CallToolResult result = *r;
        result.add_meta(
            "io.modelcontextprotocol/related-task",
            toJson(Schema::RelatedTaskMetadata().taskId(request.params().taskId())));

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onCancelTask(
        Schema::RequestId id, const Schema::CancelTaskRequest &request, const Responder &responder)
    {
        const auto it = m_tasks.find(request.params().taskId());
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(QString("Task with ID \"%1\" not found")
                                             .arg(request.params().taskId())))
                        .id(id))));
            return;
        }

        if (!it->second.callbacks.cancelTask) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(QString("Task with ID \"%1\" cannot be cancelled")
                                             .arg(request.params().taskId())))
                        .id(id))));
            return;
        }

        (*it->second.callbacks.cancelTask)();
        it->second.task.status(Schema::TaskStatus::cancelled);

        auto result = Mcp::Schema::CancelTaskResult()
                          .taskId(request.params().taskId())
                          .status(it->second.task.status())
                          .statusMessage(it->second.task.statusMessage())
                          .createdAt(it->second.task.createdAt())
                          .lastUpdatedAt(it->second.task.lastUpdatedAt())
                          .pollInterval(it->second.task.pollInterval())
                          .ttl(it->second.task.ttl());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onListTasks(
        Schema::RequestId id, const Schema::ListTasksRequest &request, const Responder &responder)
    {
        Schema::ListTasksResult result;
        // Cursor
        auto it = m_tasks.begin();
        if (request.params() && request.params()->cursor())
            it = m_tasks.find(*request.params()->cursor());

        // Pagination
        int count = 0;
        for (; it != m_tasks.end() && count < s_maxPageSize; ++it, ++count) {
            result.addTask(it->second.task);
        }
        if (it != m_tasks.end())
            result.nextCursor(it->first);

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onComplete(
        Schema::RequestId id, const Schema::CompleteRequest &request, const Responder &responder)
    {
        if (!m_completionCallback) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error().code(MethodNotFound).message("Completion not supported"))
                        .id(id))));
            return;
        }

        const auto onResult = [responder, id](Result<Schema::CompleteResult> result) mutable {
            if (result) {
                responder.write(QJsonDocument(makeResponse(id, *result)));
                return;
            }
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InternalError)
                                .message(QString("Unknown Error: %1").arg(result.error())))
                        .id(id))));
        };

        m_completionCallback(request.params(), onResult);
    }

    void onGetTask(
        Schema::RequestId id, const Schema::GetTaskRequest &request, const Responder &responder)
    {
        auto it = m_tasks.find(request.params().taskId());
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(QString("Task with ID \"%1\" not found")
                                             .arg(request.params().taskId())))
                        .id(id))));
            return;
        }

        // Update task information
        auto newTask = it->second.callbacks.updateTask(it->second.task);
        it->second.update(newTask, shared_from_this());

        auto result = Mcp::Schema::GetTaskResult()
                          .taskId(request.params().taskId())
                          .status(it->second.task.status())
                          .createdAt(it->second.task.createdAt())
                          .lastUpdatedAt(it->second.task.lastUpdatedAt())
                          .ttl(it->second.task.ttl())
                          .statusMessage(it->second.task.statusMessage())
                          .pollInterval(it->second.task.pollInterval());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onSimpleToolCall(
        Schema::RequestId id,
        const Schema::CallToolRequest &request,
        const Responder &responder,
        const Server::ToolCallback &cb)
    {
        if (request.params().task().has_value()) {
            qCWarning(mcpServerLog) << "Received call for tool" << request.params().name()
                                    << "with task parameters, but tool does not support tasks";
            responder.write(QJsonDocument(toJson(
                Schema::JSONRPCErrorResponse()
                    .error(
                        Schema::Error()
                            .code(MethodNotFound)
                            .message("Tool does not support tasks: " + request.params().name()))
                    .id(id))));
            return;
        }

        Result<Schema::CallToolResult> r = cb(request.params());

        if (!r) {
            responder.write(QJsonDocument(makeResponse(
                id,
                Schema::CallToolResult().isError(true).content(
                    {Schema::TextContent().text(r.error())}))));
            return;
        }

        responder.write(QJsonDocument(makeResponse(id, *r)));
    }

    void onToolInterfaceCall(
        Schema::RequestId id,
        const QString &sessionId,
        const Schema::CallToolRequest &request,
        const Responder &responder,
        const Server::ToolInterfaceCallback &cb)
    {
        auto sessionInfo = m_sessions.value(sessionId);
        // Should never happen — validated upstream in onRequest() via validateSessionInitialized().
        QTC_ASSERT(sessionInfo, return);
        Schema::ClientCapabilities clientCapabilities = sessionInfo->capabilities;

        ToolInterface toolInterface(
            shared_from_this(), clientCapabilities, request, sessionId, responder);

        m_pendingToolInterfaces.insert(SessionAndRequestId{sessionId, id}, toolInterface.d);

        Result<> r = cb(request.params(), toolInterface);
        if (!r) {
            responder.write(QJsonDocument(makeResponse(
                id,
                Schema::CallToolResult().isError(true).content(
                    {Schema::TextContent().text(r.error())}))));
        }
    }

    void onToolCall(
        Schema::RequestId id,
        const Schema::CallToolRequest &request,
        const Responder &responder,
        const QString sessionId)
    {
        auto toolIt = m_tools.find(request.params().name());

        if (toolIt == m_tools.end()) {
            qCWarning(mcpServerLog) << "Received call for unknown tool:" << request.params().name();

            responder.write(QJsonDocument(toJson(
                Schema::JSONRPCErrorResponse()
                    .error(
                        Schema::Error()
                            .code(MethodNotFound)
                            .message("Invalid Tool:" + request.params().name()))
                    .id(id))));

            return;
        }

        if (const Utils::Result<> ok
            = validateToolArguments(toolIt.value().tool, request.params());
            !ok) {
            qCWarning(mcpServerLog) << "Refusing call for tool" << request.params().name() << ':'
                                    << ok.error();
            responder.write(QJsonDocument(toJson(
                Schema::JSONRPCErrorResponse()
                    .error(Schema::Error().code(InvalidParams).message(ok.error()))
                    .id(id))));
            return;
        }

        qCDebug(mcpServerLog) << "Running tool" << toolIt.key();

        const auto toolExecution = toolIt.value().tool.execution().value_or(Schema::ToolExecution());
        const bool toolNeedsTask = toolExecution.taskSupport()
                                   == Schema::ToolExecution::TaskSupport::required;
        const bool toolSupportsTask = toolExecution.taskSupport()
                                      != Schema::ToolExecution::TaskSupport::forbidden;
        // 2026-07-28 clients never ask for a task per call: declaring the tasks
        // extension says they can accept one, and the server decides. Only the
        // older per-request flag can conflict with a tool that forbids tasks.
        const bool clientAcceptsTask
            = is2026Session(sessionId)
                  ? m_sessions.value(sessionId).value_or(Client{}).supportsTasks
                  : request.params().task().has_value();
        const bool clientDemandsTask = !is2026Session(sessionId)
                                       && request.params().task().has_value();

        if ((toolNeedsTask && !clientAcceptsTask) || (!toolSupportsTask && clientDemandsTask)) {
            qCWarning(mcpServerLog)
                << QString(
                       "Received call for tool %1"
                       "which %2 tasks, but the client %3 a task")
                       .arg(request.params().name())
                       .arg(toolNeedsTask ? QLatin1String("requires") : QLatin1String("does not support"))
                       .arg(clientAcceptsTask ? QLatin1String("accepts") : QLatin1String("does not accept"));
            responder.write(QJsonDocument(toJson(
                Schema::JSONRPCErrorResponse()
                    .error(
                        Schema::Error()
                            .code(MethodNotFound)
                            .message("Tool requires task parameters: " + request.params().name()))
                    .id(id))));
            return;
        }

        std::visit(
            overloaded{
                [&](const Server::ToolCallback &cb) {
                    onSimpleToolCall(id, request, responder, cb);
                },
                [&](const Server::ToolInterfaceCallback &cb) {
                    onToolInterfaceCall(id, sessionId, request, responder, cb);
                },
            },
            toolIt->callback);
    }

    void onToolsList(
        Schema::RequestId id, const Schema::ListToolsRequest &request, const Responder &responder)
    {
        auto it = m_tools.begin();
        if (request.params() && request.params()->cursor())
            it = m_tools.find(*request.params()->cursor());

        Schema::ListToolsResult result;
        int count = 0;
        for (; it != m_tools.end() && count < s_maxPageSize; ++it, ++count) {
            result.addTool(it.value().tool);
        }
        if (it != m_tools.end())
            result.nextCursor(it.key());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onPromptsList(
        Schema::RequestId id, const Schema::ListPromptsRequest &request, const Responder &responder)
    {
        auto it = m_prompts.begin();
        if (request.params() && request.params()->cursor())
            it = m_prompts.find(*request.params()->cursor());

        Schema::ListPromptsResult result;
        int count = 0;
        for (; it != m_prompts.end() && count < s_maxPageSize; ++it, ++count) {
            result.addPrompt(it->prompt);
        }
        if (it != m_prompts.end())
            result.nextCursor(it.key());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onResourcesList(
        Schema::RequestId id,
        const Schema::ListResourcesRequest &request,
        const Responder &responder)
    {
        auto it = m_resources.begin();
        if (request.params() && request.params()->cursor())
            it = m_resources.find(*request.params()->cursor());

        Schema::ListResourcesResult result;
        int count = 0;
        for (; it != m_resources.end() && count < s_maxPageSize; ++it, ++count) {
            result.addResource(it.value().resource);
        }
        if (it != m_resources.end())
            result.nextCursor(it.key());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onReadResource(
        Schema::RequestId id, const Schema::ReadResourceRequest &request, const Responder &responder)
    {
        auto it = m_resources.find(request.params().uri());
        if (it == m_resources.end()) {
            if (m_resourceFallbackCallback) {
                Result<Schema::ReadResourceResult> r = m_resourceFallbackCallback(request.params());

                if (r) {
                    responder.write(QJsonDocument(makeResponse(id, *r)));
                    return;
                }
            }

            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(
                                    QString("Resource \"%1\" not found").arg(request.params().uri())))
                        .id(id))));
            return;
        }

        Result<Schema::ReadResourceResult> r = it->callback(request.params());
        if (!r) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(Schema::Error().code(InternalError).message(r.error()))
                        .id(id))));
            return;
        }

        responder.write(QJsonDocument(makeResponse(id, *r)));
    }

    void onListResourceTemplates(
        Schema::RequestId id,
        const Schema::ListResourceTemplatesRequest &request,
        const Responder &responder)
    {
        auto it = m_resourceTemplates.begin();
        if (request.params() && request.params()->cursor())
            it = m_resourceTemplates.find(*request.params()->cursor());

        Schema::ListResourceTemplatesResult result;
        int count = 0;
        for (; it != m_resourceTemplates.end() && count < s_maxPageSize; ++it, ++count) {
            result.addResourceTemplate(it.value());
        }
        if (it != m_resourceTemplates.end())
            result.nextCursor(it.key());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onGetPrompt(
        Schema::RequestId id, const Schema::GetPromptRequest &request, const Responder &responder)
    {
        auto it = m_prompts.find(request.params().name());
        if (it == m_prompts.end()) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(
                                    QString("Prompt \"%1\" not found").arg(request.params().name())))
                        .id(id))));
            return;
        }

        QList<Schema::PromptMessage> messages = it->callback(
            request.params().arguments().value_or(QMap<QString, QString>{}));

        auto result
            = Schema::GetPromptResult().description(it->prompt.description()).messages(messages);
        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onTaskStatusNotification(
        const Schema::TaskStatusNotification &notification, QString sessionId)
    {
        SessionIdAndTaskId key{sessionId, notification.params().taskId()};
        auto it = m_clientTasks.find(key);
        if (it == m_clientTasks.end()) {
            qCWarning(mcpServerLog)
                << "Received status notification for unknown task ID"
                << notification.params().taskId() << "and session ID" << sessionId;
            return;
        }
        if (!*it)
            return;

        auto task = (*it)->task;
        task.status(notification.params().status());
        task.statusMessage(notification.params().statusMessage());
        task.lastUpdatedAt(notification.params().lastUpdatedAt());
        task.pollInterval(notification.params().pollInterval());

        (*it)->onTaskUpdate(task);
    }

    // The legacy HTTP+SSE transport is all session header, and 2026-07-28 has
    // none of one: a request of that revision arriving on /message would be
    // answered over a stream whose responder cannot carry a status at all, and
    // a listener registered from it would name the synthetic session rather
    // than the stream's, so closing the stream would not reach it. Refuse the
    // revision on this transport rather than half-serve it. Returns true when
    // the request was refused.
    bool refuse2026OnSseTransport(
        const QJsonObject &message, const QString &versionHeader, const Responder &responder)
    {
        const QString version = requestMeta(message).value(MetaKey::protocolVersion).toString();
        if (version != kVersion2026 && versionHeader != kVersion2026)
            return false;

        const QString method = message.value("method").toString();
        qCWarning(mcpServerLog) << "Refusing" << method << "of protocol version" << kVersion2026
                                << "on the HTTP+SSE transport, which carries no such session";
        if (message.contains("id")) {
            // Not writeError(): that one carries the 400 a Streamable HTTP
            // client watches for, and the /message responder drops the status
            // before the route answers 200 anyway.
            responder.write(QJsonDocument(jsonRpcError(
                message.value("id"),
                UnsupportedProtocolVersion,
                QString(
                    "Protocol version \"%1\" is not served over the HTTP+SSE transport; "
                    "use the Streamable HTTP endpoint")
                    .arg(kVersion2026))));
        }
        return true;
    }

    // A request whose header contradicts its body is refused rather than
    // answered under either revision. Both refusals are 400s, which is what a
    // 2026-07-28 client watches for.
    void refuseVersionMismatch(
        const Responder &responder,
        const QJsonObject &message,
        const QString &versionHeader,
        const QString &version)
    {
        const QString method = message.value("method").toString();
        if (!message.contains("id")) {
            qCWarning(mcpServerLog) << "Discarding" << method << "sent with protocol version"
                                    << version << "under header" << versionHeader;
            responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
            return;
        }
        writeError(
            responder,
            message.value("id"),
            HeaderMismatch,
            QString("MCP-Protocol-Version header \"%1\" does not match the requested "
                    "protocol version \"%2\"")
                .arg(versionHeader, version));
    }

    // versionHeader is the MCP-Protocol-Version the HTTP transport carried, and
    // is empty when there was none - or no HTTP request behind this at all.
    void onData(
        const QJsonObject &message,
        const Responder &responder,
        QString sessionId,
        const QString &versionHeader = {})
    {
        // Decided here rather than in the branch that acts on it: the header
        // and the body are held to each other first.
        switch (dialectFor(message, versionHeader)) {
        case Dialect::Mismatch:
            refuseVersionMismatch(
                responder,
                message,
                versionHeader,
                requestMeta(message).value(MetaKey::protocolVersion).toString());
            return;
        case Dialect::Legacy:
            onLegacyData(QJsonDocument(message), responder, sessionId);
            return;
        case Dialect::Revision2026:
        case Dialect::Unsupported:
            // on2026Data() refuses a revision this server does not know.
            on2026Data(responder, message);
            return;
        }
    }

    void onLegacyData(
        const QJsonDocument &jsonDoc, const Responder &responder, QString sessionId)
    {
        const auto request = Schema::fromJson<Schema::JSONRPCRequest>(jsonDoc.object());
        const auto clientRequest = Schema::fromJson<Schema::ClientRequest>(jsonDoc.object());
        if (request && clientRequest) {
            if (m_inspector) {
                auto dataFunc = m_inspector->onRequest(jsonDoc, sessionId);

                Responder responderForInspector;
                responderForInspector.write = [responder, dataFunc](QJsonDocument doc) {
                    dataFunc(doc.toJson(QJsonDocument::Compact));
                    responder.write(doc);
                };
                responderForInspector.writeStatus = [responder, dataFunc](
                                                        QHttpServerResponse::StatusCode code) {
                    if (code == QHttpServerResponse::StatusCode::Ok)
                        dataFunc(QByteArray("OK"));
                    else
                        dataFunc(QString("Error Code: %1").arg(static_cast<int>(code)).toUtf8());
                    responder.writeStatus(code);
                };
                responderForInspector.writeData = [responder, dataFunc](
                                                      const QByteArray &data,
                                                      const char *contentType,
                                                      QHttpServerResponse::StatusCode code) {
                    dataFunc(data);
                    responder.writeData(data, contentType, code);
                };
                responderForInspector.writeSSE = [responder, dataFunc](const QByteArray &data) {
                    dataFunc(data);
                    responder.writeSSE(data);
                };
                responderForInspector.isCanceled = [responder]() { return responder.isCanceled(); };
                onRequest(request->_id, *clientRequest, responderForInspector, sessionId);
                return;
            }

            onRequest(request->_id, *clientRequest, responder, sessionId);
            return;
        }

        const auto clientNotification = Schema::fromJson<Schema::ClientNotification>(
            jsonDoc.object());
        if (clientNotification) {
            qCDebug(mcpServerLog) << "Received JSONRPCNotification:"
                                  << Schema::dispatchValue(*clientNotification);

            if (m_inspector)
                m_inspector->onClientNotification(jsonDoc, sessionId);

            if (std::holds_alternative<Schema::TaskStatusNotification>(*clientNotification)) {
                const auto &notification = std::get<Schema::TaskStatusNotification>(
                    *clientNotification);
                onTaskStatusNotification(notification, sessionId);
            }

            if (std::holds_alternative<Schema::CancelledNotification>(*clientNotification)) {
                const auto &notification = std::get<Schema::CancelledNotification>(
                    *clientNotification);

                if (!notification.params().requestId()) {
                    qCWarning(mcpServerLog)
                        << "Received CancelledNotification without request ID, ignoring";
                    responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
                    return;
                }

                cancelPendingToolInterface(*notification.params().requestId(), sessionId);
            }

            responder.writeStatus(QHttpServerResponse::StatusCode::Accepted);
            return;
        }

        const auto clientResponse = Schema::fromJson<Schema::JSONRPCResponse>(jsonDoc.object());
        if (clientResponse) {
            auto requestId = std::visit(
                [](const auto &resp) -> std::optional<Schema::RequestId> { return resp.id(); },
                *clientResponse);

            if (!requestId) {
                qCWarning(mcpServerLog) << "Received JSONRPC response without ID, rejecting";
                responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            if (std::holds_alternative<QString>(*requestId)) {
                qCWarning(mcpServerLog)
                    << "Received JSONRPC response with non-integer ID, rejecting";
                responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            int id = std::get<int>(*requestId);
            auto it = m_serverRequests.find(id);
            if (it != m_serverRequests.end()) {
                auto callback = it.value();
                m_serverRequests.erase(it);
                callback(*clientResponse);
                return;
            }
        }

        responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
        return;
    }

    bool validateSession(QString sessionId)
    {
        if (sessionId.isEmpty())
            return false;

        // 2026-07-28 carries no session identifier, so the server never hands
        // one of these out and a client presenting one is choosing the revision
        // its request is answered under. The prefix is the server's own.
        if (is2026Session(sessionId))
            return false;

        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end())
            return false;

        if (*it)
            (*it)->lastSeen = QDateTime::currentDateTime();

        return true;
    }

    struct SessionIdAndTaskId
    {
        QString sessionId;
        QString taskId;
        bool operator<(const SessionIdAndTaskId &other) const
        {
            return std::tie(sessionId, taskId) < std::tie(other.sessionId, other.taskId);
        }
    };

    struct SessionAndRequestId
    {
        QString sessionId;
        Schema::RequestId requestId;
        bool operator<(const SessionAndRequestId &other) const
        {
            if (sessionId != other.sessionId)
                return sessionId < other.sessionId;
            if (requestId.index() != other.requestId.index())
                return requestId.index() < other.requestId.index();
            return std::visit(
                [&](const auto &a) {
                    return a < std::get<std::decay_t<decltype(a)>>(other.requestId);
                },
                requestId);
        }
    };

    struct RunningClientTask : public QObject
    {
        using Callback = std::function<void(const Utils::Result<Schema::GetTaskPayloadResult> &)>;
        std::weak_ptr<ServerPrivate> weak;
        QString sessionId;
        Schema::Task task;
        QTimer pollTimer;
        Callback callback;

        RunningClientTask(
            ServerPrivate *server, const QString &sId, const Schema::Task &t, const Callback &cb)
            : weak(server->shared_from_this())
            , sessionId(sId)
            , task(t)
            , callback(cb)
        {
            pollTimer.setSingleShot(false);
            QObject::connect(&pollTimer, &QTimer::timeout, this, &RunningClientTask::poll);

            if (task.pollInterval()) {
                pollTimer.setInterval(std::chrono::milliseconds(*task.pollInterval()));
                pollTimer.start();
            }
        }

        void poll()
        {
            if (auto d = weak.lock()) {
                d->sendServerRequest(
                    Schema::GetTaskRequest().params(
                        Schema::GetTaskRequest::Params().taskId(task.taskId())),
                    sessionId,
                    [this](const Utils::Result<Schema::JSONRPCResponse> &result) {
                        if (!result) {
                            finish(ResultError(
                                QString("Failed to get task status: %1").arg(result.error())));
                        } else {
                            if (std::holds_alternative<Schema::JSONRPCErrorResponse>(*result)) {
                                const auto &errorResponse = std::get<Schema::JSONRPCErrorResponse>(
                                    *result);
                                finish(ResultError(QString("Failed to get task status: %1")
                                                       .arg(errorResponse.error().message())));
                            } else if (std::holds_alternative<Schema::JSONRPCResultResponse>(
                                           *result)) {
                                const auto &resultResponse
                                    = std::get<Schema::JSONRPCResultResponse>(*result);
                                auto taskResult = Schema::fromJson<Schema::GetTaskResult>(
                                    resultResponse.result().additionalProperties());
                                if (!taskResult) {
                                    finish(ResultError(
                                        QString("Failed to parse task status result: %1")
                                            .arg(taskResult.error())));
                                    return;
                                }

                                onTaskUpdate(
                                    Schema::Task()
                                        .taskId(taskResult->taskId())
                                        .status(taskResult->status())
                                        .statusMessage(taskResult->statusMessage())
                                        .createdAt(taskResult->createdAt())
                                        .lastUpdatedAt(taskResult->lastUpdatedAt())
                                        .pollInterval(taskResult->pollInterval())
                                        .ttl(taskResult->ttl()));
                            }
                        }
                    });
            }
        }

        void finish(const Utils::Result<Schema::GetTaskPayloadResult> &payloadResult)
        {
            pollTimer.stop();

            callback(payloadResult);

            if (auto d = weak.lock()) {
                d->removeClientTask(sessionId, task.taskId());
            }
        }

        void onTaskUpdate(const Schema::Task &newTask)
        {
            if (newTask.pollInterval() != task.pollInterval()) {
                if (newTask.pollInterval()) {
                    pollTimer.setInterval(std::chrono::milliseconds(*newTask.pollInterval()));
                    pollTimer.start();
                } else {
                    pollTimer.stop();
                }
            }

            task = newTask;

            if (newTask.status() == Schema::TaskStatus::completed) {
                pollTimer.stop();

                if (auto d = weak.lock()) {
                    d->sendServerRequest(
                        Schema::GetTaskPayloadRequest().params(
                            Schema::GetTaskPayloadRequest::Params().taskId(task.taskId())),
                        sessionId,
                        [this](const Utils::Result<Schema::JSONRPCResponse> &result) {
                            if (!result) {
                                finish(ResultError(
                                    QString("Failed to get task payload: %1").arg(result.error())));
                            } else {
                                if (std::holds_alternative<Schema::JSONRPCErrorResponse>(*result)) {
                                    const auto &errorResponse
                                        = std::get<Schema::JSONRPCErrorResponse>(*result);
                                    finish(ResultError(QString("Failed to get task payload: %1")
                                                           .arg(errorResponse.error().message())));
                                } else if (std::holds_alternative<Schema::JSONRPCResultResponse>(
                                               *result)) {
                                    const auto &resultResponse
                                        = std::get<Schema::JSONRPCResultResponse>(*result);
                                    auto payloadResult
                                        = Schema::fromJson<Schema::GetTaskPayloadResult>(
                                            resultResponse.result().additionalProperties());

                                    finish(payloadResult);
                                }
                            }
                        });
                }
            }
        }
    };

    QMap<SessionIdAndTaskId, RunningClientTask *> m_clientTasks;

    void addClientTask(
        const QString &sessionId,
        const Schema::Task &task,
        const RunningClientTask::Callback &callback)
    {
        std::weak_ptr<ServerPrivate> weak = shared_from_this();

        m_clientTasks.insert(
            SessionIdAndTaskId{sessionId, task.taskId()},
            new RunningClientTask(this, sessionId, task, callback));
    }

    void removeClientTask(const QString &sessionId, const QString &taskId)
    {
        auto it = m_clientTasks.find(SessionIdAndTaskId{sessionId, taskId});
        if (it != m_clientTasks.end()) {
            it.value()->deleteLater();
            m_clientTasks.erase(it);
        }
    }

    struct ToolAndCallback
    {
        Schema::Tool tool;
        std::variant<Server::ToolInterfaceCallback, Server::ToolCallback> callback;
    };
    QMap<QString, ToolAndCallback> m_tools;

    struct PromptAndCallback
    {
        Schema::Prompt prompt;
        Server::PromptCallback callback;
    };
    QMap<QString, PromptAndCallback> m_prompts;

    struct ResourceAndCallback
    {
        Schema::Resource resource;
        Server::ResourceCallback callback;
    };
    QMap<QString, ResourceAndCallback> m_resources;
    Server::ResourceCallback m_resourceFallbackCallback;
    QMap<QString, Schema::ResourceTemplate> m_resourceTemplates;

    QHttpServer m_server;
    std::vector<std::unique_ptr<SseStream>> m_sseStreams;
    std::function<void(QByteArray)> m_ioOutputHandler;

    QTimer m_heartbeatTimer;

    Server::CompletionCallback m_completionCallback;

    using UpdateTaskCallback = std::function<Schema::Task(Schema::Task)>;
    using TaskResultCallback = std::function<Utils::Result<Schema::CallToolResult>()>;
    using CancelTaskCallback = std::function<void()>;

    struct TaskCallbacks
    {
        UpdateTaskCallback updateTask;
        TaskResultCallback result;
        std::optional<CancelTaskCallback> cancelTask;
        int pollingIntervalMs{1000};
    };

    struct TaskAndCallbacks
    {
        TaskAndCallbacks(
            Schema::Task task, TaskCallbacks callbacks, const std::weak_ptr<ServerPrivate> &server)
            : task(std::move(task))
            , callbacks(std::move(callbacks))
        {
            createTimer(server);
        }

        Schema::Task task;
        TaskCallbacks callbacks;
        UniqueDeleteLaterTimer timer;

        // Set while a task-backed tool waits on tasks/update. The task reports
        // these to tasks/get and stops polling the tool for status until the
        // client answers. A tool may ask more than once before the first is
        // answered - elicit() and then sample() - so each key keeps the
        // continuation that belongs to it.
        QJsonObject inputRequests;
        std::map<QString, std::function<void(const QJsonObject &)>> inputHandlers;

        void createTimer(const std::weak_ptr<ServerPrivate> &server)
        {
            if (auto ttl = task.ttl()) {
                timer.reset(new QTimer());
                timer->setSingleShot(true);

                const QDateTime createdAt = QDateTime::fromString(task.createdAt(), Qt::ISODate);
                const QDateTime now = QDateTime::currentDateTime();
                const std::chrono::milliseconds age = now - createdAt;
                const std::chrono::milliseconds remainingTtl = std::chrono::milliseconds(*ttl)
                                                               - age;
                timer->setInterval(remainingTtl);
                QObject::connect(timer.get(), &QTimer::timeout, [server, taskId = task.taskId()]() {
                    if (auto d = server.lock()) {
                        auto it = d->m_tasks.find(taskId);
                        if (it == d->m_tasks.end())
                            return;
                        d->m_tasks.erase(it);
                    }
                });
                timer->start();
            } else {
                timer.reset();
            }
        }

        void update(const Schema::Task &newTask, const std::weak_ptr<ServerPrivate> &server)
        {
            task.lastUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODate));
            task.status(newTask.status());
            task.statusMessage(newTask.statusMessage());
            task.pollInterval(newTask.pollInterval());
            if (task.ttl() != newTask.ttl()) {
                task.ttl(newTask.ttl());
                createTimer(server);
            }
        }
    };

    std::map<QString, TaskAndCallbacks> m_tasks;

    QMap<QString, std::optional<Client>> m_sessions;

    std::vector<Listener> m_listeners;

    // Tool calls suspended on a multi round-trip request, keyed by the
    // requestState token handed to the client. The stored callable owns the
    // suspended ToolInterface and resumes it on the client's retry; each entry
    // carries the timer that drops it if that retry never comes.
    using ResumeInput
        = std::function<void(const QJsonObject &, const Responder &, const QJsonValue &)>;
    struct PendingInput
    {
        ResumeInput resume;
        UniqueDeleteLaterTimer timer;
    };
    std::map<QString, PendingInput> m_pendingInput;

    // Parks a tool call that asked the client for input, under the token the
    // client was handed to retry with.
    void addPendingInput(const QString &token, ResumeInput resume)
    {
        PendingInput pending;
        pending.resume = std::move(resume);
        pending.timer.reset(new QTimer());
        pending.timer->setSingleShot(true);
        pending.timer->setInterval(kPendingInputTtlMs);
        QObject::connect(
            pending.timer.get(),
            &QTimer::timeout,
            [server = weak_from_this(), token] {
                if (auto d = server.lock()) {
                    qCWarning(mcpServerLog)
                        << "Client never retried the request that asked for input; dropping"
                        << token;
                    d->m_pendingInput.erase(token);
                }
            });
        pending.timer->start();

        m_pendingInput.insert_or_assign(token, std::move(pending));
    }

    QMap<int, std::function<void(Schema::JSONRPCResponse)>> m_serverRequests;
    QMap<SessionAndRequestId, std::weak_ptr<ToolInterfacePrivate>> m_pendingToolInterfaces;

    void cancelPendingToolInterface(Schema::RequestId id, const QString &sessionId);

    bool enableCors = false;
    QByteArray authToken;

    // Sessions are handed out to any peer that asks, so their number is a
    // resource the peer controls unless it is bounded here. Only a DELETE ends
    // a streamable-HTTP session, so the bound needs a way to give a slot back;
    // see reclaimIdleSessions().
    static constexpr int kMaxSessions = 64;
    // Subscriptions one session may hold at once. A client needs a stream per
    // set of notifications it wants, which is one for all of them.
    static constexpr int kMaxListenersPerSession = 8;
    static constexpr int kSessionIdleTimeoutSecs = 30 * 60;

    Inspector *m_inspector = nullptr;
};

Server::Server(Schema::Implementation serverInfo)
    : d(std::make_shared<ServerPrivate>(serverInfo))
{
    d->m_server.setMissingHandler(
        &d->m_server, [](const QHttpServerRequest &request, QHttpServerResponder &responder) {
            qCDebug(mcpServerIOLog) << request.url() << request.method() << "not found";
            qCDebug(mcpServerIOLog) << request.headers();
            responder.write(QHttpServerResponse::StatusCode::NotFound);
        });

    d->routeChecked(
        "/sse",
        QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &request, QHttpServerResponder &responder) {
            Q_UNUSED(request);
            if (d->sessionLimitReached()) {
                qCWarning(mcpServerLog)
                    << "Refusing new sse session, session limit reached:" << d->m_sessions.size();
                responder.write(
                    d->corsHeaders({}), QHttpServerResponse::StatusCode::ServiceUnavailable);
                return;
            }
            const QString sessionId = d->createNewSessionId();
            qCDebug(mcpServerLog) << "Starting new sse session with Id " << sessionId;
            d->m_sessions.insert(sessionId, std::nullopt);

            auto stream
                = std::make_unique<SseStream>(d->corsHeaders(sessionId), std::move(responder));

            QObject::connect(
                stream.get(),
                &SseStream::destroyed,
                [d = std::weak_ptr<ServerPrivate>(d), sessionId]() {
                    if (auto dptr = d.lock()) {
                        qCInfo(mcpServerLog) << "SSE session with Id" << sessionId << "ended";
                        dptr->deleteSession(sessionId);
                    }
                });

            stream->sendEndpoint(QString("/message?session=%1").arg(sessionId).toLatin1());
            d->m_sseStreams.emplace_back(std::move(stream));
            return;
        });

    d->routeChecked(
        "/message",
        QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            const QString sessionId = req.query().queryItemValue("session");

            if (!d->validateSession(sessionId)) {
                qCWarning(mcpServerLog) << "Received message for unknown session ID:" << sessionId
                                        << "— responding 404 to force client re-initialization"
                                        << "(known sessions:" << d->m_sessions.keys() << ")";
                responder.write(d->corsHeaders({}), QHttpServerResponse::StatusCode::NotFound);
                return;
            }

            Responder r;
            r.write = [sessionId, this](QJsonDocument json) {
                const QByteArray jsonData = json.toJson(QJsonDocument::Compact);
                qCDebug(mcpServerIOLog).noquote() << "Writing response:" << jsonData;
                d->sendDataTo(jsonData, sessionId);
            };
            r.writeStatus = [](QHttpServerResponder::StatusCode status) { Q_UNUSED(status); };
            r.writeData = [sessionId, this](
                              const QByteArray &data,
                              const char *contentType,
                              QHttpServerResponder::StatusCode status) {
                Q_UNUSED(contentType);
                Q_UNUSED(status);
                qCDebug(mcpServerIOLog).noquote() << "Writing data:" << data;
                d->sendDataTo(data, sessionId);
            };
            r.isCanceled = [] { return false; };
            r.writeSSE = [sessionId, this](QByteArray data) {
                d->sendDataTo(data, sessionId);
            };

            if (const std::optional<QJsonObject> message = parseRequestBody(req.body(), r)) {
                const QString versionHeader
                    = QString::fromUtf8(req.headers().value("mcp-protocol-version"));
                if (d->refuse2026OnSseTransport(*message, versionHeader, r)) {
                    responder.write(d->corsHeaders(sessionId),
                                    QHttpServerResponse::StatusCode::Ok);
                    return;
                }
                d->onData(*message, r, sessionId, versionHeader);
            }

            responder.write(d->corsHeaders(sessionId), QHttpServerResponse::StatusCode::Ok);
        });

    d->routeChecked(
        "/",
        QHttpServerRequest::Method::Options,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            Q_UNUSED(req);
            auto headers = d->corsHeaders({});
            responder.write(headers, QHttpServerResponse::StatusCode::Ok);
        });

    d->routeChecked(
        "/sse",
        QHttpServerRequest::Method::Options,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            Q_UNUSED(req);
            auto headers = d->corsHeaders({});
            responder.write(headers, QHttpServerResponse::StatusCode::Ok);
        });

    d->routeChecked(
        "/message",
        QHttpServerRequest::Method::Options,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            Q_UNUSED(req)
            responder.write(d->corsHeaders({}), QHttpServerResponse::StatusCode::Ok);
        });

    d->routeChecked(
        "/",
        QHttpServerRequest::Method::Delete,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            if (!req.headers().contains("mcp-session-id")) {
                qCWarning(mcpServerLog)
                    << "Received request to delete session without session ID, rejecting";
                responder.write(d->corsHeaders({}), QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            QString sessionId = QString::fromUtf8(req.headers().value("mcp-session-id"));
            if (!d->validateSession(sessionId)) {
                qCWarning(mcpServerLog)
                    << "Received request to delete session with invalid session ID,"
                       "rejecting";
                responder.write(d->corsHeaders({}), QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            qCDebug(mcpServerLog) << "Deleting session" << sessionId;
            d->deleteSession(sessionId);
            responder.write(d->corsHeaders(sessionId), QHttpServerResponse::StatusCode::Ok);
        });

    d->routeChecked(
        "/",
        QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            if (accepts(req, "text/event-stream")) {
                if (req.headers().contains("mcp-session-id")) {
                    qCDebug(mcpServerLog) << "Received SSE connection with session ID:"
                                          << req.headers().value("mcp-session-id");
                    if (!d->validateSession(
                            QString::fromUtf8(req.headers().value("mcp-session-id")))) {
                        qCWarning(mcpServerLog) << "Received SSE connection with invalid session "
                                                   "ID, closing connection";
                        responder.write(QHttpServerResponse::StatusCode::NotFound);
                        return;
                    }
                } else {
                    qCWarning(mcpServerLog)
                        << "Received SSE connection without session ID, closing connection";
                    responder.write(d->corsHeaders({}), QHttpServerResponse::StatusCode::NotFound);
                    return;
                }

                d->m_sseStreams.emplace_back(
                    std::make_unique<SseStream>(
                        d->corsHeaders(QString::fromUtf8(req.headers().value("mcp-session-id"))),
                        std::move(responder)));
                return;
            }

            responder.write(QHttpServerResponse::StatusCode::NotFound);
        });

    d->routeChecked(
        "/",
        QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) -> void {
            auto errorHeaders = d->corsHeaders({});
            errorHeaders.append("content-type", "text/plain");

            // Check header contains "Accept" with only "application/json" and "text/event-stream"
            if (!req.headers().contains("Accept")) {
                responder.write(
                    "Missing Accept header",
                    errorHeaders,
                    QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            /* Protocol Version Header
               If using HTTP, the client MUST include the MCP-Protocol-Version: <protocol-version>
               HTTP header on all subsequent requests to the MCP server, allowing the MCP server to
               respond based on the MCP protocol version.
               For example: MCP-Protocol-Version: 2025-11-25
               The protocol version sent by the client SHOULD be the one negotiated during initialization.
               For backwards compatibility, if the server does not receive an MCP-Protocol-Version
               header, and has no other way to identify the version - for example, by relying on the
               protocol version negotiated during initialization - the server SHOULD assume protocol
               version 2025-03-26.
               If the server receives a request with an invalid or unsupported MCP-Protocol-Version,
               it MUST respond with 400 Bad Request.
            */
            if (req.headers().contains("mcp-protocol-version")
                && !kSupportedProtocolVersions.contains(
                    QString::fromUtf8(req.headers().value("mcp-protocol-version")))) {
                responder.write(
                    "Unsupported MCP protocol version",
                    errorHeaders,
                    QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            // The session the rest of this route needs follows from the body,
            // so it is parsed before the responder that will carry the answer
            // exists; this one carries the answer to a body that is not JSON.
            Responder malformed;
            malformed.writeData = [&responder, errorHeaders](
                                      const QByteArray &data,
                                      const char *,
                                      QHttpServerResponse::StatusCode status) {
                responder.write(data, errorHeaders, status);
            };
            const std::optional<QJsonObject> body = parseRequestBody(req.body(), malformed);
            if (!body)
                return;
            const QJsonObject message = *body;

            // Only initialize establishes a session. Minting one for any other
            // anonymous POST announces a session that is never registered, and
            // 2026-07-28 requests never carry the header at all, so they would
            // mint one per request.
            QString sessionId;
            if (req.headers().contains("mcp-session-id")) {
                sessionId = QString::fromUtf8(req.headers().value("mcp-session-id"));
                if (sessionId.isNull() || !d->validateSession(sessionId)) {
                    qCInfo(mcpServerLog) << "Received request with invalid session ID:"
                                         << req.headers().value("mcp-session-id");

                    responder.write(
                        "Invalid session ID",
                        errorHeaders,
                        QHttpServerResponse::StatusCode::NotFound);
                    return;
                }
            } else if (message.value("method").toString() == QLatin1String("initialize")) {
                if (d->sessionLimitReached()) {
                    qCWarning(mcpServerLog)
                        << "Refusing new session, session limit reached:" << d->m_sessions.size();
                    responder.write(
                        "Too many sessions",
                        errorHeaders,
                        QHttpServerResponse::StatusCode::ServiceUnavailable);
                    return;
                }
                sessionId = d->createNewSessionId();
            }

            qCDebug(mcpServerIOLog).noquote() << "Received request with headers:\n"
                                              << req.headers() << "\nand body:\n"
                                              << req.body() << "\nEnd of body";

            const bool streamMode = accepts(req, "application/json")
                                    && accepts(req, "text/event-stream");

            if (!streamMode) {
                responder.write(
                    "Invalid Accept header",
                    errorHeaders,
                    QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            auto corsHeaders = d->corsHeaders(sessionId);
            Responder r;
            auto http = std::make_shared<QHttpServerResponder>(std::move(responder));

            r.write = [corsHeaders, http](QJsonDocument json) {
                const QByteArray jsonData = json.toJson(QJsonDocument::Compact);
                qCDebug(mcpServerIOLog).noquote() << "Writing response:" << jsonData;

                auto headers = corsHeaders;
                headers.append("content-type", "application/json");
                http->write(jsonData, headers, QHttpServerResponse::StatusCode::Ok);
            };
            r.writeStatus = [corsHeaders, http](QHttpServerResponder::StatusCode status) {
                auto headers = corsHeaders;
                http->write(headers, status);
            };
            r.writeData = [corsHeaders, http](
                              const QByteArray &data,
                              const char *contentType,
                              QHttpServerResponse::StatusCode status) {
                auto headers = corsHeaders;
                headers.append("content-type", contentType);
                http->write(data, headers, status);
            };
            r.isCanceled = [http]() { return http->isResponseCanceled(); };

            r.writeSSE = [sessionId, corsHeaders, http, sseStream = std::shared_ptr<SseStream>()](
                             QByteArray data) mutable {
                if (!sseStream)
                    sseStream = std::make_shared<SseStream>(corsHeaders, http);
                sseStream->sendData(data, sessionId);
            };

            d->onData(
                message,
                r,
                sessionId,
                QString::fromUtf8(req.headers().value("mcp-protocol-version")));
        });
}

Server::~Server() = default;

void Server::setInstructions(const QString &instructions)
{
    d->instructions = instructions;
}

bool Server::bind(QTcpServer *server)
{
    return d->bind(server);
}

QList<QTcpServer *> Server::boundTcpServers() const
{
    return d->m_server.servers();
}

void Server::addTool(const Schema::Tool &tool, const ToolInterfaceCallback &callback)
{
    d->m_tools.insert(tool.name(), ServerPrivate::ToolAndCallback{tool, callback});
    sendNotification(Schema::ToolListChangedNotification{});
}

void Server::addTool(const Schema::Tool &tool, const ToolCallback &callback)
{
    d->m_tools.insert(tool.name(), ServerPrivate::ToolAndCallback{tool, callback});
    sendNotification(Schema::ToolListChangedNotification{});
}

void Server::sendNotification(
    const Schema::ServerNotification &notification, const QString &sessionId)
{
    if (d->m_inspector)
        d->m_inspector->onServerNotification(QJsonDocument(Schema::toJson(notification)), sessionId);

    d->sendNotification(notification, sessionId);
}

Result<std::function<void(QByteArray)>> Server::bindIO(std::function<void(QByteArray)> outputHandler)
{
    if (d->m_ioOutputHandler)
        return ResultError("IO already bound");
    if (!outputHandler)
        return ResultError("Output handler cannot be null");
    d->m_ioOutputHandler = std::move(outputHandler);

    Responder r;
    r.write = [this](QJsonDocument json) {
        if (d->m_ioOutputHandler)
            d->m_ioOutputHandler(json.toJson(QJsonDocument::Compact));
    };
    r.writeStatus = [](QHttpServerResponder::StatusCode status) {
        Q_UNUSED(status);
        // We do not use HTTP status codes in IO mode, so ignore this
    };
    r.writeData = [this](
                      const QByteArray &data,
                      const char *contentType,
                      QHttpServerResponse::StatusCode status) {
        Q_UNUSED(contentType);
        Q_UNUSED(status);
        Q_ASSERT(
            data.contains('\n')
            == false); // We use newlines to separate messages, so data cannot contain newlines
        if (d->m_ioOutputHandler)
            d->m_ioOutputHandler(data);
    };
    r.writeSSE = [this](QByteArray data) { d->m_ioOutputHandler(data); };
    r.isCanceled = [] { return false; };

    return [this, r = std::move(r)](QByteArray data) mutable {
        if (const std::optional<QJsonObject> message = parseRequestBody(data, r))
            d->onData(*message, r, {});
    };
}

void Server::removeTool(const QString &toolName)
{
    if (d->m_tools.remove(toolName) > 0)
        sendNotification(Schema::ToolListChangedNotification{});
}

void Server::addPrompt(const Schema::Prompt &prompt, const PromptCallback &callback)
{
    d->m_prompts.insert(prompt.name(), {prompt, callback});
    sendNotification(Schema::PromptListChangedNotification{});
}

void Server::removePrompt(const QString &promptName)
{
    if (d->m_prompts.remove(promptName) > 0)
        sendNotification(Schema::PromptListChangedNotification{});
}

void Server::addResource(const Schema::Resource &resource, const ResourceCallback &callback)
{
    d->m_resources.insert(resource.uri(), {resource, callback});
    sendNotification(Schema::ResourceListChangedNotification{});
}

void Server::removeResource(const QString &uri)
{
    if (d->m_resources.remove(uri) > 0)
        sendNotification(Schema::ResourceListChangedNotification{});
}

void Server::addResourceTemplate(const Schema::ResourceTemplate &resourceTemplate)
{
    d->m_resourceTemplates.insert(resourceTemplate.name(), resourceTemplate);
    sendNotification(Schema::ResourceListChangedNotification{});
}

void Server::removeResourceTemplate(const QString &name)
{
    if (d->m_resourceTemplates.remove(name) > 0)
        sendNotification(Schema::ResourceListChangedNotification{});
}

void Server::setCompletionCallback(const CompletionCallback &callback)
{
    d->m_completionCallback = callback;
}

void Server::setResourceFallbackCallback(const ResourceCallback &callback)
{
    d->m_resourceFallbackCallback = callback;
}

void Server::setCorsEnabled(bool enabled)
{
    d->enableCors = enabled;
}

void Server::setAuthToken(const QByteArray &token)
{
    d->authToken = token;
}

struct ToolInterfacePrivate
{
    Schema::ClientCapabilities _clientCapabilities;
    std::weak_ptr<ServerPrivate> _server;
    Schema::CallToolRequest _initialRequest;
    QString _sessionId;
    Responder _responder;
    UniqueDeleteLaterTimer _longRunningToolTimer;
    mutable ToolInterface::CancelTaskCallback _cancelTaskCallback;

    bool _isFinished = false;
    bool _isTask = false;
    QString _taskId;
    bool _supportsTasks = false;

    // Explicit constructor (C++17 has no parenthesized aggregate initialization).
    ToolInterfacePrivate(
        Schema::ClientCapabilities clientCapabilities,
        std::weak_ptr<ServerPrivate> server,
        Schema::CallToolRequest initialRequest,
        QString sessionId,
        Responder responder)
        : _clientCapabilities(std::move(clientCapabilities))
        , _server(std::move(server))
        , _initialRequest(std::move(initialRequest))
        , _sessionId(std::move(sessionId))
        , _responder(std::move(responder))
    {
        // A 2026-07-28 session entry belongs to whichever client wrote it last,
        // so everything this call needs from it is taken here, while the request
        // that wrote it is still being dispatched.
        if (auto server = _server.lock()) {
            const auto session = server->m_sessions.value(_sessionId);
            _supportsTasks = session && session->supportsTasks;
        }
    }

    ~ToolInterfacePrivate()
    {
        if (!_isFinished && _cancelTaskCallback)
            _cancelTaskCallback();
    }

    void removeFromPending()
    {
        if (auto server = _server.lock())
            server->m_pendingToolInterfaces.remove(
                ServerPrivate::SessionAndRequestId{_sessionId, _initialRequest.id()});
    }

    // The pending entry is keyed by the id of the request being served, so it
    // moves with the id: removeFromPending() reads the id as it is by then, and
    // a notifications/cancelled names the id the client itself used.
    void takeOverRequestId(const Schema::RequestId &id)
    {
        auto server = _server.lock();
        if (!server) {
            _initialRequest.id(id);
            return;
        }
        const auto key = [this] {
            return ServerPrivate::SessionAndRequestId{_sessionId, _initialRequest.id()};
        };
        const auto pending = server->m_pendingToolInterfaces.take(key());
        _initialRequest.id(id);
        if (!pending.expired())
            server->m_pendingToolInterfaces.insert(key(), pending);
    }

    // Parks a request the tool made from inside a task. There is no open
    // request to answer, so it surfaces through tasks/get instead and the
    // client replies with tasks/update.
    Utils::Result<> suspendTaskForInput(
        const QString &key,
        const QJsonObject &inputRequest,
        const std::function<void(const QJsonObject &)> &onResponse)
    {
        auto server = _server.lock();
        if (!server)
            return Utils::ResultError(QString("Task is no longer available"));

        const auto it = server->m_tasks.find(_taskId);
        if (it == server->m_tasks.end())
            return Utils::ResultError(QString("Task is no longer available"));

        // The key is what tasks/update answers under, so two requests of one
        // kind cannot both be outstanding. Refusing the second tells the tool;
        // taking its place would drop the first one's continuation.
        if (it->second.inputHandlers.count(key)) {
            return Utils::ResultError(
                QString("A %1 request of this task is already waiting for an answer").arg(key));
        }

        it->second.inputRequests.insert(key, inputRequest);
        it->second.inputHandlers[key] = onResponse;
        it->second.task.status(Schema::TaskStatus::input_required);
        return Utils::ResultOk;
    }

    void cancel()
    {
        if (_isFinished)
            return;
        _isFinished = true;
        removeFromPending();

        if (_cancelTaskCallback) {
            // Timer/polling path: stop timer and invoke user's cancel callback.
            // The HTTP connection is SSE; the client initiated cancellation so no
            // final event is needed.
            _cancelTaskCallback();
            return;
        }

        if (_isTask && !_taskId.isEmpty()) {
            // Push-based task path: cancel in m_tasks. The CreateTaskResult
            // response was already sent when startTask() was called.
            if (auto server = _server.lock()) {
                auto it = server->m_tasks.find(_taskId);
                if (it != server->m_tasks.end()) {
                    if (it->second.callbacks.cancelTask)
                        (*it->second.callbacks.cancelTask)();
                    it->second.task.status(Schema::TaskStatus::cancelled);
                }
            }
            return;
        }

        // Non-task async: the HTTP connection is still open waiting for the
        // response, so send a JSON-RPC error.
        _responder.write(QJsonDocument(
            Schema::toJson(
                Schema::JSONRPCErrorResponse()
                    .error(Schema::Error().code(RequestCancelled).message("Request cancelled"))
                    .id(_initialRequest.id()))));
    }

    void finish(const Utils::Result<Schema::CallToolResult> &result, bool isLongRunningTask = false)
    {
        if (isFinished() || (_isTask && !isLongRunningTask)) {
            qCWarning(mcpServerLog)
                << "Attempted to finish a tool that is already finished or started a task";
            return;
        }

        _isFinished = true;
        removeFromPending();

        if (!result) {
            _responder.write(QJsonDocument(makeResponse(
                _initialRequest.id(),
                Schema::CallToolResult().isError(true).content(
                    {Schema::TextContent().text(result.error())}))));
            return;
        }

        _responder.write(QJsonDocument(makeResponse(_initialRequest.id(), *result)));
    }
    bool isFinished() const
    {
        if (_isFinished) {
            return true;
        }

        // Check if the task exists / its status == completed.
        if (_isTask) {
            if (_longRunningToolTimer)
                return false; // If we have a timer, the task is still running

            if (auto serverPrivate = _server.lock()) {
                auto it = serverPrivate->m_tasks.find(_taskId);
                if (it == serverPrivate->m_tasks.end()
                    || it->second.task.status() == Schema::TaskStatus::completed
                    || it->second.task.status() == Schema::TaskStatus::cancelled
                    || it->second.task.status() == Schema::TaskStatus::failed) {
                    return true;
                }
            }
        }

        return false;
    }
};

void ServerPrivate::cancelPendingToolInterface(Schema::RequestId id, const QString &sessionId)
{
    auto it = m_pendingToolInterfaces.find(SessionAndRequestId{sessionId, id});
    if (it == m_pendingToolInterfaces.end())
        return;
    auto tiPrivate = it.value().lock();
    m_pendingToolInterfaces.erase(it);
    if (tiPrivate)
        tiPrivate->cancel();
}

void ServerPrivate::deleteSession(const QString &sessionId)
{
    qCDebug(mcpServerLog) << "Deleting session ID" << sessionId;
    m_sessions.remove(sessionId);

    // Cancel and remove any pending tool interfaces for this session.
    for (auto it = m_pendingToolInterfaces.begin(); it != m_pendingToolInterfaces.end();) {
        if (it.key().sessionId == sessionId) {
            auto tiPrivate = it.value().lock();
            it = m_pendingToolInterfaces.erase(it);
            if (tiPrivate)
                tiPrivate->cancel();
        } else {
            ++it;
        }
    }

    // A subscription outlives the request that opened it, so reclaiming the
    // session has to end it too: nothing else would, and the deterministic
    // 2026-07-28 identifier hands the next client of that name the stale one.
    //
    // Ended where the client can hear it rather than by dropping the entry: the
    // revision has the server send notifications/cancelled naming the listen
    // request to terminate its stream, and says so for stdio in particular,
    // where no closed connection would say it instead. A stream that simply
    // stops delivering is indistinguishable from a quiet server.
    //
    // Named before anything is sent, and looked up again for each, for the
    // reason deliverTo2026Listeners() gives: a send may reach back into
    // m_listeners and erase from or reallocate the vector while it is walked.
    QList<QJsonValue> subscriptions;
    for (const Listener &listener : m_listeners) {
        if (listener.session == sessionId)
            subscriptions.append(listener.subscriptionId);
    }

    for (const QJsonValue &subscriptionId : subscriptions) {
        const auto still = std::find_if(
            m_listeners.cbegin(),
            m_listeners.cend(),
            [&sessionId, &subscriptionId](const Listener &l) {
                return l.session == sessionId && l.subscriptionId == subscriptionId;
            });
        if (still == m_listeners.cend())
            continue;

        const auto ended = V2026::CancelledNotification().params(
            V2026::CancelledNotificationParams()
                .requestId(toRequestId(subscriptionId))
                .reason(QString("The session this subscription was opened for was reclaimed")));
        still->send(QJsonDocument(V2026::toJson(ended)).toJson(QJsonDocument::Compact));
    }
    std::erase_if(m_listeners, [&sessionId](const Listener &l) { return l.session == sessionId; });

    if (m_inspector)
        m_inspector->onSessionEnded(sessionId);
}

ToolInterface::ToolInterface(
    std::weak_ptr<ServerPrivate> serverPrivate,
    const Schema::ClientCapabilities &clientCaps,
    const Schema::CallToolRequest &request,
    const QString &sessionId,
    const Responder &responder)
    : d(std::make_shared<ToolInterfacePrivate>(clientCaps, serverPrivate, request, sessionId, responder))
{}

ToolInterface::~ToolInterface() {}

const Schema::ClientCapabilities &ToolInterface::clientCapabilities() const
{
    return d->_clientCapabilities;
}

// Suspends a tool call under the multi round-trip request pattern: the client
// is told what input is needed and retries the original request with the
// answer, at which point the stored continuation resumes the tool. This
// replaces the server-initiated requests that 2026-07-28 removed.
static void suspendForInput(
    ServerPrivate &server,
    const std::shared_ptr<ToolInterfacePrivate> &tool,
    const QString &key,
    const QJsonObject &inputRequest,
    const std::function<void(const QJsonObject &)> &onResponse)
{
    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);

    server.addPendingInput(
        token,
        [tool, key, onResponse](
            const QJsonObject &responses, const Responder &responder, const QJsonValue &requestId) {
            // The retry is a new JSON-RPC request, so the tool must answer on
            // its responder and under its id.
            tool->_responder = responder;
            tool->takeOverRequestId(toRequestId(requestId));
            onResponse(responses.value(key).toObject());
        });

    tool->_responder.write(QJsonDocument(QJsonObject{
        {"jsonrpc", "2.0"},
        {"id", toJsonValue(tool->_initialRequest.id())},
        {"result",
         QJsonObject{
             {"resultType", QString(kResultTypeInputRequired)},
             {"requestState", token},
             {"inputRequests", QJsonObject{{key, inputRequest}}}}}}));
}

void ToolInterface::elicit(
    const Schema::ElicitRequestParams &params, const ElicitResultCallback &cb) const
{
    if (d->isFinished()) {
        qCWarning(mcpServerLog) << "A finished tool should not ask for elicitation.";
        cb(Utils::ResultError("Tool is already finished"));
        return;
    }

    const bool wantsTask
        = std::visit([](const auto &p) -> bool { return p.task().has_value(); }, params);

    if (!d->_clientCapabilities.elicitation()) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit, but client does not "
                                   "support elicitation";
        cb(Utils::ResultError("Client does not support elicitation"));
        return;
    }

    const bool hasElicitSupport = d->_clientCapabilities.elicitation().has_value();
    const bool hasFormSupportObject = hasElicitSupport
                                      && d->_clientCapabilities.elicitation()->form().has_value();
    const bool hasUrlSupportObject = hasElicitSupport
                                     && d->_clientCapabilities.elicitation()->url().has_value();
    const bool hasFormOrUrlSupport = hasFormSupportObject || hasUrlSupportObject;
    const bool hasFormSupport
        = hasFormSupportObject
          || !hasFormOrUrlSupport; // Fallback if no support is declared, but the elicitation capability is present

    if (std::holds_alternative<Schema::ElicitRequestFormParams>(params) && !hasFormSupport) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit with form parameters, "
                                   "but client does not support elicitation forms";
        cb(Utils::ResultError("Client does not support elicitation forms"));
        return;
    } else if (std::holds_alternative<Schema::ElicitRequestURLParams>(params) && !hasUrlSupportObject) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit with URL parameters, "
                                   "but client does not support elicitation URLs";
        cb(Utils::ResultError("Client does not support elicitation URLs"));
        return;
    }

    // Task-flavoured elicitation is a 2025-11-25 capability. Under 2026-07-28
    // it is the tasks extension that governs it, and the tool call's own task
    // carries the request, so the per-request flag has no meaning there.
    if (wantsTask && !is2026Session(d->_sessionId)
        && (!d->_clientCapabilities.tasks() || !d->_clientCapabilities.tasks()->requests()
            || !d->_clientCapabilities.tasks()->requests()->elicitation()
            || !d->_clientCapabilities.tasks()->requests()->elicitation()->create())) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit with task parameters, "
                                   "but client does not support task elicitation";
        cb(Utils::ResultError("Client does not support task elicitation"));
        return;
    }

    if (auto serverPrivate = d->_server.lock()) {
        if (is2026Session(d->_sessionId)) {
            const QJsonObject request{
                {"method", "elicitation/create"},
                {"params", withoutLegacyTask(Schema::toJson(params))}};
            const auto onResponse = [cb](const QJsonObject &response) {
                if (auto parsed = Schema::fromJson<Schema::ElicitResult>(response))
                    cb(*parsed);
                else
                    cb(Utils::ResultError(
                        "Failed to parse elicit result from client: " + parsed.error()));
            };

            if (d->_isTask) {
                if (const Utils::Result<> suspended
                    = d->suspendTaskForInput("elicitation", request, onResponse);
                    !suspended) {
                    cb(Utils::ResultError(suspended.error()));
                }
                return;
            }

            suspendForInput(*serverPrivate, d, "elicitation", request, onResponse);
            return;
        }

        serverPrivate->sendServerRequest(
            Schema::ElicitRequest().params(params),
            d->_sessionId,
            [d = this->d, cb, wantsTask](const Schema::JSONRPCResponse &response) {
                Utils::Result<Schema::ElicitResult> r;

                if (std::holds_alternative<Schema::JSONRPCResultResponse>(response)) {
                    const auto &jsonRpcResult = std::get<Schema::JSONRPCResultResponse>(response);

                    auto elicitResult = Schema::fromJson<Schema::ElicitResult>(
                        jsonRpcResult.result().additionalProperties());

                    if (elicitResult) {
                        cb(Utils::Result<Schema::ElicitResult>(elicitResult));
                        return;
                    }

                    if (wantsTask) {
                        auto elicitTaskResult = Schema::fromJson<Schema::CreateTaskResult>(
                            jsonRpcResult.result().additionalProperties());

                        if (!elicitTaskResult) {
                            qCWarning(mcpServerLog)
                                << "Failed to parse elicit task result from client:"
                                << elicitTaskResult.error();
                            cb(Utils::ResultError(
                                "Failed to parse elicit task result from client: "
                                + elicitTaskResult.error()));
                            return;
                        }

                        if (auto serverPrivate = d->_server.lock()) {
                            serverPrivate->addClientTask(
                                d->_sessionId,
                                elicitTaskResult->task(),
                                [cb](const Utils::Result<Schema::GetTaskPayloadResult> &taskResult) {
                                    cb(Schema::fromJson<Schema::ElicitResult>(
                                        Schema::toJson(*taskResult)));
                                });
                        }

                        return;
                    }

                    qCWarning(mcpServerLog)
                        << "Failed to parse elicit result from client:" << elicitResult.error();
                    cb(Utils::ResultError(
                        "Failed to parse elicit result from client: " + elicitResult.error()));
                    return;
                }

                const auto &error = std::get<Schema::JSONRPCErrorResponse>(response);
                qCWarning(mcpServerLog)
                    << "Received elicit error from client:" << Schema::toJson(error.error());
                r = Utils::ResultError("Client error: " + error.error().message());
                cb(Utils::ResultError("Client error: " + error.error().message()));
            });
    } else {
        qCWarning(mcpServerLog) << "elicit() called after server shutdown; "
                                   "resolving callback with error";
        cb(Utils::ResultError("Server is shutting down"));
    }
}

void ToolInterface::sample(
    const Schema::CreateMessageRequestParams &params, const SampleResultCallback &cb) const
{
    if (d->isFinished()) {
        qCWarning(mcpServerLog) << "A finished tool should not ask for sampling.";
        cb(Utils::ResultError("Tool is already finished"));
        return;
    }

    const bool wantsTask = params.task().has_value();
    if (wantsTask) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit with task parameters, "
                                   "which is not yet supported";
        cb(Utils::ResultError("Elicit does not support tasks"));
        return;
    }
    if (!d->_clientCapabilities.sampling()) {
        qCWarning(mcpServerLog) << "Caller attempted to sample, but client does not "
                                   "support sampling";
        cb(Utils::ResultError("Client does not support sampling"));
        return;
    }

    if (auto serverPrivate = d->_server.lock()) {
        if (is2026Session(d->_sessionId)) {
            const QJsonObject request{
                {"method", "sampling/createMessage"},
                {"params", withoutLegacyTask(Schema::toJson(params))}};
            const auto onResponse = [cb](const QJsonObject &response) {
                if (auto parsed = Schema::fromJson<Schema::CreateMessageResult>(response))
                    cb(*parsed);
                else
                    cb(Utils::ResultError(
                        "Failed to parse sample result from client: " + parsed.error()));
            };

            if (d->_isTask) {
                if (const Utils::Result<> suspended
                    = d->suspendTaskForInput("sampling", request, onResponse);
                    !suspended) {
                    cb(Utils::ResultError(suspended.error()));
                }
                return;
            }

            suspendForInput(*serverPrivate, d, "sampling", request, onResponse);
            return;
        }

        serverPrivate->sendServerRequest(
            Schema::CreateMessageRequest().params(params),
            d->_sessionId, // sessionId is not needed for samples as they are one-off and not associated with a task
            [cb](const Schema::JSONRPCResponse &response) {
                Utils::Result<Schema::CreateMessageResult> r;

                if (std::holds_alternative<Schema::JSONRPCResultResponse>(response)) {
                    const auto &jsonRpcResult = std::get<Schema::JSONRPCResultResponse>(response);

                    auto createMessageResult = Schema::fromJson<Schema::CreateMessageResult>(
                        jsonRpcResult.result().additionalProperties());

                    if (!createMessageResult) {
                        qCWarning(mcpServerLog) << "Failed to parse sample result from client:"
                                                << createMessageResult.error();
                        cb(Utils::ResultError(
                            "Failed to parse sample result from client: "
                            + createMessageResult.error()));
                        return;
                    }

                    qCDebug(mcpServerLog) << "Received sample result from client:"
                                          << Schema::toJson(jsonRpcResult.result());

                    cb(Utils::Result<Schema::CreateMessageResult>(createMessageResult));
                    return;
                }

                const auto &error = std::get<Schema::JSONRPCErrorResponse>(response);
                qCWarning(mcpServerLog)
                    << "Received sample error from client:" << Schema::toJson(error.error());
                r = Utils::ResultError("Client error: " + error.error().message());
                cb(Utils::ResultError("Client error: " + error.error().message()));
            });
    } else {
        qCWarning(mcpServerLog) << "sample() called after server shutdown; "
                                   "resolving callback with error";
        cb(Utils::ResultError("Server is shutting down"));
    }
}

void ToolInterface::notify(const Schema::ServerNotification &notification) const
{
    if (d->isFinished()) {
        qCWarning(mcpServerLog) << "A finished tool should not send notifications";
        return;
    }

    if (auto serverPrivate = d->_server.lock())
        serverPrivate->sendNotification(notification, d->_sessionId);
    else
        qCWarning(mcpServerLog) << "notify() called after server shutdown; notification dropped";
}

void ToolInterface::finish(const Utils::Result<Schema::CallToolResult> &result) const
{
    d->finish(result);
}

Utils::Result<ToolInterface::TaskProgressNotify> ToolInterface::startTask(
    std::optional<int> pollingIntervalMs,
    const UpdateTaskCallback &onUpdateTask,
    const TaskResultCallback &onResultCallback,
    const std::optional<CancelTaskCallback> &onCancelTaskCallback,
    std::optional<int> ttlMs,
    std::optional<Schema::ProgressToken> progressToken) const
{
    if (d->isFinished() || d->_isTask) {
        qCWarning(mcpServerLog)
            << "Attempted to finish a tool that is already finished or started a task";
        return Utils::ResultError(
            "Attempted to start a task for a tool that is already finished or started a "
            "task");
    }

    // The update and result callbacks are mandatory and stored as std::function.
    // Calling an empty std::function throws std::bad_function_call, which would
    // crash the server either synchronously in the polling timer or on the next
    // tasks/get or tasks/result request from the client. Fail fast here.
    if (!onUpdateTask)
        return Utils::ResultError("onUpdateTask callback must not be empty");
    if (!onResultCallback)
        return Utils::ResultError("onResultCallback callback must not be empty");

    // 2025-11-25 clients ask for a task per request; 2026-07-28 clients instead
    // declare the tasks extension once and let the server decide.
    const bool tasksNegotiated = is2026Session(d->_sessionId)
                                     ? d->_supportsTasks
                                     : d->_initialRequest.params().task().has_value();

    if (!tasksNegotiated) {
        if (!pollingIntervalMs) {
            qCWarning(mcpServerLog)
                << "Attempted to start a task without providing a polling interval for a client "
                   "that does not support server-initiated tasks";
            return Utils::ResultError(
                "Polling interval must be provided for clients that do not support "
                "server-initiated tasks");
        }

        d->_responder.write = [&writeSSE = d->_responder.writeSSE](QJsonDocument json) {
            const QByteArray data = json.toJson(QJsonDocument::Compact);
            writeSSE(data);
        };

        d->_longRunningToolTimer.reset(new QTimer());
        d->_longRunningToolTimer->setSingleShot(false);
        d->_longRunningToolTimer->setInterval(*pollingIntervalMs);
        QObject::connect(
            d->_longRunningToolTimer.get(),
            &QTimer::timeout,
            [self = *this, pcounter = 0, onUpdateTask, onResultCallback, progressToken]() mutable {
                if (self.d->_responder.isCanceled()) {
                    self.d->_longRunningToolTimer->stop();
                    self.d->_longRunningToolTimer.reset();
                    return;
                }

                auto task = onUpdateTask(
                    Schema::Task()
                        .pollInterval(self.d->_longRunningToolTimer->interval())
                        .status(Schema::TaskStatus::working));

                if (task.status() == Schema::TaskStatus::input_required)
                    return;

                if (task.status() == Schema::TaskStatus::working) {
                    if (progressToken) {
                        Schema::ServerNotification notification =
                            Schema::ProgressNotification().params(
                                Schema::ProgressNotificationParams()
                                    .progress(pcounter++)
                                    .message(task.statusMessage().value_or(
                                        QString("Task is working...")))
                                    .progressToken(*progressToken));

                        self.d->_responder.writeSSE(QJsonDocument(Schema::toJson(notification))
                                                        .toJson(QJsonDocument::Compact));
                    }
                    return;
                }

                if (task.status() == Schema::TaskStatus::completed
                    || task.status() == Schema::TaskStatus::failed
                    || task.status() == Schema::TaskStatus::cancelled) {
                    auto result = onResultCallback();
                    if (!result) {
                        qCWarning(mcpServerLog) << "Task completed with error:" << result.error();
                    }

                    self.d->finish(result, true);
                }

                self.d->_longRunningToolTimer->stop();
                self.d->_longRunningToolTimer.reset();
            });

        d->_cancelTaskCallback = [self = d.get(), userCb = onCancelTaskCallback]() {
            if (userCb)
                (*userCb)();

            if (self->_longRunningToolTimer) {
                self->_longRunningToolTimer->stop();
                self->_longRunningToolTimer.reset();
            }
        };

        d->_longRunningToolTimer->start();
        d->_isTask = true;
        return nullptr;
    }

    d->_isTask = true;

    if (auto serverPrivate = d->_server.lock()) {
        const bool is2026 = is2026Session(d->_sessionId);
        auto taskId = QUuid::createUuid().toString();
        auto task = Schema::Task()
                        .taskId(taskId)
                        .ttl(ttlMs)
                        .status(Schema::TaskStatus::working)
                        .pollInterval(
                            is2026 ? pollingIntervalMs.value_or(kDefaultTaskPollIntervalMs)
                                   : pollingIntervalMs)
                        .createdAt(QDateTime::currentDateTime().toString(Qt::ISODate))
                        .lastUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODate));

        const auto callbacks
            = ServerPrivate::TaskCallbacks{onUpdateTask, onResultCallback, onCancelTaskCallback};
        serverPrivate->m_tasks.insert(
            std::make_pair(taskId, ServerPrivate::TaskAndCallbacks(task, callbacks, d->_server)));
        d->_taskId = taskId;

        QJsonObject json;
        if (is2026) {
            QJsonObject result = toExtensionTask(task);
            result.insert("resultType", QString(kResultTypeTask));
            json = QJsonObject{
                {"jsonrpc", "2.0"},
                {"id", toJsonValue(d->_initialRequest.id())},
                {"result", result}};
        } else {
            json = Schema::toJson(
                Schema::JSONRPCResultResponse()
                    .id(d->_initialRequest.id())
                    .result(
                        Schema::Result().additionalProperties(
                            Schema::toJson(Schema::CreateTaskResult().task(task)))));
        }

        d->_responder.write(QJsonDocument(json));

        auto notifyTaskUpdate =
            [weak = d->_server, taskId, sessionId = d->_sessionId](
                const Schema::TaskStatus &status,
                const std::optional<QString> &statusMessage,
                const std::optional<int> &ttl) {
                if (auto d = weak.lock()) {
                    auto it = d->m_tasks.find(taskId);
                    if (it == d->m_tasks.end()) {
                        qCWarning(mcpServerLog)
                            << "Attempted to update non-existent task with ID" << taskId;
                        return;
                    }
                    auto task = it->second.task;
                    it->second
                        .update(task.status(status).statusMessage(statusMessage).ttl(ttl), weak);

                    auto params = Schema::TaskStatusNotificationParams()
                                      .taskId(taskId)
                                      .status(it->second.task.status())
                                      .createdAt(it->second.task.createdAt())
                                      .lastUpdatedAt(it->second.task.lastUpdatedAt())
                                      .pollInterval(*it->second.task.pollInterval())
                                      .statusMessage(*it->second.task.statusMessage())
                                      .ttl(*it->second.task.ttl());

                    d->sendNotification(Schema::TaskStatusNotification().params(params), sessionId);
                }
            };

        return notifyTaskUpdate;
    }

    return Utils::ResultError("Failed to start task: Server instance no longer exists");
}

void Server::setInspector(Inspector *inspector)
{
    d->m_inspector = inspector;
}

} // namespace Mcp
