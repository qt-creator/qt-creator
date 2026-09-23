// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <mcp/server/mcpserver.h>

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <optional>

using namespace Mcp;
using namespace Utils;

namespace {

constexpr auto kVersion2026 = "2026-07-28";
constexpr auto kVersion2025 = "2025-11-25";
constexpr auto kMetaVersion = "io.modelcontextprotocol/protocolVersion";
constexpr auto kMetaCapabilities = "io.modelcontextprotocol/clientCapabilities";
constexpr auto kMetaClientInfo = "io.modelcontextprotocol/clientInfo";
constexpr auto kMetaServerInfo = "io.modelcontextprotocol/serverInfo";
constexpr auto kMetaSubscriptionId = "io.modelcontextprotocol/subscriptionId";
constexpr auto kTasksExtension = "io.modelcontextprotocol/tasks";

// The server speaks JSON-RPC over whatever bindIO() is handed, which is enough
// to drive both revisions without a socket: every request below is fed in as a
// byte array and every response, notification and stream event arrives in
// Harness::out in the order the server wrote it.
class Harness
{
public:
    Harness()
        : server(Schema::Implementation().name("tst_mcpprotocol").version("1.0").title("Test"))
    {
        const Result<std::function<void(QByteArray)>> bound = server.bindIO(
            [this](const QByteArray &data) { out.append(QJsonDocument::fromJson(data).object()); });
        QTC_ASSERT(bound, return);
        m_send = *bound;
    }

    // Feeds one message in and returns everything the server wrote in response.
    QList<QJsonObject> send(const QJsonObject &message)
    {
        out.clear();
        m_send(QJsonDocument(message).toJson(QJsonDocument::Compact));
        return out;
    }

    // Convenience for the common case of a request answered by exactly one
    // message; returns an empty object (which fails every check below) if the
    // server said anything else.
    QJsonObject sendOne(const QJsonObject &message)
    {
        const QList<QJsonObject> replies = send(message);
        return replies.size() == 1 ? replies.first() : QJsonObject{};
    }

    Mcp::Server server;
    QList<QJsonObject> out;

private:
    std::function<void(QByteArray)> m_send;
};

QJsonObject legacyRequest(int id, const QString &method, const QJsonObject &params = {})
{
    return QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}};
}

// A 2026-07-28 request states its protocol version, its capabilities and its
// identity in the params' _meta rather than in a header or a handshake.
QJsonObject request2026(
    int id,
    const QString &method,
    QJsonObject params = {},
    const QJsonObject &clientCapabilities = {},
    const QString &version = QString::fromLatin1(kVersion2026),
    const QJsonObject &clientInfo = QJsonObject{{"name", "tst_mcpprotocol"}, {"version", "1.0"}})
{
    QJsonObject meta = params.value("_meta").toObject();
    meta.insert(kMetaVersion, version);
    meta.insert(kMetaCapabilities, clientCapabilities);
    meta.insert(kMetaClientInfo, clientInfo);
    params.insert("_meta", meta);

    QJsonObject message{{"jsonrpc", "2.0"}, {"method", method}, {"params", params}};
    if (id >= 0)
        message.insert("id", id);
    return message;
}

QJsonObject result(const QJsonObject &response)
{
    return response.value("result").toObject();
}

int errorCode(const QJsonObject &response)
{
    return response.value("error").toObject().value("code").toInt();
}

Schema::Tool namedTool(const QString &name)
{
    return Schema::Tool().name(name).title(name).description(name).inputSchema(
        Schema::Tool::InputSchema());
}

QJsonObject callTool(int id, const QString &name, const QJsonObject &clientCapabilities)
{
    return request2026(id, "tools/call", QJsonObject{{"name", name}}, clientCapabilities);
}

QJsonObject namedCallTool(int id, const QString &name, const QString &clientName)
{
    return request2026(
        id,
        "tools/call",
        QJsonObject{{"name", name}},
        {},
        QString::fromLatin1(kVersion2026),
        QJsonObject{{"name", clientName}, {"version", "1.0"}});
}

// Nothing obliges a 2026-07-28 client to name itself, and a client that does not
// shares its synthetic session with every other client that does not.
QJsonObject anonymousCallTool(int id, const QString &name, const QJsonObject &clientCapabilities)
{
    return request2026(
        id,
        "tools/call",
        QJsonObject{{"name", name}},
        clientCapabilities,
        QString::fromLatin1(kVersion2026),
        QJsonObject{});
}

} // namespace

