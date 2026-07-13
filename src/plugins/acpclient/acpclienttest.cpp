// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpclienttest.h"

#include "acpchatcontroller.h"
#include "acpclientobject.h"
#include "acppermissionhandler.h"
#include "acpsettings.h"
#include "acpstdiotransport.h"
#include "acptransport.h"

#include <acp/acp.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QTest>

using namespace Acp;
using namespace Utils;

namespace AcpClient::Internal {

static Utils::FilePath testServerBinary()
{
    return Utils::FilePath::fromUserInput(QLatin1String(ACP_TESTSERVER_DIR))
        .pathAppended("acptestserver")
        .withExecutableSuffix();
}

// The test server links against the same Qt and Qt Creator libraries as the
// running Qt Creator; make sure it finds both.
static Utils::FilePaths testServerLibrarySearchPaths()
{
    return {Utils::FilePath::fromUserInput(QCoreApplication::applicationDirPath()),
            Utils::FilePath::fromUserInput(QLibraryInfo::path(QLibraryInfo::BinariesPath))};
}

static Utils::Environment testServerEnvironment()
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.prependOrSetLibrarySearchPaths(testServerLibrarySearchPaths());
    return env;
}

static InitializeRequest testInitializeRequest()
{
    InitializeRequest request;
    request.protocolVersion(1);
    request.clientInfo(Implementation().name("AcpClientTest").version("1.0"));
    return request;
}

// In-process loopback transport. start()/stop() only emit the corresponding
// signals; everything written by the client is recorded for inspection.
class TestTransport final : public AcpTransport
{
public:
    void start() override { emit started(); }
    void stop() override { emit finished(); }

    void injectData(const QByteArray &data) { parseData(data); }
    void injectMessage(const QJsonObject &message)
    {
        injectData(QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n');
    }

    QJsonObject lastSentMessage() const
    {
        return sentMessages.isEmpty() ? QJsonObject() : sentMessages.last();
    }

    QList<QJsonObject> sentMessages;
    QByteArray sentRaw;

protected:
    void sendData(const QByteArray &data) override
    {
        sentRaw += data;
        const QList<QByteArray> lines = data.split('\n');
        for (const QByteArray &line : lines) {
            if (!line.trimmed().isEmpty())
                sentMessages.append(QJsonDocument::fromJson(line).object());
        }
    }
};

static QJsonObject makeResponse(const QJsonValue &id, const QJsonObject &result)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["result"] = result;
    return message;
}

static QJsonObject makeErrorResponse(const QJsonValue &id, int code, const QString &errorMessage)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["error"] = toJson(Error().code(code).message(errorMessage));
    return message;
}

static QJsonObject makeRequest(const QJsonValue &id, const QString &method,
                               const QJsonObject &params)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["method"] = method;
    message["params"] = params;
    return message;
}

static QJsonObject makeNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["method"] = method;
    message["params"] = params;
    return message;
}

static QJsonObject permissionRequestParams(const QString &sessionId)
{
    return toJson(RequestPermissionRequest()
                      .sessionId(sessionId)
                      .toolCall(ToolCallUpdate().toolCallId("tool-1").title("Test tool"))
                      .addOption(PermissionOption()
                                     .optionId("allow")
                                     .name("Allow")
                                     .kind(PermissionOptionKind::allow_once))
                      .addOption(PermissionOption()
                                     .optionId("reject")
                                     .name("Reject")
                                     .kind(PermissionOptionKind::reject_once)));
}

// Fixture for tests driving AcpClientObject against a real acptestserver
// process over stdio.
class ServerFixture
{
public:
    ServerFixture()
        : client(&transport)
    {
        QObject::connect(&transport, &AcpTransport::errorOccurred, &client,
                         [this](const QString &error) { transportErrors << error; });
        QObject::connect(&transport, &AcpTransport::finished, &client,
                         [this] { ++finishedCount; });
        QObject::connect(&client, &AcpClientObject::sessionUpdate, &client,
                         [this](const QString &sessionId, const SessionUpdate &update) {
                             updates.append({sessionId, update});
                         });
        QObject::connect(&client, &AcpClientObject::initializeResult, &client,
                         [this](const InitializeResponse &response) {
                             initializeResponse = response;
                         });
    }

    ~ServerFixture() { transport.stop(); }

    // Starts the server and runs the initialize handshake. Callers must bail
    // out with QTest::currentTestFailed() afterwards.
    void start(const QStringList &arguments)
    {
        const Utils::FilePath binary = testServerBinary();
        QVERIFY2(binary.isExecutableFile(),
                 qPrintable(QString("acptestserver not found: %1").arg(binary.toUserOutput())));

        transport.setCommandLine(Utils::CommandLine(binary, arguments));
        transport.setEnvironment(testServerEnvironment());
        transport.start();
        // Bail out quickly with the actual error text if the server fails to
        // come up, instead of running into the timeout.
        QTRY_VERIFY_WITH_TIMEOUT(client.state() == AcpClientObject::State::Connecting
                                     || !transportErrors.isEmpty(),
                                 15000);
        QVERIFY2(transportErrors.isEmpty(), qPrintable(transportErrors.join("; ")));

        bool initialized = false;
        std::optional<Error> initializeError;
        client.initialize(testInitializeRequest(),
                          [&](const QJsonObject &, const std::optional<Error> &error) {
                              initialized = true;
                              initializeError = error;
                          });
        QTRY_VERIFY_WITH_TIMEOUT(initialized || !transportErrors.isEmpty(), 15000);
        QVERIFY2(transportErrors.isEmpty(), qPrintable(transportErrors.join("; ")));
        QVERIFY(initialized);
        QVERIFY(!initializeError);
        QCOMPARE(client.state(), AcpClientObject::State::Initialized);
        QVERIFY(initializeResponse);
    }

    QString newSession()
    {
        QString sessionId;
        bool done = false;
        client.newSession(NewSessionRequest().cwd(QDir::currentPath()),
                          [&](const QJsonObject &result, const std::optional<Error> &error) {
                              done = true;
                              if (!error) {
                                  if (const auto response = fromJson<NewSessionResponse>(
                                          QJsonValue(result)))
                                      sessionId = response->sessionId();
                              }
                          });
        [&] { QTRY_VERIFY(done); }();
        return sessionId;
    }

    AcpStdioTransport transport;
    AcpClientObject client;
    QStringList transportErrors;
    int finishedCount = 0;
    QList<std::pair<QString, SessionUpdate>> updates;
    std::optional<InitializeResponse> initializeResponse;
};

#define START_SERVER(fixture, ...) \
    do { \
        (fixture).start(QStringList(__VA_ARGS__)); \
        if (QTest::currentTestFailed()) \
            return; \
    } while (false)

