// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <acp/acp.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Acp;

// Helper: parse a JSON string into a QJsonObject
static QJsonObject jsonObj(const QByteArray &json)
{
    return QJsonDocument::fromJson(json).object();
}

// Helper: compare a struct's round-trip: json -> fromJson -> toJson -> compare with original
// For synchronous fromJson (returns Utils::Result<T> directly)
template<typename T>
void verifyRoundtrip(const QJsonObject &original)
{
    auto parsed = fromJson<T>(QJsonValue(original));
    QVERIFY_RESULT(parsed);
    QJsonObject serialized = toJson(*parsed);
    QCOMPARE(serialized, original);
}

class tst_Acp : public QObject
{
    Q_OBJECT

private slots:
    // --- Enums ---
    void roleEnum();
    void roleEnumInvalid();
    void sessionConfigOptionCategoryEnum();
    void toolKindEnum();
    void toolKindDeleteKeyword();
    void toolCallStatusEnum();
    void planEntryPriorityEnum();
    void planEntryStatusEnum();
    void stopReasonEnum();
    void permissionOptionKindEnum();
    void enumInvalidType();

    // --- Simple type aliases ---
    void sessionIdAlias();
    void errorCodeAlias();
    void protocolVersionAlias();

    // --- Variant types ---
    void requestIdNull();
    void requestIdInt();
    void requestIdString();
    void requestIdInvalid();
    void embeddedResourceResourceText();
    void embeddedResourceResourceBlob();

    // --- Structs: all-optional fields ---
    void mcpCapabilitiesEmpty();
    void mcpCapabilitiesFull();
    void mcpCapabilitiesRoundtrip();
    void annotationsRoundtrip();

    // --- Structs: required + optional fields ---
    void textContentRoundtrip();
    void textContentMissingRequired();
    void textContentNotObject();
    void audioContentRoundtrip();
    void imageContentRoundtrip();
    void imageContentOptionalUri();

    // --- Structs with arrays ---
    void availableCommandRoundtrip();
    void availableCommandsUpdateRoundtrip();

    // --- Struct with nested types ---
    void toolCallLocationRoundtrip();
    void envVariableRoundtrip();

    // --- Config types ---
    void sessionConfigSelectOptionRoundtrip();
    void sessionConfigSelectGroupRoundtrip();

    // --- Content types ---
    void contentRoundtrip();
    void diffRoundtrip();
    void terminalRoundtrip();

    // --- Plan types ---
    void planEntryRoundtrip();
    void planRoundtrip();

    // --- Session types ---
    void contentChunkRoundtrip();
    void sessionModeRoundtrip();

    // --- Builder pattern ---
    void builderPatternTextContent();
    void builderPatternToolCall();

    // --- Meta field ---
    void metaFieldRoundtrip();

    // --- Error handling ---
    void fromJsonWrongType();
    void fromJsonMissingMultipleRequired();
    void emptyObjectForRequiredFields();
};

// =============================================================================
// Enums
// =============================================================================

void tst_Acp::roleEnum()
{
    // toString
    QCOMPARE(toString(Role::assistant), "assistant");
    QCOMPARE(toString(Role::user), "user");

    // fromJson
    auto r1 = fromJson<Role>(QJsonValue("assistant"));
    QVERIFY(r1.has_value());
    QVERIFY(*r1 == Role::assistant);

    auto r2 = fromJson<Role>(QJsonValue("user"));
    QVERIFY(r2.has_value());
    QVERIFY(*r2 == Role::user);

    // toJsonValue
    QCOMPARE(toJsonValue(Role::assistant), QJsonValue("assistant"));
    QCOMPARE(toJsonValue(Role::user), QJsonValue("user"));
}

void tst_Acp::roleEnumInvalid()
{
    auto r = fromJson<Role>(QJsonValue("invalid_role"));
    QVERIFY(!r.has_value());
}

void tst_Acp::sessionConfigOptionCategoryEnum()
{
    QCOMPARE(toString(SessionConfigOptionCategory::mode), "mode");
    QCOMPARE(toString(SessionConfigOptionCategory::model), "model");
    QCOMPARE(toString(SessionConfigOptionCategory::thought_level), "thought_level");

    auto r = fromJson<SessionConfigOptionCategory>(QJsonValue("model"));
    QVERIFY_RESULT(r);
    QVERIFY(*r == SessionConfigOptionCategory::model);
}