class tst_McpProtocol : public QObject
{
    Q_OBJECT

private slots:
    void dispatchesByTheRequestsOwnProtocolVersion();
    void refusesAnUnsupportedProtocolVersion();
    void refusesCapabilitiesItCannotRead();
    void doesNotServeMethodsTheRevisionRemoved();
    void servesTasksAsAnExtension();
    void tagsCacheableResultsWithTheirFreshness();
    void doesNotTagUncacheableResults();
    void resumesAToolCallThatAskedForInput();
    void cancelsAResumedToolCallUnderTheRetrysId();
    void refusesAnUnknownRequestState();
    void startsATaskOnlyForAClientThatNegotiatedTheExtension();
    void doesNotLetOneClientNegotiateTasksForAnother();
    void keepsServingAClientItAlreadyKnowsAtTheSessionLimit();
    void deliversResourceUpdatesToBothRevisions();
    void endsOnlyTheSubscriptionOfTheSessionThatCancels();
    void refusesASecondSubscriptionUnderAnIdItAlreadyHolds();
    void refusesMoreSubscriptionsThanASessionMayHold();
    void answersEveryInputRequestATaskIsWaitingOn();
};

// The revision is decided per message, not per connection: the same method with
// and without the _meta version has to reach the two dispatches.
void tst_McpProtocol::dispatchesByTheRequestsOwnProtocolVersion()
{
    Harness h;

    // Without the _meta key this is the earlier revision, which still opens with
    // a handshake. 2026-07-28 cannot be the outcome of one, so a client asking
    // for it here is answered with the latest revision that has a handshake.
    const QJsonObject legacy = h.sendOne(legacyRequest(
        1,
        "initialize",
        QJsonObject{
            {"protocolVersion", kVersion2026},
            {"capabilities", QJsonObject{}},
            {"clientInfo", QJsonObject{{"name", "tst"}, {"version", "1.0"}}}}));
    QCOMPARE(result(legacy).value("protocolVersion").toString(), QString(kVersion2025));

    // With it there is no handshake at all: server/discover answers straight away.
    const QJsonObject discover = h.sendOne(request2026(2, "server/discover"));
    QCOMPARE(result(discover).value("resultType").toString(), QString("complete"));
    QVERIFY(
        result(discover).value("supportedVersions").toArray().contains(QJsonValue(kVersion2026)));
    QCOMPARE(
        result(discover)
            .value("_meta")
            .toObject()
            .value(kMetaServerInfo)
            .toObject()
            .value("name")
            .toString(),
        QString("tst_mcpprotocol"));
}

void tst_McpProtocol::refusesAnUnsupportedProtocolVersion()
{
    Harness h;

    const QJsonObject response = h.sendOne(request2026(1, "tools/list", {}, {}, "1999-01-01"));
    QCOMPARE(errorCode(response), -32022);

    // The version decides which methods exist, so a method 2026-07-28 removed is
    // still refused for its version and not for its name.
    const QJsonObject removed = h.sendOne(request2026(2, "ping", {}, {}, "1999-01-01"));
    QCOMPARE(errorCode(removed), -32022);
}

// A client that declares nothing and one whose declaration cannot be read are
// different things: the second is an error, not an empty capability set.
void tst_McpProtocol::refusesCapabilitiesItCannotRead()
{
    Harness h;
    h.server.addTool(namedTool("noop"), [](const Schema::CallToolRequestParams &) {
        return Schema::CallToolResult().isError(false);
    });

    QJsonObject request = request2026(1, "tools/list");
    QJsonObject params = request.value("params").toObject();
    QJsonObject meta = params.value("_meta").toObject();
    meta.insert(kMetaCapabilities, "elicitation");
    params.insert("_meta", meta);
    request.insert("params", params);

    QCOMPARE(errorCode(h.sendOne(request)), -32602);

    // Declaring nothing at all is still fine.
    QJsonObject silent = request2026(2, "tools/list");
    QJsonObject silentParams = silent.value("params").toObject();
    QJsonObject silentMeta = silentParams.value("_meta").toObject();
    silentMeta.remove(kMetaCapabilities);
    silentParams.insert("_meta", silentMeta);
    silent.insert("params", silentParams);

    QVERIFY(result(h.sendOne(silent)).contains("tools"));
}