// Fixture for tests driving the AcpChatController workflows end-to-end, the
// same way AcpChatTab does.
class ControllerFixture
{
public:
    ControllerFixture()
    {
        QObject::connect(&controller, &AcpChatController::connectionStateChanged, &controller,
                         [this](AcpClientObject::State state) { states.append(state); });
        QObject::connect(&controller, &AcpChatController::agentInfoReceived, &controller,
                         [this](const QString &name, const QString &version, const QString &) {
                             agentName = name;
                             agentVersion = version;
                         });
        QObject::connect(&controller, &AcpChatController::sessionSelectionRequired, &controller,
                         [this] { ++sessionSelectionRequiredCount; });
        QObject::connect(&controller, &AcpChatController::sessionCreated, &controller,
                         [this](const QString &sessionId) { createdSessions.append(sessionId); });
        QObject::connect(&controller, &AcpChatController::sessionLoaded, &controller,
                         [this](const QString &sessionId) { loadedSessions.append(sessionId); });
        QObject::connect(&controller, &AcpChatController::sessionsListed, &controller,
                         [this](const QList<SessionInfo> &sessions,
                                const std::optional<QString> &nextCursor) {
                             listedSessions.append(sessions);
                             lastCursor = nextCursor;
                             ++listCount;
                         });
        QObject::connect(&controller, &AcpChatController::sessionDeleted, &controller,
                         [this](const QString &sessionId) { deletedSessions.append(sessionId); });
        QObject::connect(&controller, &AcpChatController::sessionClosed, &controller,
                         [this](const QString &sessionId) { closedSessions.append(sessionId); });
        QObject::connect(&controller, &AcpChatController::configOptionsReceived, &controller,
                         [this](const QList<SessionConfigOption> &options) {
                             configOptions = options;
                         });
        QObject::connect(&controller, &AcpChatController::sessionModesReceived, &controller,
                         [this](const QList<SessionMode> &modes, const QString &currentModeId) {
                             sessionModes = modes;
                             currentMode = currentModeId;
                         });
        QObject::connect(&controller, &AcpChatController::currentModeChanged, &controller,
                         [this](const QString &modeId) { currentMode = modeId; });
        QObject::connect(&controller, &AcpChatController::sessionUpdate, &controller,
                         [this](const QString &, const SessionUpdate &update) {
                             updates.append(update);
                         });
        QObject::connect(&controller, &AcpChatController::promptFinished, &controller,
                         [this] { ++promptFinishedCount; });
        QObject::connect(&controller, &AcpChatController::authenticationRequired, &controller,
                         [this](const QList<AuthMethod> &methods) { authMethods = methods; });
        QObject::connect(&controller, &AcpChatController::authenticationFailed, &controller,
                         [this](const QString &error) { authErrors.append(error); });
        QObject::connect(&controller, &AcpChatController::permissionRequested, &controller,
                         [this](const QJsonValue &id, const RequestPermissionRequest &request) {
                             permissionIds.append(id);
                             permissionRequests.append(request);
                         });
        QObject::connect(&controller, &AcpChatController::permissionCancelledByAgent, &controller,
                         [this](const QJsonValue &id) { cancelledPermissionIds.append(id); });
        QObject::connect(&controller, &AcpChatController::errorOccurred, &controller,
                         [this](const QString &error) { errors.append(error); });
    }

    ~ControllerFixture()
    {
        // Shut down while the fixture is fully alive and flush the deferred
        // deletion of the controller's helper objects, so that the controller
        // destructor does not hard-delete children that still have pending
        // deleteLater() events.
        controller.disconnectFromServer();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // Connects to acptestserver and waits until the initialize handshake is
    // done. Callers must bail out with QTest::currentTestFailed() afterwards.
    void connectToServer(const QStringList &arguments)
    {
        const Utils::FilePath binary = testServerBinary();
        QVERIFY2(binary.isExecutableFile(),
                 qPrintable(QString("acptestserver not found: %1").arg(binary.toUserOutput())));

        AcpSettings::ServerInfo info;
        info.id = "acptestserver";
        info.name = "AcpTestServer";
        info.launchCommand = Utils::CommandLine(binary, arguments);
        const QString libraryPathVariable = Utils::HostOsInfo::isWindowsHost()
                                                ? QString("PATH")
                                                : Utils::HostOsInfo::isMacHost()
                                                      ? QString("DYLD_LIBRARY_PATH")
                                                      : QString("LD_LIBRARY_PATH");
        Utils::EnvironmentItems items;
        for (const Utils::FilePath &path : testServerLibrarySearchPaths()) {
            items.append(Utils::EnvironmentItem(libraryPathVariable, path.nativePath(),
                                                Utils::EnvironmentItem::Prepend));
        }
        info.envChanges = Utils::EnvironmentChanges(items);
        controller.connectToServer(info);
        // Bail out quickly with the actual error text if the server fails to
        // come up, instead of running into the timeout.
        QTRY_VERIFY_WITH_TIMEOUT(controller.isInitialized() || !errors.isEmpty(), 15000);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join("; ")));
        QTRY_VERIFY(sessionSelectionRequiredCount > 0);
    }

    AcpChatController controller;
    QList<AcpClientObject::State> states;
    QString agentName;
    QString agentVersion;
    int sessionSelectionRequiredCount = 0;
    QStringList createdSessions;
    QStringList loadedSessions;
    QList<SessionInfo> listedSessions;
    std::optional<QString> lastCursor;
    int listCount = 0;
    QStringList deletedSessions;
    QStringList closedSessions;
    QList<SessionConfigOption> configOptions;
    QList<SessionMode> sessionModes;
    QString currentMode;
    QList<SessionUpdate> updates;
    int promptFinishedCount = 0;
    QList<AuthMethod> authMethods;
    QStringList authErrors;
    QList<QJsonValue> permissionIds;
    QList<RequestPermissionRequest> permissionRequests;
    QList<QJsonValue> cancelledPermissionIds;
    QStringList errors;
};

#define CONNECT_CONTROLLER(fixture, ...) \
    do { \
        (fixture).connectToServer(QStringList(__VA_ARGS__)); \
        if (QTest::currentTestFailed()) \
            return; \
    } while (false)

class AcpClientTest final : public QObject
{
    Q_OBJECT

private slots:
    // Tier 1a: newline-delimited JSON framing (AcpTransport)
    void testFramingSingleMessage();
    void testFramingChunkedData();
    void testFramingMultipleMessages();
    void testFramingCrLfAndEmptyLines();
    void testFramingMalformedJson();
    void testFramingNonObject();
    void testFramingSendFormat();

    // Tier 1b: JSON-RPC engine (AcpClientObject on a loopback transport)
    void testClientInitialize();
    void testClientStringIdResponse();
    void testClientUnknownIdResponse();
    void testClientErrorResponse();
    void testClientAgentRequestDispatch();
    void testClientAgentRequestInvalidParams();
    void testClientUnknownAgentRequest();
    void testClientAgentCancelsRequest();
    void testClientSessionUpdateNotification();
    void testClientDisconnectClearsState();
    void testPermissionHandlerOutcomes();
    void testPermissionHandlerAgentCancel();

    // Tier 1c: stdio transport against the real acptestserver
    void testStdioStartStop();
    void testStdioMissingExecutable();
    void testStdioCleanServerExit();
    void testStdioStderrNoise();

    // Tier 2: protocol workflows against acptestserver (AcpClientObject)
    void testE2ePromptStreaming();
    void testE2eAllUpdateKinds();
    void testE2ePermissionGranted();
    void testE2ePermissionDenied();
    void testE2ePermissionCancelledByAgent();
    void testE2eCancelPrompt();
    void testE2eAuthentication();
    void testE2eSessionManagement();
    void testE2eServerCrash();
    void testE2eInvalidResponseLine();

    // Tier 3: chat workflows against acptestserver (AcpChatController)
    void testControllerConnectAndSession();
    void testControllerPromptStreaming();
    void testControllerAuthentication();
    void testControllerCapabilityGating();
    void testControllerSessionManagement();
    void testControllerCancelPrompt();
    void testControllerServerCrash();
    void testControllerDisconnect();
};