void tst_Acp::toolKindEnum()
{
    QCOMPARE(toString(ToolKind::read), "read");
    QCOMPARE(toString(ToolKind::edit), "edit");
    QCOMPARE(toString(ToolKind::execute), "execute");

    auto r = fromJson<ToolKind>(QJsonValue("search"));
    QVERIFY_RESULT(r);
    QVERIFY(*r == ToolKind::search);
}

void tst_Acp::toolKindDeleteKeyword()
{
    // "delete" is a C++ keyword, escaped as delete_ in the enum
    QCOMPARE(toString(ToolKind::delete_), "delete");

    auto r = fromJson<ToolKind>(QJsonValue("delete"));
    QVERIFY_RESULT(r);
    QVERIFY(*r == ToolKind::delete_);

    QCOMPARE(toJsonValue(ToolKind::delete_), QJsonValue("delete"));
}

void tst_Acp::toolCallStatusEnum()
{
    QCOMPARE(toString(ToolCallStatus::pending), "pending");
    QCOMPARE(toString(ToolCallStatus::in_progress), "in_progress");
    QCOMPARE(toString(ToolCallStatus::completed), "completed");
    QCOMPARE(toString(ToolCallStatus::failed), "failed");
}

void tst_Acp::planEntryPriorityEnum()
{
    auto r = fromJson<PlanEntryPriority>(QJsonValue("high"));
    QVERIFY_RESULT(r);
    QVERIFY(*r == PlanEntryPriority::high);
    QCOMPARE(toJsonValue(PlanEntryPriority::medium), QJsonValue("medium"));
}

void tst_Acp::planEntryStatusEnum()
{
    auto r = fromJson<PlanEntryStatus>(QJsonValue("completed"));
    QVERIFY_RESULT(r);
    QVERIFY(*r == PlanEntryStatus::completed);
}

void tst_Acp::stopReasonEnum()
{
    auto r = fromJson<StopReason>(QJsonValue("end_turn"));
    QVERIFY_RESULT(r);
    QVERIFY(*r == StopReason::end_turn);
    QCOMPARE(toString(StopReason::max_tokens), "max_tokens");
}

void tst_Acp::permissionOptionKindEnum()
{
    auto r = fromJson<PermissionOptionKind>(QJsonValue("allow_once"));
    QVERIFY_RESULT(r);
    QVERIFY(*r == PermissionOptionKind::allow_once);
    QCOMPARE(toString(PermissionOptionKind::reject_always), "reject_always");
}

void tst_Acp::enumInvalidType()
{
    // Passing a number where a string enum is expected
    auto r = fromJson<Role>(QJsonValue(42));
    QVERIFY(!r.has_value());
}

// =============================================================================
// Simple type aliases
// =============================================================================

void tst_Acp::sessionIdAlias()
{
    auto r = fromJson<SessionId>(QJsonValue("sess-123"));
    QVERIFY_RESULT(r);
    QCOMPARE(*r, QString("sess-123"));
}

void tst_Acp::errorCodeAlias()
{
    // ErrorCode is just an int alias
    ErrorCode code = -32700;
    QCOMPARE(code, -32700);
}

void tst_Acp::protocolVersionAlias()
{
    auto r = fromJson<ProtocolVersion>(QJsonValue(1));
    QVERIFY_RESULT(r);
    QCOMPARE(*r, 1);
}

// =============================================================================
// Variant types
// =============================================================================

void tst_Acp::requestIdNull()
{
    auto r = fromJson<RequestId>(QJsonValue(QJsonValue::Null));
    QVERIFY_RESULT(r);
    QVERIFY(std::holds_alternative<std::monostate>(*r));
    QCOMPARE(toJsonValue(*r), QJsonValue(QJsonValue::Null));
}

void tst_Acp::requestIdInt()
{
    auto r = fromJson<RequestId>(QJsonValue(42));
    QVERIFY_RESULT(r);
    QVERIFY(std::holds_alternative<int>(*r));
    QCOMPARE(std::get<int>(*r), 42);
    QCOMPARE(toJsonValue(*r), QJsonValue(42));
}

void tst_Acp::requestIdString()
{
    auto r = fromJson<RequestId>(QJsonValue("req-abc"));
    QVERIFY_RESULT(r);
    QVERIFY(std::holds_alternative<QString>(*r));
    QCOMPARE(std::get<QString>(*r), "req-abc");
    QCOMPARE(toJsonValue(*r), QJsonValue("req-abc"));
}

void tst_Acp::requestIdInvalid()
{
    auto r = fromJson<RequestId>(QJsonValue(true));
    QVERIFY(!r.has_value());
}