// initialize, ping, logging/setLevel and resources/subscribe|unsubscribe are all
// gone in 2026-07-28 and must not be reachable through it, even though the
// handlers behind them are still there for the earlier revision.
void tst_McpProtocol::doesNotServeMethodsTheRevisionRemoved()
{
    Harness h;

    for (const QString &method :
         {QString("initialize"),
          QString("ping"),
          QString("logging/setLevel"),
          QString("resources/subscribe")}) {
        const QJsonObject response = h.sendOne(request2026(1, method));
        QCOMPARE(errorCode(response), -32601);
    }
}

// tasks/* stopped being core in the same revision, but comes back as an
// extension, so it has to stay reachable - and be advertised as an extension.
void tst_McpProtocol::servesTasksAsAnExtension()
{
    Harness h;

    const QJsonObject discover = h.sendOne(request2026(1, "server/discover"));
    QVERIFY(result(discover)
                .value("capabilities")
                .toObject()
                .value("extensions")
                .toObject()
                .contains(kTasksExtension));

    // An unknown task id is a "not found", not a "no such method".
    const QJsonObject get = h.sendOne(
        request2026(2, "tasks/get", QJsonObject{{"taskId", "does-not-exist"}}));
    QCOMPARE(errorCode(get), -32602);
}

void tst_McpProtocol::tagsCacheableResultsWithTheirFreshness()
{
    Harness h;
    h.server.addTool(namedTool("noop"), [](const Schema::CallToolRequestParams &) {
        return Schema::CallToolResult().isError(false);
    });

    const QJsonObject response = h.sendOne(request2026(1, "tools/list"));
    QCOMPARE(result(response).value("resultType").toString(), QString("complete"));
    QVERIFY(result(response).value("ttlMs").toInt() > 0);
    QCOMPARE(result(response).value("cacheScope").toString(), QString("private"));
}

void tst_McpProtocol::doesNotTagUncacheableResults()
{
    Harness h;
    h.server.addTool(namedTool("noop"), [](const Schema::CallToolRequestParams &) {
        return Schema::CallToolResult().isError(false);
    });

    const QJsonObject response = h.sendOne(callTool(1, "noop", {}));
    QCOMPARE(result(response).value("resultType").toString(), QString("complete"));
    QVERIFY(!result(response).contains("ttlMs"));
    QVERIFY(!result(response).contains("cacheScope"));
}

// 2026-07-28 dropped the server-initiated request: a tool that needs something
// from the client answers the call with input_required and a token, and the
// client retries the same call with the answer.
void tst_McpProtocol::resumesAToolCallThatAskedForInput()
{
    Harness h;
    h.server.addTool(
        namedTool("ask"),
        [](const Schema::CallToolRequestParams &, const Mcp::ToolInterface &tool) -> Result<> {
            tool.elicit(
                Schema::ElicitRequestFormParams()
                    .message("Your name?")
                    .requestedSchema(
                        Schema::ElicitRequestFormParams::RequestedSchema()
                            .addProperty("name", Schema::StringSchema().title("Name"))),
                [tool](const Result<Schema::ElicitResult> &elicited) {
                    if (!elicited) {
                        tool.finish(ResultError(elicited.error()));
                        return;
                    }
                    const auto &answer = elicited->content()->value("name");
                    tool.finish(
                        Schema::CallToolResult().isError(false).addStructuredContent(
                            "name",
                            std::holds_alternative<QString>(answer) ? std::get<QString>(answer)
                                                                    : QString()));
                });
            return ResultOk;
        });

    // The extensions capability rides along, as a 2026-07-28 client sends it:
    // the codec has to ignore it without losing the elicitation beside it.
    const QJsonObject capabilities{
        {"elicitation", QJsonObject{}},
        {"extensions", QJsonObject{{kTasksExtension, QJsonObject{}}}}};

    const QJsonObject suspended = h.sendOne(callTool(1, "ask", capabilities));
    QCOMPARE(result(suspended).value("resultType").toString(), QString("input_required"));
    const QString requestState = result(suspended).value("requestState").toString();
    QVERIFY(!requestState.isEmpty());
    QCOMPARE(
        result(suspended)
            .value("inputRequests")
            .toObject()
            .value("elicitation")
            .toObject()
            .value("method")
            .toString(),
        QString("elicitation/create"));

    QJsonObject retry = request2026(
        2,
        "tools/call",
        QJsonObject{
            {"name", "ask"},
            {"requestState", requestState},
            {"inputResponses",
             QJsonObject{
                 {"elicitation",
                  QJsonObject{{"action", "accept"}, {"content", QJsonObject{{"name", "Alice"}}}}}}}},
        capabilities);

    const QJsonObject finished = h.sendOne(retry);
    QCOMPARE(result(finished).value("resultType").toString(), QString("complete"));
    QCOMPARE(
        result(finished).value("structuredContent").toObject().value("name").toString(),
        QString("Alice"));

    // The token is spent, and a second retry must not resume anything.
    retry.insert("id", 3);
    QCOMPARE(errorCode(h.sendOne(retry)), -32602);
}