// --- Tier 1a -----------------------------------------------------------------

// A single complete JSON line results in exactly one decoded message.
void AcpClientTest::testFramingSingleMessage()
{
    TestTransport transport;
    QList<QJsonObject> received;
    connect(&transport, &AcpTransport::messageReceived, this,
            [&](const QJsonObject &message) { received.append(message); });

    transport.injectData("{\"a\":1}\n");
    QCOMPARE(received.size(), 1);
    QCOMPARE(received.first().value("a").toInt(), 1);
}

// A message split across multiple data chunks is only delivered once the
// terminating newline arrives.
void AcpClientTest::testFramingChunkedData()
{
    TestTransport transport;
    QList<QJsonObject> received;
    connect(&transport, &AcpTransport::messageReceived, this,
            [&](const QJsonObject &message) { received.append(message); });

    transport.injectData("{\"a\"");
    QCOMPARE(received.size(), 0);
    transport.injectData(":1}");
    QCOMPARE(received.size(), 0);
    transport.injectData("\n");
    QCOMPARE(received.size(), 1);
    QCOMPARE(received.first().value("a").toInt(), 1);
}

// Multiple messages arriving in one data chunk are delivered separately, in order.
void AcpClientTest::testFramingMultipleMessages()
{
    TestTransport transport;
    QList<QJsonObject> received;
    connect(&transport, &AcpTransport::messageReceived, this,
            [&](const QJsonObject &message) { received.append(message); });

    transport.injectData("{\"a\":1}\n{\"b\":2}\n");
    QCOMPARE(received.size(), 2);
    QCOMPARE(received.at(0).value("a").toInt(), 1);
    QCOMPARE(received.at(1).value("b").toInt(), 2);
}

// CR-LF line endings and blank lines are tolerated without errors.
void AcpClientTest::testFramingCrLfAndEmptyLines()
{
    TestTransport transport;
    QList<QJsonObject> received;
    connect(&transport, &AcpTransport::messageReceived, this,
            [&](const QJsonObject &message) { received.append(message); });

    transport.injectData("{\"a\":1}\r\n\r\n\n");
    QCOMPARE(received.size(), 1);
    QCOMPARE(received.first().value("a").toInt(), 1);
}

// A malformed JSON line is reported as an error and the parser recovers:
// the next valid line is still delivered.
void AcpClientTest::testFramingMalformedJson()
{
    TestTransport transport;
    QList<QJsonObject> received;
    QStringList errors;
    connect(&transport, &AcpTransport::messageReceived, this,
            [&](const QJsonObject &message) { received.append(message); });
    connect(&transport, &AcpTransport::errorOccurred, this,
            [&](const QString &error) { errors.append(error); });

    transport.injectData("{oops\n{\"a\":1}\n");
    QCOMPARE(errors.size(), 1);
    QCOMPARE(received.size(), 1);
    QCOMPARE(received.first().value("a").toInt(), 1);
}

// A valid JSON line that is not an object is reported as an error and dropped.
void AcpClientTest::testFramingNonObject()
{
    TestTransport transport;
    QList<QJsonObject> received;
    QStringList errors;
    connect(&transport, &AcpTransport::messageReceived, this,
            [&](const QJsonObject &message) { received.append(message); });
    connect(&transport, &AcpTransport::errorOccurred, this,
            [&](const QString &error) { errors.append(error); });

    transport.injectData("[1,2]\n");
    QCOMPARE(errors.size(), 1);
    QCOMPARE(received.size(), 0);
}

// send() produces compact JSON terminated by exactly one newline.
void AcpClientTest::testFramingSendFormat()
{
    TestTransport transport;
    transport.send(QJsonObject{{"jsonrpc", "2.0"}, {"method", "test"}});

    QVERIFY(transport.sentRaw.endsWith('\n'));
    QCOMPARE(transport.sentRaw.count('\n'), 1);
    const QJsonObject decoded = QJsonDocument::fromJson(transport.sentRaw.chopped(1)).object();
    QCOMPARE(decoded.value("method").toString(), "test");
}

// --- Tier 1b -----------------------------------------------------------------

// The initialize handshake walks the state machine Disconnected -> Connecting ->
// InitializeRequested -> Initialized, sends a well-formed request, and delivers
// the parsed response through both the callback and the initializeResult signal.
void AcpClientTest::testClientInitialize()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    QCOMPARE(client.state(), AcpClientObject::State::Disconnected);

    transport.start();
    QCOMPARE(client.state(), AcpClientObject::State::Connecting);

    bool callbackInvoked = false;
    std::optional<Error> callbackError;
    client.initialize(testInitializeRequest(),
                      [&](const QJsonObject &, const std::optional<Error> &error) {
                          callbackInvoked = true;
                          callbackError = error;
                      });
    QCOMPARE(client.state(), AcpClientObject::State::InitializeRequested);
    QCOMPARE(transport.sentMessages.size(), 1);
    const QJsonObject sent = transport.lastSentMessage();
    QCOMPARE(sent.value("method").toString(), "initialize");
    QVERIFY(sent.value("id").isDouble());
    QCOMPARE(sent.value("params").toObject().value("protocolVersion").toInt(), 1);

    std::optional<InitializeResponse> receivedResponse;
    connect(&client, &AcpClientObject::initializeResult, this,
            [&](const InitializeResponse &response) { receivedResponse = response; });

    InitializeResponse response;
    response.protocolVersion(1);
    response.agentInfo(Implementation().name("fake-agent").version("2.0"));
    transport.injectMessage(makeResponse(sent.value("id"), toJson(response)));

    QCOMPARE(client.state(), AcpClientObject::State::Initialized);
    QVERIFY(callbackInvoked);
    QVERIFY(!callbackError);
    QVERIFY(receivedResponse);
    QVERIFY(receivedResponse->agentInfo());
    QCOMPARE(receivedResponse->agentInfo()->name(), "fake-agent");
}

// Responses with a non-integer id are rejected; the correct integer id still
// completes the pending request afterwards.
void AcpClientTest::testClientStringIdResponse()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    bool callbackInvoked = false;
    client.newSession(NewSessionRequest().cwd("/tmp"),
                      [&](const QJsonObject &, const std::optional<Error> &) {
                          callbackInvoked = true;
                      });
    const QJsonValue id = transport.lastSentMessage().value("id");

    // A response with a string id must be rejected...
    transport.injectMessage(makeResponse(QString::number(id.toDouble()), QJsonObject()));
    QVERIFY(!callbackInvoked);

    // ...while the correct integer id still completes the request.
    transport.injectMessage(makeResponse(id, toJson(NewSessionResponse().sessionId("s"))));
    QVERIFY(callbackInvoked);
}

// A response for an id that was never sent is ignored.
void AcpClientTest::testClientUnknownIdResponse()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    bool callbackInvoked = false;
    client.newSession(NewSessionRequest().cwd("/tmp"),
                      [&](const QJsonObject &, const std::optional<Error> &) {
                          callbackInvoked = true;
                      });

    transport.injectMessage(makeResponse(999999, QJsonObject()));
    QVERIFY(!callbackInvoked);
}