void tst_Acp::embeddedResourceResourceText()
{
    const QJsonObject json = jsonObj(R"({
        "text": "file contents",
        "uri": "file:///test.txt"
    })");
    auto r = fromJson<EmbeddedResourceResource>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QVERIFY(std::holds_alternative<TextResourceContents>(*r));
    const auto &trc = std::get<TextResourceContents>(*r);
    QCOMPARE(trc._text, "file contents");
    QCOMPARE(trc._uri, "file:///test.txt");
}

void tst_Acp::embeddedResourceResourceBlob()
{
    const QJsonObject json = jsonObj(R"({
        "blob": "AQIDBA==",
        "uri": "file:///data.bin"
    })");
    auto r = fromJson<EmbeddedResourceResource>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QVERIFY(std::holds_alternative<BlobResourceContents>(*r));
    const auto &brc = std::get<BlobResourceContents>(*r);
    QCOMPARE(brc._blob, "AQIDBA==");
    QCOMPARE(brc._uri, "file:///data.bin");
}

// =============================================================================
// Structs: all-optional fields
// =============================================================================

void tst_Acp::mcpCapabilitiesEmpty()
{
    const QJsonObject json = jsonObj("{}");
    auto r = fromJson<McpCapabilities>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QVERIFY(!r->http().has_value());
    QVERIFY(!r->sse().has_value());
    QVERIFY(!r->_meta().has_value());
    QCOMPARE(toJson(*r), QJsonObject());
}

void tst_Acp::mcpCapabilitiesFull()
{
    const QJsonObject json = jsonObj(R"({
        "http": true,
        "sse": false,
        "_meta": {"version": "1.0"}
    })");
    auto r = fromJson<McpCapabilities>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(*r->http(), true);
    QCOMPARE(*r->sse(), false);
    QVERIFY(r->_meta().has_value());
    QCOMPARE(r->_meta()->value("version").toString(), "1.0");
}

void tst_Acp::mcpCapabilitiesRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "http": true,
        "sse": false,
        "_meta": {"key": "value"}
    })");
    verifyRoundtrip<McpCapabilities>(json);
}

void tst_Acp::annotationsRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "audience": ["user"],
        "lastModified": "2025-01-01T00:00:00Z",
        "priority": 0.8
    })");
    auto r = fromJson<Annotations>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->audience()->size(), 1);
    QCOMPARE(*r->lastModified(), "2025-01-01T00:00:00Z");
    QVERIFY(qFuzzyCompare(*r->priority(), 0.8));
    verifyRoundtrip<Annotations>(json);
}

// =============================================================================
// Structs: required + optional fields
// =============================================================================

void tst_Acp::textContentRoundtrip()
{
    const QJsonObject json = jsonObj(R"({"text": "Hello, world!"})");
    auto r = fromJson<TextContent>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->text(), "Hello, world!");
    QVERIFY(!r->annotations().has_value());
    verifyRoundtrip<TextContent>(json);

    // With optional annotations
    const QJsonObject json2 = jsonObj(R"({"text": "Hi", "annotations": "some-ref"})");
    verifyRoundtrip<TextContent>(json2);
}

void tst_Acp::textContentMissingRequired()
{
    // Missing "text" field
    const QJsonObject json = jsonObj(R"({"annotations": "ref"})");
    auto r = fromJson<TextContent>(QJsonValue(json));
    QVERIFY(!r.has_value());
}

void tst_Acp::textContentNotObject()
{
    auto r = fromJson<TextContent>(QJsonValue("not an object"));
    QVERIFY(!r.has_value());

    auto r2 = fromJson<TextContent>(QJsonValue(42));
    QVERIFY(!r2.has_value());

    auto r3 = fromJson<TextContent>(QJsonValue(QJsonValue::Null));
    QVERIFY(!r3.has_value());
}

void tst_Acp::audioContentRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "data": "base64audiodata==",
        "mimeType": "audio/wav"
    })");
    auto r = fromJson<AudioContent>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->data(), "base64audiodata==");
    QCOMPARE(r->mimeType(), "audio/wav");
    verifyRoundtrip<AudioContent>(json);
}

void tst_Acp::imageContentRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "data": "base64imgdata==",
        "mimeType": "image/png"
    })");
    auto r = fromJson<ImageContent>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->data(), "base64imgdata==");
    QCOMPARE(r->mimeType(), "image/png");
    QVERIFY(!r->uri().has_value());
    verifyRoundtrip<ImageContent>(json);
}