// The retry is a request of its own, so the call it resumed is pending under
// the retry's id from then on: that is the id the client would name to cancel
// it, and the id the answer would carry.
void tst_McpProtocol::cancelsAResumedToolCallUnderTheRetrysId()
{
    Harness h;

    // A resumed call that does not answer at once, as an asynchronous tool's
    // would not. Holding the interface is what keeps it pending.
    std::optional<Mcp::ToolInterface> resumed;
    h.server.addTool(
        namedTool("ask"),
        [&resumed](const Schema::CallToolRequestParams &, const Mcp::ToolInterface &tool)
            -> Result<> {
            tool.elicit(
                Schema::ElicitRequestFormParams()
                    .message("Your name?")
                    .requestedSchema(
                        Schema::ElicitRequestFormParams::RequestedSchema()
                            .addProperty("name", Schema::StringSchema().title("Name"))),
                [&resumed, tool](const Result<Schema::ElicitResult> &) { resumed = tool; });
            return ResultOk;
        });

    const QJsonObject capabilities{{"elicitation", QJsonObject{}}};
    const QJsonObject suspended = h.sendOne(callTool(1, "ask", capabilities));
    const QString requestState = result(suspended).value("requestState").toString();
    QVERIFY(!requestState.isEmpty());

    const QJsonObject retry = request2026(
        2,
        "tools/call",
        QJsonObject{
            {"name", "ask"},
            {"requestState", requestState},
            {"inputResponses",
             QJsonObject{
                 {"elicitation",
                  QJsonObject{{"action", "accept"}, {"content", QJsonObject{{"name", "Alice"}}}}}}}},
        capabilities);
    QVERIFY(h.send(retry).isEmpty());
    QVERIFY(resumed.has_value());

    const auto cancelOf = [](int requestId) {
        return request2026(-1, "notifications/cancelled", QJsonObject{{"requestId", requestId}});
    };

    // The id of the call that was suspended names nothing any more: the pending
    // call moved to the retry's id rather than being left under both.
    QVERIFY(h.send(cancelOf(1)).isEmpty());

    const QList<QJsonObject> cancelled = h.send(cancelOf(2));
    QCOMPARE(cancelled.size(), 1);
    QCOMPARE(cancelled.first().value("id").toInt(), 2);
    QCOMPARE(errorCode(cancelled.first()), -32800);
}

void tst_McpProtocol::refusesAnUnknownRequestState()
{
    Harness h;
    h.server.addTool(namedTool("noop"), [](const Schema::CallToolRequestParams &) {
        return Schema::CallToolResult().isError(false);
    });

    const QJsonObject response = h.sendOne(request2026(
        1, "tools/call", QJsonObject{{"name", "noop"}, {"requestState", "never-handed-out"}}));
    QCOMPARE(errorCode(response), -32602);
}