// An error response delivers the parsed Acp::Error to the callback,
// with an empty result.
void AcpClientTest::testClientErrorResponse()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    std::optional<Error> callbackError;
    QJsonObject callbackResult{{"marker", true}};
    client.newSession(NewSessionRequest().cwd("/tmp"),
                      [&](const QJsonObject &result, const std::optional<Error> &error) {
                          callbackError = error;
                          callbackResult = result;
                      });
    const QJsonValue id = transport.lastSentMessage().value("id");

    transport.injectMessage(
        makeErrorResponse(id, ErrorCode::Authentication_required, "login first"));
    QVERIFY(callbackError);
    QCOMPARE(callbackError->code(), ErrorCode::Authentication_required);
    QCOMPARE(callbackError->message(), "login first");
    QVERIFY(callbackResult.isEmpty());
}

// An agent request is dispatched as a typed signal carrying the raw id, and
// sendResponse() answers it exactly once - a second response for the same id
// is dropped.
void AcpClientTest::testClientAgentRequestDispatch()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    QList<QJsonValue> requestIds;
    QList<RequestPermissionRequest> requests;
    connect(&client, &AcpClientObject::requestPermissionRequested, this,
            [&](const QJsonValue &id, const RequestPermissionRequest &request) {
                requestIds.append(id);
                requests.append(request);
            });

    transport.injectMessage(
        makeRequest(7, "session/request_permission", permissionRequestParams("session-1")));
    QCOMPARE(requestIds.size(), 1);
    QCOMPARE(requestIds.first().toInt(), 7);
    QCOMPARE(requests.first().options().size(), 2);
    QCOMPARE(requests.first().options().first().optionId(), "allow");

    // Responding twice must only produce one message on the wire.
    const int sentBefore = transport.sentMessages.size();
    client.sendResponse(requestIds.first(), QJsonObject{{"answer", 42}});
    QCOMPARE(transport.sentMessages.size(), sentBefore + 1);
    const QJsonObject sent = transport.lastSentMessage();
    QCOMPARE(sent.value("id").toInt(), 7);
    QCOMPARE(sent.value("result").toObject().value("answer").toInt(), 42);

    client.sendResponse(requestIds.first(), QJsonObject());
    QCOMPARE(transport.sentMessages.size(), sentBefore + 1);
}

// An agent request with invalid parameters emits no signal and is automatically
// answered with an Invalid_params error.
void AcpClientTest::testClientAgentRequestInvalidParams()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    int requestCount = 0;
    connect(&client, &AcpClientObject::requestPermissionRequested, this,
            [&](const QJsonValue &, const RequestPermissionRequest &) { ++requestCount; });

    transport.injectMessage(
        makeRequest(8, "session/request_permission", QJsonObject{{"bogus", true}}));
    QCOMPARE(requestCount, 0);
    const QJsonObject sent = transport.lastSentMessage();
    QCOMPARE(sent.value("id").toInt(), 8);
    QCOMPARE(sent.value("error").toObject().value("code").toInt(), ErrorCode::Invalid_params);
}

// An unknown agent request method is automatically answered with Method_not_found.
void AcpClientTest::testClientUnknownAgentRequest()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    transport.injectMessage(makeRequest(9, "bogus/method", QJsonObject()));
    const QJsonObject sent = transport.lastSentMessage();
    QCOMPARE(sent.value("id").toInt(), 9);
    QCOMPARE(sent.value("error").toObject().value("code").toInt(), ErrorCode::Method_not_found);
}

// $/cancel_request for an unknown id does nothing; for a pending id it emits
// requestCancelled, auto-answers with Request_cancelled, and invalidates the id.
void AcpClientTest::testClientAgentCancelsRequest()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    QList<QJsonValue> cancelledIds;
    connect(&client, &AcpClientObject::requestCancelled, this,
            [&](const QJsonValue &id) { cancelledIds.append(id); });

    transport.injectMessage(
        makeRequest(10, "session/request_permission", permissionRequestParams("session-1")));

    // Cancelling an unknown id does nothing.
    const int sentBefore = transport.sentMessages.size();
    transport.injectMessage(makeNotification(
        "$/cancel_request", toJson(CancelRequestNotification().requestId(RequestId(999)))));
    QVERIFY(cancelledIds.isEmpty());
    QCOMPARE(transport.sentMessages.size(), sentBefore);

    // Cancelling the pending id emits the signal and auto-answers with an error.
    transport.injectMessage(makeNotification(
        "$/cancel_request", toJson(CancelRequestNotification().requestId(RequestId(10)))));
    QCOMPARE(cancelledIds.size(), 1);
    QCOMPARE(cancelledIds.first().toInt(), 10);
    const QJsonObject sent = transport.lastSentMessage();
    QCOMPARE(sent.value("id").toInt(), 10);
    QCOMPARE(sent.value("error").toObject().value("code").toInt(), ErrorCode::Request_cancelled);

    // The request is gone; a later response is not sent anymore.
    client.sendResponse(cancelledIds.first(), QJsonObject());
    QCOMPARE(transport.sentMessages.size(), sentBefore + 1);
}

// A session/update notification is delivered as a sessionUpdate signal with the
// correct variant type; invalid updates emit nothing.
void AcpClientTest::testClientSessionUpdateNotification()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    QList<std::pair<QString, SessionUpdate>> updates;
    connect(&client, &AcpClientObject::sessionUpdate, this,
            [&](const QString &sessionId, const SessionUpdate &update) {
                updates.append({sessionId, update});
            });

    SessionUpdate update;
    update._value = ContentChunk().content(TextContent().text("hello"));
    update._kind = "agent_message_chunk";
    transport.injectMessage(makeNotification(
        "session/update",
        toJson(SessionNotification().sessionId("session-1").update(update))));

    QCOMPARE(updates.size(), 1);
    QCOMPARE(updates.first().first, "session-1");
    QCOMPARE(updates.first().second.kind(), "agent_message_chunk");
    const auto *chunk = updates.first().second.get<ContentChunk>();
    QVERIFY(chunk);
    const auto *text = std::get_if<TextContent>(&chunk->content());
    QVERIFY(text);
    QCOMPARE(text->text(), "hello");

    // An invalid update must not emit anything.
    transport.injectMessage(makeNotification("session/update", QJsonObject{{"bogus", 1}}));
    QCOMPARE(updates.size(), 1);
}

// Stopping the transport clears all pending request state: late responses and
// late outgoing responses have no effect.
void AcpClientTest::testClientDisconnectClearsState()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    transport.start();

    bool callbackInvoked = false;
    client.newSession(NewSessionRequest().cwd("/tmp"),
                      [&](const QJsonObject &, const std::optional<Error> &) {
                          callbackInvoked = true;
                      });
    const QJsonValue requestId = transport.lastSentMessage().value("id");
    transport.injectMessage(
        makeRequest(11, "session/request_permission", permissionRequestParams("session-1")));

    transport.stop();
    QCOMPARE(client.state(), AcpClientObject::State::Disconnected);

    // Neither late responses nor late outgoing responses have any effect.
    transport.injectMessage(makeResponse(requestId, QJsonObject()));
    QVERIFY(!callbackInvoked);
    const int sentBefore = transport.sentMessages.size();
    client.sendResponse(11, QJsonObject());
    QCOMPARE(transport.sentMessages.size(), sentBefore);
}