void tst_Acp::imageContentOptionalUri()
{
    const QJsonObject json = jsonObj(R"({
        "data": "base64imgdata==",
        "mimeType": "image/png",
        "uri": "https://example.com/img.png"
    })");
    auto r = fromJson<ImageContent>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(*r->uri(), "https://example.com/img.png");
    verifyRoundtrip<ImageContent>(json);
}

// =============================================================================
// Structs with arrays
// =============================================================================

void tst_Acp::availableCommandRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "description": "Run tests",
        "name": "/test"
    })");
    auto r = fromJson<AvailableCommand>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->name(), "/test");
    QCOMPARE(r->description(), "Run tests");
    verifyRoundtrip<AvailableCommand>(json);
}

void tst_Acp::availableCommandsUpdateRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "availableCommands": [
            {"description": "Build project", "name": "/build"},
            {"description": "Run all tests", "name": "/test"}
        ]
    })");
    auto r = fromJson<AvailableCommandsUpdate>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->availableCommands().size(), 2);
    QCOMPARE(r->availableCommands().at(0)._name, "/build");
    QCOMPARE(r->availableCommands().at(1)._name, "/test");
    verifyRoundtrip<AvailableCommandsUpdate>(json);
}

// =============================================================================
// Struct with nested types
// =============================================================================

void tst_Acp::toolCallLocationRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "path": "/src/main.cpp",
        "line": 42
    })");
    auto r = fromJson<ToolCallLocation>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->path(), "/src/main.cpp");
    QCOMPARE(*r->line(), 42);
    verifyRoundtrip<ToolCallLocation>(json);
}

void tst_Acp::envVariableRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "name": "PATH",
        "value": "/usr/bin"
    })");
    auto r = fromJson<EnvVariable>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->name(), "PATH");
    QCOMPARE(r->value(), "/usr/bin");
    verifyRoundtrip<EnvVariable>(json);
}

// =============================================================================
// Config types
// =============================================================================

void tst_Acp::sessionConfigSelectOptionRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "name": "GPT-4",
        "value": "gpt-4",
        "description": "Most capable model"
    })");
    auto r = fromJson<SessionConfigSelectOption>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->name(), "GPT-4");
    QCOMPARE(r->value(), "gpt-4");
    QCOMPARE(*r->description(), "Most capable model");
    verifyRoundtrip<SessionConfigSelectOption>(json);
}

void tst_Acp::sessionConfigSelectGroupRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "group": "premium",
        "name": "Premium Models",
        "options": [
            {"name": "Model A", "value": "a"},
            {"name": "Model B", "value": "b"}
        ]
    })");
    auto r = fromJson<SessionConfigSelectGroup>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->group(), "premium");
    QCOMPARE(r->options().size(), 2);
    QCOMPARE(r->options().at(0)._name, "Model A");
    verifyRoundtrip<SessionConfigSelectGroup>(json);
}

// =============================================================================
// Content types (used in ToolCallContent variant)
// =============================================================================

void tst_Acp::contentRoundtrip()
{
    const QJsonObject json = jsonObj(R"({"content": "Some output text"})");
    auto r = fromJson<Content>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->content(), "Some output text");
    verifyRoundtrip<Content>(json);
}

void tst_Acp::diffRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "newText": "new content",
        "oldText": "old content",
        "path": "/src/file.cpp"
    })");
    auto r = fromJson<Diff>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->newText(), "new content");
    QCOMPARE(*r->oldText(), "old content");
    QCOMPARE(r->path(), "/src/file.cpp");
    verifyRoundtrip<Diff>(json);
}

void tst_Acp::terminalRoundtrip()
{
    const QJsonObject json = jsonObj(R"({"terminalId": "term-abc"})");
    auto r = fromJson<Terminal>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->terminalId(), "term-abc");
    verifyRoundtrip<Terminal>(json);
}

// =============================================================================
// Plan types
// =============================================================================

void tst_Acp::planEntryRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "content": "Implement feature X",
        "priority": "high",
        "status": "pending"
    })");
    auto r = fromJson<PlanEntry>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->content(), "Implement feature X");
    QCOMPARE(r->priority(), "high");
    QCOMPARE(r->status(), "pending");
    verifyRoundtrip<PlanEntry>(json);
}

void tst_Acp::planRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "entries": [
            {"content": "Step 1", "priority": "high", "status": "completed"},
            {"content": "Step 2", "priority": "low", "status": "pending"}
        ]
    })");
    auto r = fromJson<Plan>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->entries().size(), 2);
    QCOMPARE(r->entries().at(0)._content, "Step 1");
    QCOMPARE(r->entries().at(1)._status, "pending");
    verifyRoundtrip<Plan>(json);
}

// =============================================================================
// Session types
// =============================================================================