// A task must never be handed to a client that did not ask for one, and under
// 2026-07-28 asking means naming the tasks extension in the request's own
// capabilities.
void tst_McpProtocol::startsATaskOnlyForAClientThatNegotiatedTheExtension()
{
    Harness h;
    h.server.addTool(
        namedTool("slow").execution(
            Schema::ToolExecution().taskSupport(Schema::ToolExecution::TaskSupport::optional)),
        [](const Schema::CallToolRequestParams &, const Mcp::ToolInterface &tool) -> Result<> {
            const Result<Mcp::ToolInterface::TaskProgressNotify> started = tool.startTask(
                [](Schema::Task task) { return task; },
                []() -> Result<Schema::CallToolResult> {
                    return Schema::CallToolResult().isError(false);
                },
                std::nullopt,
                std::nullopt);
            if (!started)
                return ResultError(started.error());
            return ResultOk;
        });

    // No extensions: the tool cannot start a task and says so.
    const QJsonObject refused = h.sendOne(callTool(1, "slow", {}));
    QVERIFY(result(refused).value("isError").toBool());

    // With the extension the call is answered by a task handle instead. It
    // stands beside a member the codec does know, since that is the shape a
    // 2026-07-28 client sends and the one the 2025 shaped struct has to let
    // through.
    const QJsonObject capabilities{
        {"elicitation", QJsonObject{}},
        {"extensions", QJsonObject{{kTasksExtension, QJsonObject{}}}}};
    const QJsonObject accepted = h.sendOne(callTool(2, "slow", capabilities));
    QCOMPARE(result(accepted).value("resultType").toString(), QString("task"));
    QCOMPARE(result(accepted).value("status").toString(), QString("working"));
    QVERIFY(result(accepted).value("pollIntervalMs").toInt() > 0);

    const QString taskId = result(accepted).value("taskId").toString();
    QVERIFY(!taskId.isEmpty());

    const QJsonObject polled = h.sendOne(
        request2026(3, "tasks/get", QJsonObject{{"taskId", taskId}}, capabilities));
    QCOMPARE(result(polled).value("taskId").toString(), taskId);
    QCOMPARE(result(polled).value("resultType").toString(), QString("complete"));
}

// Two clients that did not name themselves share a session entry, and the last
// request to arrive owns it. A tool call that outlives its request must not go
// back to that entry to find out whether it may hand out a task.
void tst_McpProtocol::doesNotLetOneClientNegotiateTasksForAnother()
{
    Harness h;

    QList<Mcp::ToolInterface> parked;
    h.server.addTool(
        namedTool("deferred")
            .execution(
                Schema::ToolExecution().taskSupport(Schema::ToolExecution::TaskSupport::optional)),
        [&parked](const Schema::CallToolRequestParams &, const Mcp::ToolInterface &tool) -> Result<> {
            parked.append(tool);
            return ResultOk;
        });

    const QJsonObject withTasks{{"extensions", QJsonObject{{kTasksExtension, QJsonObject{}}}}};

    h.send(anonymousCallTool(1, "deferred", {}));
    h.send(anonymousCallTool(2, "deferred", withTasks));
    QCOMPARE(parked.size(), 2);

    // The first caller never declared the extension, whatever the second one did
    // to the session they share.
    const Result<Mcp::ToolInterface::TaskProgressNotify> started = parked.at(0).startTask(
        [](Schema::Task task) { return task; },
        []() -> Result<Schema::CallToolResult> { return Schema::CallToolResult().isError(false); },
        std::nullopt,
        std::nullopt);
    QVERIFY(!started);
}

// A 2026-07-28 request mints a session for a client identity the server has not
// seen, so it takes part in the session limit. Reaching that limit must not lock
// out the clients that are already through it.
void tst_McpProtocol::keepsServingAClientItAlreadyKnowsAtTheSessionLimit()
{
    Harness h;
    h.server.addTool(namedTool("noop"), [](const Schema::CallToolRequestParams &) {
        return Schema::CallToolResult().isError(false);
    });

    const auto served = [&h](const QJsonObject &request) {
        const QList<QJsonObject> replies = h.send(request);
        return replies.size() == 1 && replies.first().contains("result");
    };

    QVERIFY(served(namedCallTool(1, "noop", "first")));

    // Not the limit, only more identities than any sensible one: the flood is
    // over as soon as one of them is refused. Saying so is what makes a raised
    // limit legible here, since the limit itself is private to the server.
    constexpr int identities = 200;
    int refused = 0;
    for (int i = 0; i < identities && refused == 0; ++i) {
        if (!served(namedCallTool(i + 2, "noop", QString("flood%1").arg(i))))
            ++refused;
    }
    const QString notReached
        = QString("the session limit was not reached in %1 client identities").arg(identities);
    QVERIFY2(refused == 1, qPrintable(notReached));

    QVERIFY(served(namedCallTool(1000, "noop", "first")));
}