// The permission handler encodes the selected and cancelled outcomes correctly
// on the wire.
void AcpClientTest::testPermissionHandlerOutcomes()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    AcpPermissionHandler handler(&client);
    transport.start();

    QList<QJsonValue> permissionIds;
    connect(&handler, &AcpPermissionHandler::permissionRequested, this,
            [&](const QJsonValue &id, const RequestPermissionRequest &) {
                permissionIds.append(id);
            });

    transport.injectMessage(
        makeRequest(20, "session/request_permission", permissionRequestParams("session-1")));
    QCOMPARE(permissionIds.size(), 1);

    handler.sendPermissionResponse(permissionIds.first(), "allow");
    QJsonObject outcome = transport.lastSentMessage()
                              .value("result").toObject()
                              .value("outcome").toObject();
    QCOMPARE(outcome.value("outcome").toString(), "selected");
    QCOMPARE(outcome.value("optionId").toString(), "allow");

    transport.injectMessage(
        makeRequest(21, "session/request_permission", permissionRequestParams("session-1")));
    QCOMPARE(permissionIds.size(), 2);

    handler.sendPermissionCancelled(permissionIds.last());
    outcome = transport.lastSentMessage()
                  .value("result").toObject()
                  .value("outcome").toObject();
    QCOMPARE(outcome.value("outcome").toString(), "cancelled");
}

// When the agent withdraws a pending permission request via $/cancel_request,
// the handler emits permissionCancelledByAgent with the matching id.
void AcpClientTest::testPermissionHandlerAgentCancel()
{
    TestTransport transport;
    AcpClientObject client(&transport);
    AcpPermissionHandler handler(&client);
    transport.start();

    QList<QJsonValue> permissionIds;
    QList<QJsonValue> cancelledIds;
    connect(&handler, &AcpPermissionHandler::permissionRequested, this,
            [&](const QJsonValue &id, const RequestPermissionRequest &) {
                permissionIds.append(id);
            });
    connect(&handler, &AcpPermissionHandler::permissionCancelledByAgent, this,
            [&](const QJsonValue &id) { cancelledIds.append(id); });

    transport.injectMessage(
        makeRequest(22, "session/request_permission", permissionRequestParams("session-1")));
    QCOMPARE(permissionIds.size(), 1);

    transport.injectMessage(makeNotification(
        "$/cancel_request", toJson(CancelRequestNotification().requestId(RequestId(22)))));
    QCOMPARE(cancelledIds.size(), 1);
    QCOMPARE(cancelledIds.first().toInt(), 22);
}

// --- Tier 1c -----------------------------------------------------------------

// The server starts, the initialize handshake completes, and an expected stop
// tears down without reporting errors.
void AcpClientTest::testStdioStartStop()
{
    ServerFixture fixture;
    START_SERVER(fixture);

    fixture.transport.stop();
    QCOMPARE(fixture.client.state(), AcpClientObject::State::Disconnected);
    QVERIFY2(fixture.transportErrors.isEmpty(),
             qPrintable(fixture.transportErrors.join("; ")));
}

// A nonexistent server command reports an error and finished, but never started.
void AcpClientTest::testStdioMissingExecutable()
{
    AcpStdioTransport transport;
    QStringList errors;
    int startedCount = 0;
    int finishedCount = 0;
    connect(&transport, &AcpTransport::errorOccurred, this,
            [&](const QString &error) { errors.append(error); });
    connect(&transport, &AcpTransport::started, this, [&] { ++startedCount; });
    connect(&transport, &AcpTransport::finished, this, [&] { ++finishedCount; });

    transport.setCommandLine(
        Utils::CommandLine(Utils::FilePath::fromUserInput("nonexistent-acp-server-xyz")));
    transport.start();

    QCOMPARE(errors.size(), 1);
    QVERIFY(errors.first().contains("Command not found"));
    QCOMPARE(finishedCount, 1);
    QCOMPARE(startedCount, 0);
}

// A voluntary server exit with code 0 emits finished without any error.
void AcpClientTest::testStdioCleanServerExit()
{
    ServerFixture fixture;
    START_SERVER(fixture);

    // Ask the server to quit cleanly via the test-only request.
    fixture.transport.send(makeRequest(12345, "test/quit", QJsonObject()));

    QTRY_VERIFY(fixture.finishedCount > 0);
    QVERIFY2(fixture.transportErrors.isEmpty(),
             qPrintable(fixture.transportErrors.join("; ")));
    QCOMPARE(fixture.client.state(), AcpClientObject::State::Disconnected);
}

// Server output on stderr never corrupts the protocol stream on stdout.
void AcpClientTest::testStdioStderrNoise()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--stderr-noise"});

    // The handshake in start() already proves stderr does not corrupt the
    // protocol stream; a session round-trip confirms it once more.
    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;
    QVERIFY(!sessionId.isEmpty());
    QVERIFY2(fixture.transportErrors.isEmpty(),
             qPrintable(fixture.transportErrors.join("; ")));
}

// --- Tier 2 ------------------------------------------------------------------

// session/prompt streams all agent_message_chunk updates in order, and all of
// them arrive before the end_turn response completes the request.
void AcpClientTest::testE2ePromptStreaming()
{
    ServerFixture fixture;
    START_SERVER(fixture);

    QVERIFY(fixture.initializeResponse->agentInfo());
    QCOMPARE(fixture.initializeResponse->agentInfo()->name(), "acptestserver");
    QCOMPARE(fixture.initializeResponse->protocolVersion(), 1);

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;
    QVERIFY(!sessionId.isEmpty());

    int updatesAtFinish = -1;
    std::optional<StopReason> stopReason;
    fixture.client.prompt(
        PromptRequest().sessionId(sessionId).addPrompt(TextContent().text("hello")),
        [&](const QJsonObject &result, const std::optional<Error> &error) {
            QVERIFY(!error);
            updatesAtFinish = fixture.updates.size();
            if (const auto response = fromJson<PromptResponse>(QJsonValue(result)))
                stopReason = response->stopReason();
        });

    QTRY_VERIFY(stopReason.has_value());
    QCOMPARE(toString(*stopReason), toString(StopReason::end_turn));
    QCOMPARE(fixture.updates.size(), 3);
    QCOMPARE(updatesAtFinish, 3); // all chunks arrived before the response

    for (int i = 0; i < 3; ++i) {
        const auto &[updateSessionId, update] = fixture.updates.at(i);
        QCOMPARE(updateSessionId, sessionId);
        QCOMPARE(update.kind(), "agent_message_chunk");
        const auto *chunk = update.get<ContentChunk>();
        QVERIFY(chunk);
        const auto *text = std::get_if<TextContent>(&chunk->content());
        QVERIFY(text);
        QCOMPARE(text->text(), QString("chunk-%1:hello").arg(i + 1));
    }
}

// A prompt streaming one session/update of every kind delivers them in order,
// each deserialized to the matching variant type.
void AcpClientTest::testE2eAllUpdateKinds()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--updates-all"});

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;

    bool promptDone = false;
    fixture.client.prompt(
        PromptRequest().sessionId(sessionId).addPrompt(TextContent().text("hello")),
        [&](const QJsonObject &, const std::optional<Error> &) { promptDone = true; });
    QTRY_VERIFY(promptDone);

    const QStringList expectedKinds{"plan", "tool_call", "tool_call_update",
                                    "agent_message_chunk", "available_commands_update",
                                    "usage_update"};
    QCOMPARE(fixture.updates.size(), expectedKinds.size());
    for (int i = 0; i < expectedKinds.size(); ++i)
        QCOMPARE(fixture.updates.at(i).second.kind(), expectedKinds.at(i));

    QVERIFY(fixture.updates.at(0).second.get<Plan>());
    QVERIFY(fixture.updates.at(1).second.get<ToolCall>());
    QCOMPARE(fixture.updates.at(1).second.get<ToolCall>()->toolCallId(), "tool-1");
    QVERIFY(fixture.updates.at(2).second.get<ToolCallUpdate>());
    QVERIFY(fixture.updates.at(3).second.get<ContentChunk>());
    QVERIFY(fixture.updates.at(4).second.get<AvailableCommandsUpdate>());
    QVERIFY(fixture.updates.at(5).second.get<UsageUpdate>());
    QCOMPARE(fixture.updates.at(5).second.get<UsageUpdate>()->used(), 100);
}