void tst_Acp::contentChunkRoundtrip()
{
    const QJsonObject json = jsonObj(R"({"content": "streaming chunk"})");
    auto r = fromJson<ContentChunk>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->content(), "streaming chunk");
    verifyRoundtrip<ContentChunk>(json);
}

void tst_Acp::sessionModeRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "id": "mode-code",
        "name": "Code Mode",
        "description": "Optimized for coding tasks"
    })");
    auto r = fromJson<SessionMode>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QCOMPARE(r->id(), "mode-code");
    QCOMPARE(r->name(), "Code Mode");
    QCOMPARE(*r->description(), "Optimized for coding tasks");
    verifyRoundtrip<SessionMode>(json);
}

// =============================================================================
// Builder pattern
// =============================================================================

void tst_Acp::builderPatternTextContent()
{
    auto tc = TextContent().text("hello").annotations("ann-ref");
    QCOMPARE(tc.text(), "hello");
    QCOMPARE(*tc.annotations(), "ann-ref");

    // Serialize and verify
    QJsonObject json = toJson(tc);
    QCOMPARE(json["text"].toString(), "hello");
    QCOMPARE(json["annotations"].toString(), "ann-ref");
}

void tst_Acp::builderPatternToolCall()
{
    auto tc = ToolCall()
                  .toolCallId("tc-1")
                  .title("Reading file")
                  .kind("read")
                  .status("in_progress");
    QCOMPARE(tc.toolCallId(), "tc-1");
    QCOMPARE(tc.title(), "Reading file");
    QCOMPARE(*tc.kind(), "read");
    QCOMPARE(*tc.status(), "in_progress");
}

// =============================================================================
// Meta field
// =============================================================================

void tst_Acp::metaFieldRoundtrip()
{
    const QJsonObject json = jsonObj(R"({
        "text": "hello",
        "_meta": {"custom_key": 42, "nested": {"a": true}}
    })");
    auto r = fromJson<TextContent>(QJsonValue(json));
    QVERIFY_RESULT(r);
    QVERIFY(r->_meta().has_value());
    QCOMPARE(r->_meta()->value("custom_key").toInt(), 42);
    QVERIFY(r->_meta()->value("nested").toObject().value("a").toBool());
    verifyRoundtrip<TextContent>(json);
}

// =============================================================================
// Error handling
// =============================================================================

void tst_Acp::fromJsonWrongType()
{
    // String where object expected
    auto r1 = fromJson<McpCapabilities>(QJsonValue("string"));
    QVERIFY(!r1.has_value());

    // Number where object expected
    auto r2 = fromJson<TextContent>(QJsonValue(123));
    QVERIFY(!r2.has_value());

    // Array where object expected
    auto r3 = fromJson<AudioContent>(QJsonValue(QJsonArray()));
    QVERIFY(!r3.has_value());

    // Bool where object expected
    auto r4 = fromJson<EnvVariable>(QJsonValue(true));
    QVERIFY(!r4.has_value());

    // Null where object expected
    auto r5 = fromJson<Diff>(QJsonValue(QJsonValue::Null));
    QVERIFY(!r5.has_value());
}

void tst_Acp::fromJsonMissingMultipleRequired()
{
    // AudioContent requires both "data" and "mimeType"
    {
        auto r = fromJson<AudioContent>(QJsonValue(jsonObj(R"({"data": "abc"})")));
        QVERIFY(!r.has_value());
    }
    {
        auto r = fromJson<AudioContent>(QJsonValue(jsonObj(R"({"mimeType": "audio/wav"})")));
        QVERIFY(!r.has_value());
    }
    {
        auto r = fromJson<AudioContent>(QJsonValue(jsonObj("{}")));
        QVERIFY(!r.has_value());
    }
}

void tst_Acp::emptyObjectForRequiredFields()
{
    // Structs with required fields should fail on empty object
    auto r1 = fromJson<TextContent>(QJsonValue(jsonObj("{}")));
    QVERIFY(!r1.has_value());

    auto r2 = fromJson<EnvVariable>(QJsonValue(jsonObj("{}")));
    QVERIFY(!r2.has_value());

    // Structs with all-optional fields should succeed on empty object
    auto r3 = fromJson<McpCapabilities>(QJsonValue(jsonObj("{}")));
    QVERIFY(r3.has_value());

    auto r4 = fromJson<Annotations>(QJsonValue(jsonObj("{}")));
    QVERIFY(r4.has_value());
}

QTEST_GUILESS_MAIN(tst_Acp)

#include "tst_acp.moc"