// The 2026-07-28 listeners are additional recipients, not the only ones: a
// resource update still owes the earlier revision's transports the notification
// they have always received.
void tst_McpProtocol::deliversResourceUpdatesToBothRevisions()
{
    Harness h;

    const QJsonObject listen = request2026(
        1,
        "subscriptions/listen",
        QJsonObject{
            {"notifications",
             QJsonObject{{"resourceSubscriptions", QJsonArray{"file:///watched"}}}}});
    const QJsonObject ack = h.sendOne(listen);
    QCOMPARE(ack.value("method").toString(), QString("notifications/subscriptions/acknowledged"));
    QCOMPARE(
        ack.value("params").toObject().value("_meta").toObject().value(kMetaSubscriptionId).toInt(),
        1);

    h.out.clear();
    h.server.sendNotification(
        Schema::ResourceUpdatedNotification().params(
            Schema::ResourceUpdatedNotificationParams().uri("file:///watched")));

    // The tagged copy for the listener that named the URI, and no untagged one
    // beside it: here the bound stream is that listener's own sink.
    QCOMPARE(h.out.size(), 1);
    const QJsonObject tagged = h.out.at(0);
    QCOMPARE(tagged.value("method").toString(), QString("notifications/resources/updated"));
    QCOMPARE(
        tagged.value("params").toObject().value("_meta").toObject().value(kMetaSubscriptionId).toInt(),
        1);

    // A URI the filter did not name reaches the bound stream only.
    h.out.clear();
    h.server.sendNotification(
        Schema::ResourceUpdatedNotification().params(
            Schema::ResourceUpdatedNotificationParams().uri("file:///ignored")));
    QCOMPARE(h.out.size(), 1);
    QVERIFY(!h.out.at(0).value("params").toObject().value("_meta").toObject().contains(
        kMetaSubscriptionId));
}

// A subscription is named by the client's own JSON-RPC id, which says nothing
// about who opened it: small integers make "1" the common choice rather than a
// corner. Cancelling has to end the subscription of the session that asks, and
// the one it finds first is not necessarily that one.
void tst_McpProtocol::endsOnlyTheSubscriptionOfTheSessionThatCancels()
{
    Harness h;

    const QJsonObject alice{{"name", "alice"}, {"version", "1.0"}};
    const QJsonObject bob{{"name", "bob"}, {"version", "1.0"}};
    const auto filterFor = [](const QString &uri) {
        return QJsonObject{
            {"notifications", QJsonObject{{"resourceSubscriptions", QJsonArray{uri}}}}};
    };

    // The same subscription id from two clients, each watching its own
    // resource, so only the session tells the two streams apart. Alice's is
    // registered first, which is the one a search by id alone would find.
    for (const auto &[who, uri] :
         {std::pair{alice, QString("file:///alice")}, std::pair{bob, QString("file:///bob")}}) {
        QCOMPARE(
            h.sendOne(request2026(1, "subscriptions/listen", filterFor(uri), {}, kVersion2026, who))
                .value("method")
                .toString(),
            QString("notifications/subscriptions/acknowledged"));
    }

    const auto taggedCopiesFor = [&h](const QString &uri) {
        h.out.clear();
        h.server.sendNotification(
            Schema::ResourceUpdatedNotification().params(
                Schema::ResourceUpdatedNotificationParams().uri(uri)));
        int tagged = 0;
        for (const QJsonObject &notification : std::as_const(h.out)) {
            if (notification.value("params").toObject().value("_meta").toObject().contains(
                    kMetaSubscriptionId)) {
                ++tagged;
            }
        }
        return tagged;
    };
    QCOMPARE(taggedCopiesFor("file:///alice"), 1);
    QCOMPARE(taggedCopiesFor("file:///bob"), 1);

    // Bob ends his. Alice's was opened first and carries the same id, so a
    // search that does not weigh the session ends hers instead.
    QJsonObject cancel = request2026(
        0, "notifications/cancelled", QJsonObject{{"requestId", 1}}, {}, kVersion2026, bob);
    cancel.remove("id");
    h.send(cancel);

    QCOMPARE(taggedCopiesFor("file:///bob"), 0);
    QCOMPARE(taggedCopiesFor("file:///alice"), 1);
}