// A permission request during a prompt blocks the turn; selecting the allow
// option resumes streaming and finishes with end_turn.
void AcpClientTest::testE2ePermissionGranted()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--permission"});
    AcpPermissionHandler handler(&fixture.client);

    QList<QJsonValue> permissionIds;
    QList<RequestPermissionRequest> permissionRequests;
    connect(&handler, &AcpPermissionHandler::permissionRequested, &handler,
            [&](const QJsonValue &id, const RequestPermissionRequest &request) {
                permissionIds.append(id);
                permissionRequests.append(request);
            });

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;

    std::optional<StopReason> stopReason;
    fixture.client.prompt(
        PromptRequest().sessionId(sessionId).addPrompt(TextContent().text("do it")),
        [&](const QJsonObject &result, const std::optional<Error> &error) {
            QVERIFY(!error);
            if (const auto response = fromJson<PromptResponse>(QJsonValue(result)))
                stopReason = response->stopReason();
        });

    QTRY_COMPARE(permissionIds.size(), 1);
    QCOMPARE(permissionRequests.first().options().size(), 2);
    QCOMPARE(permissionRequests.first().options().at(0).optionId(), "allow");
    QCOMPARE(permissionRequests.first().options().at(1).optionId(), "reject");
    QVERIFY(!stopReason); // the prompt is blocked on the permission

    handler.sendPermissionResponse(permissionIds.first(), "allow");
    QTRY_VERIFY(stopReason.has_value());
    QCOMPARE(toString(*stopReason), toString(StopReason::end_turn));
    QCOMPARE(fixture.updates.size(), 2); // chunk before and after the permission
}

// Answering a permission request with the cancelled outcome finishes the prompt
// with stopReason cancelled.
void AcpClientTest::testE2ePermissionDenied()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--permission"});
    AcpPermissionHandler handler(&fixture.client);

    QList<QJsonValue> permissionIds;
    connect(&handler, &AcpPermissionHandler::permissionRequested, &handler,
            [&](const QJsonValue &id, const RequestPermissionRequest &) {
                permissionIds.append(id);
            });

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;

    std::optional<StopReason> stopReason;
    fixture.client.prompt(
        PromptRequest().sessionId(sessionId).addPrompt(TextContent().text("do it")),
        [&](const QJsonObject &result, const std::optional<Error> &error) {
            QVERIFY(!error);
            if (const auto response = fromJson<PromptResponse>(QJsonValue(result)))
                stopReason = response->stopReason();
        });

    QTRY_COMPARE(permissionIds.size(), 1);
    handler.sendPermissionCancelled(permissionIds.first());
    QTRY_VERIFY(stopReason.has_value());
    QCOMPARE(toString(*stopReason), toString(StopReason::cancelled));
}

// Cancelling the whole prompt while a permission request is pending makes the
// agent withdraw it via $/cancel_request; the client reports
// permissionCancelledByAgent and the turn ends cancelled.
void AcpClientTest::testE2ePermissionCancelledByAgent()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--permission"});
    AcpPermissionHandler handler(&fixture.client);

    QList<QJsonValue> permissionIds;
    QList<QJsonValue> cancelledIds;
    connect(&handler, &AcpPermissionHandler::permissionRequested, &handler,
            [&](const QJsonValue &id, const RequestPermissionRequest &) {
                permissionIds.append(id);
            });
    connect(&handler, &AcpPermissionHandler::permissionCancelledByAgent, &handler,
            [&](const QJsonValue &id) { cancelledIds.append(id); });

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;

    std::optional<StopReason> stopReason;
    fixture.client.prompt(
        PromptRequest().sessionId(sessionId).addPrompt(TextContent().text("do it")),
        [&](const QJsonObject &result, const std::optional<Error> &error) {
            QVERIFY(!error);
            if (const auto response = fromJson<PromptResponse>(QJsonValue(result)))
                stopReason = response->stopReason();
        });

    // Cancel the whole prompt while the permission request is pending: the
    // server withdraws its permission request via $/cancel_request.
    QTRY_COMPARE(permissionIds.size(), 1);
    fixture.client.cancelSession(CancelNotification().sessionId(sessionId));

    QTRY_COMPARE(cancelledIds.size(), 1);
    QCOMPARE(cancelledIds.first(), permissionIds.first());
    QTRY_VERIFY(stopReason.has_value());
    QCOMPARE(toString(*stopReason), toString(StopReason::cancelled));
}

// session/cancel during a streaming prompt finishes the turn with
// stopReason cancelled.
void AcpClientTest::testE2eCancelPrompt()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--cancel"});

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;

    std::optional<StopReason> stopReason;
    fixture.client.prompt(
        PromptRequest().sessionId(sessionId).addPrompt(TextContent().text("work")),
        [&](const QJsonObject &result, const std::optional<Error> &error) {
            QVERIFY(!error);
            if (const auto response = fromJson<PromptResponse>(QJsonValue(result)))
                stopReason = response->stopReason();
        });

    // Wait for the first streamed chunk, then cancel.
    QTRY_COMPARE(fixture.updates.size(), 1);
    QVERIFY(!stopReason);
    fixture.client.cancelSession(CancelNotification().sessionId(sessionId));

    QTRY_VERIFY(stopReason.has_value());
    QCOMPARE(toString(*stopReason), toString(StopReason::cancelled));
}

// session/new fails with Authentication_required until authenticate succeeds
// with the advertised method; a wrong method id fails.
void AcpClientTest::testE2eAuthentication()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--require-auth"});

    QVERIFY(fixture.initializeResponse->authMethods());
    QCOMPARE(fixture.initializeResponse->authMethods()->size(), 1);
    QCOMPARE(name(fixture.initializeResponse->authMethods()->first()), "Test Login");

    // session/new fails until authenticate succeeds.
    std::optional<Error> sessionError;
    bool sessionDone = false;
    fixture.client.newSession(NewSessionRequest().cwd(QDir::currentPath()),
                              [&](const QJsonObject &, const std::optional<Error> &error) {
                                  sessionDone = true;
                                  sessionError = error;
                              });
    QTRY_VERIFY(sessionDone);
    QVERIFY(sessionError);
    QCOMPARE(sessionError->code(), ErrorCode::Authentication_required);

    std::optional<Error> wrongMethodError;
    bool wrongMethodDone = false;
    fixture.client.authenticate(AuthenticateRequest().methodId("bogus"),
                                [&](const QJsonObject &, const std::optional<Error> &error) {
                                    wrongMethodDone = true;
                                    wrongMethodError = error;
                                });
    QTRY_VERIFY(wrongMethodDone);
    QVERIFY(wrongMethodError);

    std::optional<Error> authError;
    bool authDone = false;
    fixture.client.authenticate(AuthenticateRequest().methodId("test-login"),
                                [&](const QJsonObject &, const std::optional<Error> &error) {
                                    authDone = true;
                                    authError = error;
                                });
    QTRY_VERIFY(authDone);
    QVERIFY(!authError);

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;
    QVERIFY(!sessionId.isEmpty());
}