// Nothing obliges a client to end a subscription before opening another, and a
// stream that cannot report itself closed is never pruned, so the count a
// session may hold is the server's to bound.
void tst_McpProtocol::refusesMoreSubscriptionsThanASessionMayHold()
{
    Harness h;

    const auto listen = [](int id) {
        return request2026(
            id,
            "subscriptions/listen",
            QJsonObject{
                {"notifications", QJsonObject{{"toolsListChanged", true}}}});
    };

    // Not the cap, only more subscriptions than any sensible one: the loop is
    // over as soon as one of them is refused.
    constexpr int subscriptions = 64;
    int acknowledged = 0;
    int refused = 0;
    int refusedCode = 0;
    for (int id = 1; id <= subscriptions && refused == 0; ++id) {
        const QJsonObject answer = h.sendOne(listen(id));
        if (answer.value("method").toString() == "notifications/subscriptions/acknowledged") {
            ++acknowledged;
        } else if (answer.contains("error")) {
            refusedCode = errorCode(answer);
            ++refused;
        }
    }

    const QString notBounded = QString("%1 subscriptions were accepted; %2 acknowledged")
                                   .arg(subscriptions)
                                   .arg(acknowledged);
    QVERIFY2(refused == 1, qPrintable(notBounded));
    QCOMPARE(refusedCode, -32098);

    // The refusal is of the subscription, not of the session: what it already
    // holds keeps working.
    h.out.clear();
    h.server.sendNotification(Schema::ToolListChangedNotification{});
    int tagged = 0;
    for (const QJsonObject &notification : std::as_const(h.out)) {
        if (notification.value("params").toObject().value("_meta").toObject().contains(
                kMetaSubscriptionId)) {
            ++tagged;
        }
    }
    QCOMPARE(tagged, acknowledged);
}

// Two instances of one client - two windows, two copies of an agent - report
// the same identity and so share a session, and the subscription id is the
// client's own. The pair is therefore kept unique at the listen rather than
// sorted out at the cancel: the instance that listened first keeps its stream,
// the second is told why it has none, and the id ends exactly one subscription.
void tst_McpProtocol::refusesASecondSubscriptionUnderAnIdItAlreadyHolds()
{
    Harness h;

    const QJsonObject twins{{"name", "twin"}, {"version", "1.0"}};
    const auto listenFor = [&twins](const QString &uri) {
        return request2026(
            1,
            "subscriptions/listen",
            QJsonObject{
                {"notifications", QJsonObject{{"resourceSubscriptions", QJsonArray{uri}}}}},
            {},
            kVersion2026,
            twins);
    };

    QCOMPARE(h.sendOne(listenFor("file:///one")).value("method").toString(),
             QString("notifications/subscriptions/acknowledged"));

    const QJsonObject refused = h.sendOne(listenFor("file:///two"));
    QCOMPARE(errorCode(refused), -32097);

    const auto delivers = [&h](const QString &uri) {
        h.out.clear();
        h.server.sendNotification(
            Schema::ResourceUpdatedNotification().params(
                Schema::ResourceUpdatedNotificationParams().uri(uri)));
        for (const QJsonObject &notification : std::as_const(h.out)) {
            if (notification.value("params").toObject().value("_meta").toObject().contains(
                    kMetaSubscriptionId)) {
                return true;
            }
        }
        return false;
    };

    // The one that was there is untouched, and the one that was refused is not
    // half-open: nothing was taken from the first instance to give the second.
    QVERIFY(delivers("file:///one"));
    QVERIFY(!delivers("file:///two"));

    QJsonObject cancel = request2026(
        0, "notifications/cancelled", QJsonObject{{"requestId", 1}}, {}, kVersion2026, twins);
    cancel.remove("id");
    h.send(cancel);

    QVERIFY(!delivers("file:///one"));
}

// A task-backed tool may have an elicitation and a sampling request out at
// once, and each is answered on its own. Two of one kind cannot both be
// outstanding - tasks/update answers under the kind - so the second is refused
// and the tool hears why, rather than quietly taking the first one's place.
void tst_McpProtocol::answersEveryInputRequestATaskIsWaitingOn()
{
    Harness h;
    QStringList heard;
    QString refusal;
    h.server.addTool(
        namedTool("askTwice")
            .execution(Schema::ToolExecution().taskSupport(
                Schema::ToolExecution::TaskSupport::optional)),
        [&heard, &refusal](
            const Schema::CallToolRequestParams &, const Mcp::ToolInterface &tool) -> Result<> {
            const Result<Mcp::ToolInterface::TaskProgressNotify> started = tool.startTask(
                [](Schema::Task task) { return task; },
                []() -> Result<Schema::CallToolResult> {
                    return Schema::CallToolResult().isError(false);
                },
                std::nullopt,
                std::nullopt);
            if (!started)
                return ResultError(started.error());

            const auto form = Schema::ElicitRequestFormParams()
                                  .message("Your name?")
                                  .requestedSchema(
                                      Schema::ElicitRequestFormParams::RequestedSchema()
                                          .addProperty("v", Schema::StringSchema().title("V")));
            tool.elicit(form, [&heard](const Result<Schema::ElicitResult> &elicited) {
                heard << (elicited ? QString("elicitation") : QString("elicitation failed"));
            });
            tool.sample(
                Schema::CreateMessageRequestParams().maxTokens(16),
                [&heard](const Result<Schema::CreateMessageResult> &sampled) {
                    heard << (sampled ? QString("sampling") : QString("sampling failed"));
                });
            // A second of a kind already outstanding is refused, not dropped.
            tool.elicit(form, [&refusal](const Result<Schema::ElicitResult> &elicited) {
                if (!elicited)
                    refusal = elicited.error();
            });
            return ResultOk;
        });

    const QJsonObject capabilities{
        {"elicitation", QJsonObject{}},
        {"sampling", QJsonObject{}},
        {"extensions", QJsonObject{{kTasksExtension, QJsonObject{}}}}};
    const QJsonObject accepted = h.sendOne(callTool(1, "askTwice", capabilities));
    QCOMPARE(result(accepted).value("resultType").toString(), QString("task"));
    const QString taskId = result(accepted).value("taskId").toString();
    QVERIFY(!taskId.isEmpty());

    QVERIFY2(!refusal.isEmpty(), "the second elicitation was not refused");
    QVERIFY(heard.isEmpty());

    const QJsonObject waiting = h.sendOne(
        request2026(2, "tasks/get", QJsonObject{{"taskId", taskId}}, capabilities));
    QCOMPARE(result(waiting).value("status").toString(), QString("input_required"));
    QStringList outstanding = result(waiting).value("inputRequests").toObject().keys();
    outstanding.sort();
    QCOMPARE(outstanding, QStringList({"elicitation", "sampling"}));

    // Answering one leaves the other outstanding, and the task still waiting.
    h.sendOne(request2026(
        3,
        "tasks/update",
        QJsonObject{
            {"taskId", taskId},
            {"inputResponses",
             QJsonObject{{"elicitation",
                          QJsonObject{{"action", "accept"},
                                      {"content", QJsonObject{{"v", "x"}}}}}}}},
        capabilities));
    QCOMPARE(heard, QStringList({"elicitation"}));

    const QJsonObject half = h.sendOne(
        request2026(4, "tasks/get", QJsonObject{{"taskId", taskId}}, capabilities));
    QCOMPARE(result(half).value("status").toString(), QString("input_required"));
    QCOMPARE(
        result(half).value("inputRequests").toObject().keys(), QStringList({"sampling"}));

    // And the other one is heard from too, rather than lost with the first.
    h.sendOne(request2026(
        5,
        "tasks/update",
        QJsonObject{
            {"taskId", taskId},
            {"inputResponses",
             QJsonObject{{"sampling",
                          QJsonObject{{"role", "assistant"},
                                      {"model", "test"},
                                      {"content",
                                       QJsonObject{{"type", "text"}, {"text", "hi"}}}}}}}},
        capabilities));
    QCOMPARE(heard.size(), 2);
    QCOMPARE(heard.at(1), QString("sampling"));
}

QTEST_GUILESS_MAIN(tst_McpProtocol)

#include "tst_mcpprotocol.moc"