// Session lifecycle against the advertised capabilities: paginated session/list,
// session/load with history replay, load failure for unknown ids,
// session/delete, and session/close.
void AcpClientTest::testE2eSessionManagement()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--sessions", "5"});

    QVERIFY(fixture.initializeResponse->agentCapabilities());
    const auto &sessionCaps = fixture.initializeResponse->agentCapabilities()
                                  ->sessionCapabilities();
    QVERIFY(sessionCaps);
    QVERIFY(sessionCaps->list());
    QVERIFY(sessionCaps->delete_());
    QVERIFY(sessionCaps->close());

    // List all pages (page size 2, 5 seeded sessions).
    QStringList sessionIds;
    std::optional<QString> cursor;
    for (int page = 0; page < 3; ++page) {
        bool listDone = false;
        std::optional<QString> nextCursor;
        fixture.client.listSessions(
            ListSessionsRequest().cursor(cursor),
            [&](const QJsonObject &result, const std::optional<Error> &error) {
                listDone = true;
                QVERIFY(!error);
                const auto response = fromJson<ListSessionsResponse>(QJsonValue(result));
                QVERIFY(response.has_value());
                for (const SessionInfo &session : response->sessions())
                    sessionIds.append(session.sessionId());
                nextCursor = response->nextCursor();
            });
        QTRY_VERIFY(listDone);
        if (QTest::currentTestFailed())
            return;
        cursor = nextCursor;
        if (!cursor)
            break;
    }
    QCOMPARE(sessionIds.size(), 5);
    QCOMPARE(sessionIds.first(), "session-1");
    QCOMPARE(sessionIds.last(), "session-5");
    QVERIFY(!cursor);

    // Loading a known session replays history.
    bool loadDone = false;
    fixture.client.loadSession(
        LoadSessionRequest().sessionId("session-3").cwd(QDir::currentPath()),
        [&](const QJsonObject &, const std::optional<Error> &error) {
            loadDone = true;
            QVERIFY(!error);
        });
    QTRY_VERIFY(loadDone);
    QCOMPARE(fixture.updates.size(), 1);
    QCOMPARE(fixture.updates.first().first, "session-3");
    QCOMPARE(fixture.updates.first().second.kind(), "agent_message_chunk");

    // Loading an unknown session fails.
    std::optional<Error> loadError;
    bool loadUnknownDone = false;
    fixture.client.loadSession(
        LoadSessionRequest().sessionId("session-99").cwd(QDir::currentPath()),
        [&](const QJsonObject &, const std::optional<Error> &error) {
            loadUnknownDone = true;
            loadError = error;
        });
    QTRY_VERIFY(loadUnknownDone);
    QVERIFY(loadError);
    QCOMPARE(loadError->code(), ErrorCode::Resource_not_found);

    // Deleting removes the session from the list.
    bool deleteDone = false;
    fixture.client.deleteSession(DeleteSessionRequest().sessionId("session-2"),
                                 [&](const QJsonObject &, const std::optional<Error> &error) {
                                     deleteDone = true;
                                     QVERIFY(!error);
                                 });
    QTRY_VERIFY(deleteDone);

    QStringList remaining;
    bool relistDone = false;
    fixture.client.listSessions(
        ListSessionsRequest(),
        [&](const QJsonObject &result, const std::optional<Error> &error) {
            relistDone = true;
            QVERIFY(!error);
            if (const auto response = fromJson<ListSessionsResponse>(QJsonValue(result))) {
                for (const SessionInfo &session : response->sessions())
                    remaining.append(session.sessionId());
            }
        });
    QTRY_VERIFY(relistDone);
    QCOMPARE(remaining, QStringList({"session-1", "session-3"})); // first page only
    QVERIFY(!remaining.contains("session-2"));

    // Closing succeeds.
    bool closeDone = false;
    fixture.client.closeSession(CloseSessionRequest().sessionId("session-3"),
                                [&](const QJsonObject &, const std::optional<Error> &error) {
                                    closeDone = true;
                                    QVERIFY(!error);
                                });
    QTRY_VERIFY(closeDone);
}

// A server crash mid-prompt surfaces the stderr text in the transport error,
// disconnects, silently drops the pending request, and later sends fail
// gracefully.
void AcpClientTest::testE2eServerCrash()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--crash-on-prompt", "--stderr-noise"});

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;

    bool callbackInvoked = false;
    fixture.client.prompt(
        PromptRequest().sessionId(sessionId).addPrompt(TextContent().text("boom")),
        [&](const QJsonObject &, const std::optional<Error> &) { callbackInvoked = true; });

    QTRY_VERIFY(fixture.finishedCount > 0);
    QTRY_VERIFY(!fixture.transportErrors.isEmpty());
    QVERIFY2(fixture.transportErrors.first().contains("simulated crash"),
             qPrintable(fixture.transportErrors.join("; ")));
    QCOMPARE(fixture.client.state(), AcpClientObject::State::Disconnected);
    QVERIFY(!callbackInvoked); // pending requests are dropped silently

    // Sending after the server died reports an error instead of crashing.
    const int errorsBefore = fixture.transportErrors.size();
    fixture.client.prompt(PromptRequest().sessionId(sessionId), {});
    QTRY_COMPARE(fixture.transportErrors.size(), errorsBefore + 1);
    QVERIFY(fixture.transportErrors.last().contains("not running"));
}

// A non-JSON line in the middle of the stream is reported once as a parse error;
// the valid response after it still completes the request.
void AcpClientTest::testE2eInvalidResponseLine()
{
    ServerFixture fixture;
    START_SERVER(fixture, {"--invalid-response-on-prompt"});

    const QString sessionId = fixture.newSession();
    if (QTest::currentTestFailed())
        return;

    std::optional<StopReason> stopReason;
    fixture.client.prompt(
        PromptRequest().sessionId(sessionId).addPrompt(TextContent().text("hello")),
        [&](const QJsonObject &result, const std::optional<Error> &error) {
            QVERIFY(!error);
            if (const auto response = fromJson<PromptResponse>(QJsonValue(result)))
                stopReason = response->stopReason();
        });

    // The garbage line is reported as an error, the valid response after it
    // still completes the request.
    QTRY_VERIFY(stopReason.has_value());
    QCOMPARE(toString(*stopReason), toString(StopReason::end_turn));
    QCOMPARE(fixture.transportErrors.size(), 1);
    QVERIFY(fixture.transportErrors.first().contains("JSON parse error"));
}

// --- Tier 3 ------------------------------------------------------------------

// Connecting emits agent info and asks for session selection; creating a session
// reports its id, config options, and modes; mode and config changes round-trip
// through the server.
void AcpClientTest::testControllerConnectAndSession()
{
    ControllerFixture fixture;
    CONNECT_CONTROLLER(fixture, {"--config-options", "--modes"});

    QCOMPARE(fixture.agentName, "acptestserver");
    QCOMPARE(fixture.agentVersion, "1.0");
    QCOMPARE(fixture.sessionSelectionRequiredCount, 1);

    fixture.controller.createNewSession(Utils::FilePath::fromUserInput(QDir::currentPath()));
    QTRY_COMPARE(fixture.createdSessions.size(), 1);
    QCOMPARE(fixture.controller.sessionId(), fixture.createdSessions.first());

    QCOMPARE(fixture.configOptions.size(), 2);
    QCOMPARE(fixture.configOptions.first().id(), "test.autoApprove");
    QCOMPARE(fixture.configOptions.last().id(), "test.model");
    QCOMPARE(fixture.sessionModes.size(), 2);
    QCOMPARE(fixture.currentMode, "ask");

    // Changing the mode round-trips through the server.
    fixture.controller.setSessionMode("code");
    QTRY_COMPARE(fixture.currentMode, "code");

    // Changing a config option reports the new state.
    fixture.controller.setConfigOption("test.autoApprove", true);
    QTRY_VERIFY(Utils::anyOf(fixture.updates, [](const SessionUpdate &update) {
        return update.kind() == QLatin1String("config_option_update");
    }));
    QVERIFY(fixture.errors.isEmpty());
}

// sendPrompt() delivers the streamed chunks as sessionUpdate signals followed by
// promptFinished - the sequence AcpChatTab renders.
void AcpClientTest::testControllerPromptStreaming()
{
    ControllerFixture fixture;
    CONNECT_CONTROLLER(fixture);

    fixture.controller.createNewSession();
    QTRY_COMPARE(fixture.createdSessions.size(), 1);

    fixture.controller.sendPrompt("hello", {}, false);
    QTRY_COMPARE(fixture.promptFinishedCount, 1);
    QCOMPARE(fixture.updates.size(), 3);
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(fixture.updates.at(i).kind(), "agent_message_chunk");
        const auto *chunk = fixture.updates.at(i).get<ContentChunk>();
        QVERIFY(chunk);
        const auto *text = std::get_if<TextContent>(&chunk->content());
        QVERIFY(text);
        QCOMPARE(text->text(), QString("chunk-%1:hello").arg(i + 1));
    }
    QVERIFY(fixture.errors.isEmpty());
}

// Creating a session against an auth-requiring agent emits
// authenticationRequired; a failing authenticate reports authenticationFailed;
// a successful one creates the session.
void AcpClientTest::testControllerAuthentication()
{
    ControllerFixture fixture;
    CONNECT_CONTROLLER(fixture, {"--require-auth"});

    // Creating a session triggers the authentication workflow.
    fixture.controller.createNewSession();
    QTRY_COMPARE(fixture.authMethods.size(), 1);
    QCOMPARE(name(fixture.authMethods.first()), "Test Login");
    QVERIFY(fixture.createdSessions.isEmpty());

    // A failing authentication is reported.
    fixture.controller.authenticate("bogus");
    QTRY_COMPARE(fixture.authErrors.size(), 1);

    // A successful authentication creates the session.
    fixture.controller.authenticate("test-login");
    QTRY_COMPARE(fixture.createdSessions.size(), 1);
}

// supportsSessionList/Delete/Close reflect the capabilities advertised by the
// agent in the initialize response.
void AcpClientTest::testControllerCapabilityGating()
{
    {
        ControllerFixture fixture;
        CONNECT_CONTROLLER(fixture);
        QVERIFY(!fixture.controller.supportsSessionList());
        QVERIFY(!fixture.controller.supportsSessionDelete());
        QVERIFY(!fixture.controller.supportsSessionClose());
    }
    {
        ControllerFixture fixture;
        CONNECT_CONTROLLER(fixture, {"--sessions", "3"});
        QVERIFY(fixture.controller.supportsSessionList());
        QVERIFY(fixture.controller.supportsSessionDelete());
        QVERIFY(fixture.controller.supportsSessionClose());
    }
}

// The session picker workflows: paginated listSessions, loadSession with history
// replay, deleteSession, and closeSession which asks for a new session
// selection.
void AcpClientTest::testControllerSessionManagement()
{
    ControllerFixture fixture;
    CONNECT_CONTROLLER(fixture, {"--sessions", "5"});

    // Paginated listing, the way the session picker drives it.
    fixture.controller.listSessions();
    QTRY_COMPARE(fixture.listCount, 1);
    QCOMPARE(fixture.listedSessions.size(), 2);
    QVERIFY(fixture.lastCursor);

    fixture.controller.listSessions(fixture.lastCursor);
    QTRY_COMPARE(fixture.listCount, 2);
    QCOMPARE(fixture.listedSessions.size(), 4);

    // Loading a session replays its history.
    fixture.controller.loadSession("session-1",
                                   Utils::FilePath::fromUserInput(QDir::currentPath()));
    QTRY_COMPARE(fixture.loadedSessions.size(), 1);
    QCOMPARE(fixture.controller.sessionId(), "session-1");
    QTRY_VERIFY(!fixture.updates.isEmpty());
    QCOMPARE(fixture.updates.first().kind(), "agent_message_chunk");

    // Deleting.
    fixture.controller.deleteSession("session-2");
    QTRY_COMPARE(fixture.deletedSessions, QStringList("session-2"));

    // Closing ends the session and asks for a new selection.
    const int selectionsBefore = fixture.sessionSelectionRequiredCount;
    fixture.controller.closeSession();
    QTRY_COMPARE(fixture.closedSessions, QStringList("session-1"));
    QVERIFY(fixture.controller.sessionId().isEmpty());
    QCOMPARE(fixture.sessionSelectionRequiredCount, selectionsBefore + 1);
    QVERIFY(fixture.errors.isEmpty());
}

// cancelPrompt() during a streaming prompt leads to promptFinished without
// errors.
void AcpClientTest::testControllerCancelPrompt()
{
    ControllerFixture fixture;
    CONNECT_CONTROLLER(fixture, {"--cancel"});

    fixture.controller.createNewSession();
    QTRY_COMPARE(fixture.createdSessions.size(), 1);

    fixture.controller.sendPrompt("work", {}, false);
    QTRY_COMPARE(fixture.updates.size(), 1);
    QCOMPARE(fixture.promptFinishedCount, 0);

    fixture.controller.cancelPrompt();
    QTRY_COMPARE(fixture.promptFinishedCount, 1);
    QVERIFY(fixture.errors.isEmpty());
}

// A server crash mid-prompt is reported through errorOccurred and results in a
// disconnected state.
void AcpClientTest::testControllerServerCrash()
{
    ControllerFixture fixture;
    CONNECT_CONTROLLER(fixture, {"--crash-on-prompt"});

    fixture.controller.createNewSession();
    QTRY_COMPARE(fixture.createdSessions.size(), 1);

    fixture.controller.sendPrompt("boom", {}, false);
    QTRY_VERIFY(!fixture.errors.isEmpty());
    QTRY_VERIFY(fixture.states.contains(AcpClientObject::State::Disconnected));
}

// disconnectFromServer() emits the disconnected state and fully resets the
// controller.
void AcpClientTest::testControllerDisconnect()
{
    ControllerFixture fixture;
    CONNECT_CONTROLLER(fixture);

    fixture.controller.createNewSession();
    QTRY_COMPARE(fixture.createdSessions.size(), 1);

    fixture.controller.disconnectFromServer();
    QVERIFY(fixture.states.contains(AcpClientObject::State::Disconnected));
    QVERIFY(!fixture.controller.isInitialized());
    QVERIFY(fixture.controller.sessionId().isEmpty());
}

QObject *createAcpClientTest()
{
    return new AcpClientTest;
}

} // namespace AcpClient::Internal

#include "acpclienttest.moc"
