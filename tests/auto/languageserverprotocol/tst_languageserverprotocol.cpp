// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <languageserverprotocol/lspbasemessage.h>
#include <languageserverprotocol/lspjsonrpc.h>
#include <languageserverprotocol/lspmessages.h>
#include <languageserverprotocol/lsptypes.h>
#include <languageserverprotocol/lsputils.h>

#include <utils/filepath.h>

#include <QBuffer>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using namespace LanguageServerProtocol;

class tst_LanguageServerProtocol : public QObject
{
    Q_OBJECT

private slots:
    void baseMessageRoundTrip();
    void baseMessageParseIncomplete();
    void messageIdRoundTrip();
    void requestObject();
    void notificationObject();
    void responseObject();
    void errorResponse();
    void providerUnion();
    void diagnosticCode();
    void completionItemDocumentation();
    void textDocumentSyncUnion();
    void definitionResult();
    void positionRoundTrip();
    void wireRoundTrip();
    void nonObjectInput_data();
    void nonObjectInput();
    void missingRequiredFields();
    void unknownFieldsAndValues();
    void nestedParseErrorPropagates();

    void uriToPath_data();
    void uriToPath();
    void uriRoundTrip_data();
    void uriRoundTrip();
    void cursorPositions();
    void positionWithOffset();
    void selectionRange();
    void malformedFieldTolerated();
    void enumValues();
    void constFieldMismatch();
    void discriminatedUnion();
    void untaggedUnionMismatch();
    void documentSelector();
    void workspaceEditChanges();
    void recursiveType();
    void nullPayload();
    void malformedRequestParams();
    void messageIdFromUnexpectedJson();
    void baseMessageInvalidContentLength();
    void baseMessageUnexpectedHeader();
    void fromBaseMessageErrors();
};

void tst_LanguageServerProtocol::baseMessageRoundTrip()
{
    const QJsonObject message{{"jsonrpc", "2.0"}, {"method", "initialized"}};
    const BaseMessage baseMessage = toBaseMessage(message);
    QVERIFY(baseMessage.isComplete());
    QVERIFY(baseMessage.isValid());

    const Utils::Result<QJsonObject> parsed = fromBaseMessage(baseMessage);
    QVERIFY_RESULT(parsed);
    QCOMPARE(*parsed, message);
}

void tst_LanguageServerProtocol::baseMessageParseIncomplete()
{
    const BaseMessage complete = toBaseMessage({{"jsonrpc", "2.0"}, {"method", "exit"}});
    QByteArray data = complete.header() + complete.content;
    data.chop(2);

    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QString parseError;
    BaseMessage message;
    BaseMessage::parse(&buffer, parseError, message);
    QVERIFY(parseError.isEmpty());
    QVERIFY(!message.isComplete());
}

void tst_LanguageServerProtocol::messageIdRoundTrip()
{
    const MessageId intId(42);
    QCOMPARE(intId.toJsonValue().toInt(), 42);
    QCOMPARE(MessageId(intId.toJsonValue()), intId);

    const MessageId stringId(QString("abc"));
    QCOMPARE(stringId.toJsonValue().toString(), QString("abc"));
    QCOMPARE(MessageId(stringId.toJsonValue()), stringId);

    QVERIFY(!(intId == stringId));
    QVERIFY(!MessageId().isValid());
}

void tst_LanguageServerProtocol::requestObject()
{
    HoverParams params;
    params.textDocument(TextDocumentIdentifier().uri("file:///tmp/test.cpp"));
    params.position(Position().line(2).character(4));

    const QJsonObject request
        = LanguageServerProtocol::requestObject<HoverRequest>(MessageId(7), params);
    QCOMPARE(messageMethod(request), QString(HoverRequest::method));
    QCOMPARE(messageId(request), MessageId(7));

    const Utils::Result<HoverParams> parsed = LanguageServerProtocol::params<HoverRequest>(request);
    QVERIFY_RESULT(parsed);
    QCOMPARE(parsed->position().line(), 2);
    QCOMPARE(parsed->textDocument().uri(), QString("file:///tmp/test.cpp"));
}

void tst_LanguageServerProtocol::notificationObject()
{
    DidCloseTextDocumentParams params;
    params.textDocument(TextDocumentIdentifier().uri("file:///tmp/test.cpp"));

    const QJsonObject notification
        = LanguageServerProtocol::notificationObject<DidCloseTextDocumentNotification>(params);
    QCOMPARE(messageMethod(notification), QString(DidCloseTextDocumentNotification::method));
    QVERIFY(!messageId(notification).isValid());
}

void tst_LanguageServerProtocol::responseObject()
{
    const QList<Location> locations{
        Location()
            .uri("file:///tmp/test.cpp")
            .range(
                Range().start(Position().line(1).character(0)).end(Position().line(1).character(5)))};
    const QJsonObject response = LanguageServerProtocol::responseObject<ReferencesRequest>(
        MessageId(QString("id")), locations);

    const Utils::Result<ReferencesRequestResult> parsed
        = LanguageServerProtocol::result<ReferencesRequest>(response);
    QVERIFY_RESULT(parsed);
    const auto parsedLocations = std::get_if<QList<Location>>(&*parsed);
    QVERIFY(parsedLocations);
    QCOMPARE(*parsedLocations, locations);
}

void tst_LanguageServerProtocol::errorResponse()
{
    const ResponseError error{-32601, "Method not found", {}};
    const QJsonObject response = errorResponseObject(MessageId(1), error);
    QCOMPARE(ResponseError::fromJsonObject(response.value("error").toObject()).code, error.code);

    const Utils::Result<HoverRequestResult> parsed = LanguageServerProtocol::result<HoverRequest>(
        response);
    QVERIFY(!parsed);
    QCOMPARE(parsed.error(), error.message);
}

void tst_LanguageServerProtocol::providerUnion()
{
    const QJsonObject flagCapabilities{{"hoverProvider", true}};
    const Utils::Result<ServerCapabilities> flagParsed = fromJson<ServerCapabilities>(
        flagCapabilities);
    QVERIFY_RESULT(flagParsed);
    QVERIFY(flagParsed->hoverProvider().has_value());
    QCOMPARE(std::get<bool>(*flagParsed->hoverProvider()), true);

    const QJsonObject optionCapabilities{{"hoverProvider", QJsonObject{{"workDoneProgress", true}}}};
    const Utils::Result<ServerCapabilities> optionParsed = fromJson<ServerCapabilities>(
        optionCapabilities);
    QVERIFY_RESULT(optionParsed);
    QVERIFY(optionParsed->hoverProvider().has_value());
    const auto options = std::get_if<HoverOptions>(&*optionParsed->hoverProvider());
    QVERIFY(options);
    QCOMPARE(options->workDoneProgress().value_or(false), true);
}

void tst_LanguageServerProtocol::diagnosticCode()
{
    const QJsonObject stringCode{{"range", toJson(Range())}, {"message", "msg"}, {"code", "-Wall"}};
    const Utils::Result<Diagnostic> stringParsed = fromJson<Diagnostic>(stringCode);
    QVERIFY_RESULT(stringParsed);
    QVERIFY(stringParsed->code().has_value());
    QCOMPARE(std::get<QString>(*stringParsed->code()), QString("-Wall"));

    const QJsonObject intCode{{"range", toJson(Range())}, {"message", "msg"}, {"code", 12}};
    const Utils::Result<Diagnostic> intParsed = fromJson<Diagnostic>(intCode);
    QVERIFY_RESULT(intParsed);
    QVERIFY(intParsed->code().has_value());
    QCOMPARE(std::get<int>(*intParsed->code()), 12);
}

void tst_LanguageServerProtocol::completionItemDocumentation()
{
    const QJsonObject plain{{"label", "foo"}, {"documentation", "doc"}};
    const Utils::Result<CompletionItem> plainParsed = fromJson<CompletionItem>(plain);
    QVERIFY_RESULT(plainParsed);
    QVERIFY(plainParsed->documentation().has_value());
    QCOMPARE(std::get<QString>(*plainParsed->documentation()), QString("doc"));

    const QJsonObject markup{
        {"label", "foo"},
        {"documentation", QJsonObject{{"kind", "markdown"}, {"value", "**doc**"}}}};
    const Utils::Result<CompletionItem> markupParsed = fromJson<CompletionItem>(markup);
    QVERIFY_RESULT(markupParsed);
    QVERIFY(markupParsed->documentation().has_value());
    const auto content = std::get_if<MarkupContent>(&*markupParsed->documentation());
    QVERIFY(content);
    QCOMPARE(content->value(), QString("**doc**"));
}

void tst_LanguageServerProtocol::definitionResult()
{
    const QJsonObject location{{"uri", "file:///tmp/test.cpp"}, {"range", toJson(Range())}};

    const Utils::Result<DefinitionRequestResult> single = fromJson<DefinitionRequestResult>(
        location);
    QVERIFY_RESULT(single);
    const auto singleDefinition = std::get_if<Definition>(&*single);
    QVERIFY(singleDefinition);
    QVERIFY(std::holds_alternative<Location>(*singleDefinition));

    const Utils::Result<DefinitionRequestResult> list = fromJson<DefinitionRequestResult>(
        QJsonArray{location});
    QVERIFY_RESULT(list);
    const auto listDefinition = std::get_if<Definition>(&*list);
    QVERIFY(listDefinition);
    const auto locations = std::get_if<QList<Location>>(listDefinition);
    QVERIFY(locations);
    QCOMPARE(locations->size(), 1);
    QCOMPARE(locations->first().uri(), QString("file:///tmp/test.cpp"));
}

void tst_LanguageServerProtocol::textDocumentSyncUnion()
{
    const QJsonObject kind{{"textDocumentSync", 2}};
    const Utils::Result<ServerCapabilities> kindParsed = fromJson<ServerCapabilities>(kind);
    QVERIFY_RESULT(kindParsed);
    QVERIFY(kindParsed->textDocumentSync().has_value());
    QCOMPARE(std::get<int>(*kindParsed->textDocumentSync()), TextDocumentSyncKind::Incremental);

    const QJsonObject options{
        {"textDocumentSync", QJsonObject{{"openClose", true}, {"save", true}}}};
    const Utils::Result<ServerCapabilities> optionsParsed = fromJson<ServerCapabilities>(options);
    QVERIFY_RESULT(optionsParsed);
    QVERIFY(optionsParsed->textDocumentSync().has_value());
    const auto sync = std::get_if<TextDocumentSyncOptions>(&*optionsParsed->textDocumentSync());
    QVERIFY(sync);
    QCOMPARE(sync->openClose().value_or(false), true);
    QVERIFY(sync->save().has_value());
    QCOMPARE(std::get<bool>(*sync->save()), true);
}

void tst_LanguageServerProtocol::positionRoundTrip()
{
    const Position position = Position().line(3).character(7);

    const QJsonObject json = toJson(position);
    QCOMPARE(json.value("line").toInt(), 3);
    QCOMPARE(json.value("character").toInt(), 7);

    const Utils::Result<Position> parsed = fromJson<Position>(json);
    QVERIFY_RESULT(parsed);
    QCOMPARE(*parsed, position);
}

void tst_LanguageServerProtocol::wireRoundTrip()
{
    const HoverParams params
        = HoverParams()
              .textDocument(TextDocumentIdentifier().uri("file:///tmp/test.cpp"))
              .position(Position().line(2).character(4));
    const BaseMessage sent = toBaseMessage(
        LanguageServerProtocol::requestObject<HoverRequest>(MessageId(7), params));

    QByteArray data = sent.header() + sent.content;
    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QString parseError;
    BaseMessage received;
    BaseMessage::parse(&buffer, parseError, received);
    QVERIFY(parseError.isEmpty());
    QVERIFY(received.isComplete());

    const Utils::Result<QJsonObject> message = fromBaseMessage(received);
    QVERIFY_RESULT(message);
    QCOMPARE(messageMethod(*message), QString(HoverRequest::method));
    QCOMPARE(messageId(*message), MessageId(7));

    const Utils::Result<HoverParams> parsed = LanguageServerProtocol::params<HoverRequest>(*message);
    QVERIFY_RESULT(parsed);
    QCOMPARE(*parsed, params);
}

void tst_LanguageServerProtocol::nonObjectInput_data()
{
    QTest::addColumn<QJsonValue>("input");

    QTest::newRow("null") << QJsonValue(QJsonValue::Null);
    QTest::newRow("undefined") << QJsonValue(QJsonValue::Undefined);
    QTest::newRow("bool") << QJsonValue(true);
    QTest::newRow("number") << QJsonValue(42);
    QTest::newRow("string") << QJsonValue("text");
    QTest::newRow("array") << QJsonValue(QJsonArray{1, 2});
}

void tst_LanguageServerProtocol::nonObjectInput()
{
    QFETCH(QJsonValue, input);

    const Utils::Result<Position> position = fromJson<Position>(input);
    QVERIFY(!position);
    QCOMPARE(position.error(), QString("Expected JSON object for Position"));

    QVERIFY(!fromJson<Range>(input));
    QVERIFY(!fromJson<TextEdit>(input));
    QVERIFY(!fromJson<Diagnostic>(input));
    QVERIFY(!fromJson<ServerCapabilities>(input));
}

void tst_LanguageServerProtocol::missingRequiredFields()
{
    const Utils::Result<Position> noCharacter = fromJson<Position>(QJsonObject{{"line", 1}});
    QVERIFY(!noCharacter);
    QCOMPARE(noCharacter.error(), QString("Missing required field: character"));

    QVERIFY(!fromJson<Position>(QJsonObject{}));
    QVERIFY(!fromJson<TextDocumentIdentifier>(QJsonObject{}));

    const QJsonObject range = toJson(
        Range().start(Position().line(0).character(0)).end(Position().line(0).character(1)));
    QVERIFY(!fromJson<TextEdit>(QJsonObject{{"range", range}}));
    QVERIFY(!fromJson<Diagnostic>(QJsonObject{{"range", range}}));

    const Utils::Result<DocumentSymbol> symbol = fromJson<DocumentSymbol>(
        QJsonObject{{"name", "foo"}, {"kind", SymbolKind::Function}, {"range", range}});
    QVERIFY(!symbol);
    QCOMPARE(symbol.error(), QString("Missing required field: selectionRange"));
}

void tst_LanguageServerProtocol::unknownFieldsAndValues()
{
    const Utils::Result<Position> position = fromJson<Position>(
        QJsonObject{{"line", 1}, {"character", 2}, {"$vendorExtension", "ignored"}});
    QVERIFY_RESULT(position);
    QCOMPARE(*position, Position().line(1).character(2));

    const QJsonObject range = toJson(
        Range().start(Position().line(0).character(0)).end(Position().line(0).character(1)));

    // enumerations stay open so that values of a newer protocol version pass through
    const Utils::Result<DocumentSymbol> symbol = fromJson<DocumentSymbol>(
        QJsonObject{{"name", "foo"}, {"kind", 4711}, {"range", range}, {"selectionRange", range}});
    QVERIFY_RESULT(symbol);
    QCOMPARE(symbol->kind(), 4711);

    const Utils::Result<Diagnostic> diagnostic = fromJson<Diagnostic>(
        QJsonObject{{"range", range},
                    {"message", "msg"},
                    {"data", QJsonObject{{"anything", QJsonArray{1, 2}}}}});
    QVERIFY_RESULT(diagnostic);
    QVERIFY(diagnostic->data().has_value());
    QCOMPARE(diagnostic->data()->toObject().value("anything").toArray().size(), 2);
}

void tst_LanguageServerProtocol::nestedParseErrorPropagates()
{
    const QJsonObject incompleteStart{{"start", QJsonObject{{"line", 1}}},
                                      {"end", toJson(Position().line(1).character(0))}};
    const Utils::Result<Range> range = fromJson<Range>(incompleteStart);
    QVERIFY(!range);
    QCOMPARE(range.error(), QString("start: Missing required field: character"));

    QVERIFY(!fromJson<DocumentSelector>(QJsonArray{42}));

    const QJsonObject edit{
        {"changes",
         QJsonObject{{"file:///tmp/test.cpp", QJsonArray{QJsonObject{{"newText", "x"}}}}}}};
    const Utils::Result<WorkspaceEdit> workspaceEdit = fromJson<WorkspaceEdit>(edit);
    QVERIFY(!workspaceEdit);
    QCOMPARE(workspaceEdit.error(), QString("Missing required field: range"));
}

void tst_LanguageServerProtocol::malformedFieldTolerated()
{
    const Utils::Result<Location> location = fromJson<Location>(
        QJsonObject{{"uri", "file:///tmp/test.cpp"}, {"range", "not a range"}});
    QEXPECT_FAIL("", "a required field of the wrong JSON type is silently left default", Continue);
    QVERIFY(!location);

    const Utils::Result<Position> position = fromJson<Position>(
        QJsonObject{{"line", "1"}, {"character", 2}});
    QEXPECT_FAIL("", "a required field of the wrong JSON type is silently left default", Continue);
    QVERIFY(!position);
}

void tst_LanguageServerProtocol::enumValues()
{
    const Utils::Result<MarkupKind> markdown = fromJson<MarkupKind>(QJsonValue("markdown"));
    QVERIFY_RESULT(markdown);
    QVERIFY(*markdown == MarkupKind::markdown);
    QCOMPARE(toString(*markdown), QString("markdown"));

    const Utils::Result<MarkupKind> unknown = fromJson<MarkupKind>(QJsonValue("html"));
    QVERIFY(!unknown);
    QCOMPARE(unknown.error(), QString("Invalid MarkupKind value: html"));

    QVERIFY(!fromJson<MarkupKind>(QJsonValue(42)));
    QVERIFY(!fromJson<MarkupKind>(QJsonValue(QJsonValue::Null)));
}

void tst_LanguageServerProtocol::constFieldMismatch()
{
    const Utils::Result<StringValue> snippet = fromJson<StringValue>(
        QJsonObject{{"kind", "snippet"}, {"value", "for ($1)"}});
    QVERIFY_RESULT(snippet);
    QCOMPARE(snippet->value(), QString("for ($1)"));
    QCOMPARE(toJson(*snippet).value("kind").toString(), QString("snippet"));

    const Utils::Result<StringValue> wrongKind = fromJson<StringValue>(
        QJsonObject{{"kind", "text"}, {"value", "for ($1)"}});
    QVERIFY(!wrongKind);
    QCOMPARE(wrongKind.error(), QString("Field 'kind' must be 'snippet', got: text"));

    QVERIFY(!fromJson<StringValue>(QJsonObject{{"value", "for ($1)"}}));
}

void tst_LanguageServerProtocol::discriminatedUnion()
{
    const Utils::Result<DocumentDiagnosticReport> full = fromJson<DocumentDiagnosticReport>(
        QJsonObject{{"kind", "full"}, {"items", QJsonArray{}}});
    QVERIFY_RESULT(full);
    QVERIFY(std::holds_alternative<RelatedFullDocumentDiagnosticReport>(*full));

    const Utils::Result<DocumentDiagnosticReport> unchanged = fromJson<DocumentDiagnosticReport>(
        QJsonObject{{"kind", "unchanged"}, {"resultId", "42"}});
    QVERIFY_RESULT(unchanged);
    QVERIFY(std::holds_alternative<RelatedUnchangedDocumentDiagnosticReport>(*unchanged));

    const Utils::Result<DocumentDiagnosticReport> unknownKind
        = fromJson<DocumentDiagnosticReport>(QJsonObject{{"kind", "partial"}});
    QVERIFY(!unknownKind);
    QCOMPARE(unknownKind.error(),
             QString("Invalid DocumentDiagnosticReport: unknown kind \"partial\""));

    QVERIFY(!fromJson<DocumentDiagnosticReport>(QJsonObject{}));
    QVERIFY(!fromJson<DocumentDiagnosticReport>(QJsonValue(QJsonArray{})));
    QVERIFY(!fromJson<DocumentDiagnosticReport>(QJsonObject{{"kind", "unchanged"}}));
}

void tst_LanguageServerProtocol::untaggedUnionMismatch()
{
    QVERIFY_RESULT(fromJson<ProgressToken>(QJsonValue(1)));
    QVERIFY_RESULT(fromJson<ProgressToken>(QJsonValue("token")));

    const Utils::Result<ProgressToken> boolToken = fromJson<ProgressToken>(QJsonValue(true));
    QVERIFY(!boolToken);
    QCOMPARE(boolToken.error(), QString("Invalid ProgressToken"));

    const Utils::Result<Definition> noRange = fromJson<Definition>(
        QJsonObject{{"uri", "file:///tmp/test.cpp"}});
    QVERIFY(!noRange);
    QCOMPARE(noRange.error(), QString("Invalid Definition"));

    QVERIFY(!fromJson<HoverRequestResult>(QJsonObject{{"value", "hover"}}));
}

void tst_LanguageServerProtocol::documentSelector()
{
    const Utils::Result<DocumentSelector> selector = fromJson<DocumentSelector>(
        QJsonArray{QJsonObject{{"language", "cpp"}}, QJsonObject{{"scheme", "file"}}});
    QVERIFY_RESULT(selector);
    QCOMPARE(selector->size(), 2);
    const auto textFilter = std::get_if<TextDocumentFilter>(&selector->first());
    QVERIFY(textFilter);
    const auto language = std::get_if<TextDocumentFilterLanguage>(textFilter);
    QVERIFY(language);
    QCOMPARE(language->language(), QString("cpp"));

    const Utils::Result<DocumentSelector> notAnArray = fromJson<DocumentSelector>(QJsonObject{});
    QVERIFY(!notAnArray);
    QCOMPARE(notAnArray.error(), QString("Expected JSON array for DocumentSelector"));

    QVERIFY(!fromJson<DocumentSelector>(QJsonArray{QJsonObject{{"unknown", "filter"}}}));
}

void tst_LanguageServerProtocol::workspaceEditChanges()
{
    const TextEdit textEdit
        = TextEdit()
              .range(
                  Range().start(Position().line(1).character(0)).end(Position().line(1).character(4)))
              .newText("void");
    const WorkspaceEdit edit = WorkspaceEdit().addChange("file:///tmp/test.cpp", {textEdit});

    const Utils::Result<WorkspaceEdit> parsed = fromJson<WorkspaceEdit>(toJson(edit));
    QVERIFY_RESULT(parsed);
    QCOMPARE(*parsed, edit);

    const Utils::Result<WorkspaceEdit> wrongMapType = fromJson<WorkspaceEdit>(
        QJsonObject{{"changes", QJsonArray{}}});
    QVERIFY_RESULT(wrongMapType);
    QVERIFY(!wrongMapType->changes().has_value());
}

void tst_LanguageServerProtocol::recursiveType()
{
    const QJsonObject range = toJson(
        Range().start(Position().line(1).character(0)).end(Position().line(1).character(10)));

    const Utils::Result<SelectionRange> parsed = fromJson<SelectionRange>(
        QJsonObject{{"range", range}, {"parent", QJsonObject{{"range", range}}}});
    QVERIFY_RESULT(parsed);
    QVERIFY(parsed->parent().has_value());
    QCOMPARE(parsed->parent()->range().start().line(), 1);
    QVERIFY(!parsed->parent()->parent().has_value());

    QVERIFY(!fromJson<SelectionRange>(
        QJsonObject{{"range", range}, {"parent", QJsonObject{{"range", QJsonObject{}}}}}));
}

void tst_LanguageServerProtocol::nullPayload()
{
    const Utils::Result<HoverRequestResult> nullHover = fromJson<HoverRequestResult>(
        QJsonValue(QJsonValue::Null));
    QVERIFY_RESULT(nullHover);
    QVERIFY(std::holds_alternative<std::monostate>(*nullHover));

    const QJsonObject response{{"jsonrpc", "2.0"},
                               {"id", 1},
                               {"result", QJsonValue(QJsonValue::Null)}};
    const Utils::Result<HoverRequestResult> parsed = LanguageServerProtocol::result<HoverRequest>(
        response);
    QVERIFY_RESULT(parsed);
    QVERIFY(std::holds_alternative<std::monostate>(*parsed));
}

void tst_LanguageServerProtocol::malformedRequestParams()
{
    const QJsonObject withoutParams{{"jsonrpc", "2.0"}, {"id", 1}, {"method", HoverRequest::method}};
    QVERIFY(!LanguageServerProtocol::params<HoverRequest>(withoutParams));

    const QJsonObject withoutPosition{
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", HoverRequest::method},
        {"params", QJsonObject{{"textDocument", QJsonObject{{"uri", "file:///tmp/test.cpp"}}}}}};
    const Utils::Result<HoverParams> parsed = LanguageServerProtocol::params<HoverRequest>(
        withoutPosition);
    QVERIFY(!parsed);
    QCOMPARE(parsed.error(), QString("Missing required field: position"));
}

void tst_LanguageServerProtocol::messageIdFromUnexpectedJson()
{
    QVERIFY(!MessageId(QJsonValue(QJsonValue::Null)).isValid());
    QVERIFY(!MessageId(QJsonValue(QJsonValue::Undefined)).isValid());
    QVERIFY(!MessageId(QJsonValue(true)).isValid());
    QVERIFY(!MessageId(QJsonValue(QJsonObject{})).isValid());
    QCOMPARE(MessageId(QJsonValue(42.0)), MessageId(42));
    QVERIFY(!messageId(QJsonObject{{"method", "exit"}}).isValid());
}

void tst_LanguageServerProtocol::baseMessageInvalidContentLength()
{
    QByteArray data("Content-Length: not-a-number\r\n\r\n{}");
    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QString parseError;
    BaseMessage message;
    BaseMessage::parse(&buffer, parseError, message);
    QVERIFY(!parseError.isEmpty());
    QCOMPARE(message.contentLength, 0);
    QVERIFY(message.content.isEmpty());
}

void tst_LanguageServerProtocol::baseMessageUnexpectedHeader()
{
    QByteArray data("X-Custom: value\r\n"
                    "Content-Length: 2\r\n"
                    "Content-Type: application/vscode-jsonrpc; charset=nonsense\r\n"
                    "\r\n"
                    "{}");
    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QString parseError;
    BaseMessage message;
    BaseMessage::parse(&buffer, parseError, message);
    QVERIFY(!parseError.isEmpty());
    QVERIFY(message.isComplete());
    QCOMPARE(message.encoding, BaseMessage::defaultEncoding());
    QCOMPARE(message.content, QByteArray("{}"));
}

void tst_LanguageServerProtocol::fromBaseMessageErrors()
{
    QVERIFY(!fromBaseMessage(BaseMessage("text/plain", "{}")));
    QVERIFY(!fromBaseMessage(BaseMessage(BaseMessage::jsonRpcMimeType, "not json")));
    QVERIFY(!fromBaseMessage(BaseMessage(BaseMessage::jsonRpcMimeType, "[1, 2]")));
    QVERIFY(!fromBaseMessage(BaseMessage(BaseMessage::jsonRpcMimeType, "")));
}

// The uri is the only value of the protocol that this library both parses and
// spells out, and the only one whose meaning is a local resource rather than a
// JSON shape, so it is worth its own rows.
void tst_LanguageServerProtocol::uriToPath_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<Utils::FilePath>("expected");

    QTest::newRow("empty") << QString() << Utils::FilePath();
    QTest::newRow("not a uri") << QString("/tmp/plain.txt") << Utils::FilePath();
    QTest::newRow("http") << QString("http://qt.io/index.html") << Utils::FilePath();
    QTest::newRow("no scheme") << QString("//host/share/file.txt") << Utils::FilePath();
    QTest::newRow("undecoded space")
        << QString("file:///tmp/with space.txt")
        << Utils::FilePath::fromUserInput("/tmp/with space.txt");
    QTest::newRow("percent encoded space")
        << QString("file:///tmp/with%20space.txt")
        << Utils::FilePath::fromUserInput("/tmp/with space.txt");
    QTest::newRow("percent encoded twice")
        << QString("file:///tmp/with%2520space.txt")
        << Utils::FilePath::fromUserInput("/tmp/with%20space.txt");
    QTest::newRow("non-ascii")
        << QString("file:///tmp/%C3%A4.txt") << Utils::FilePath::fromUserInput("/tmp/ä.txt");
#ifdef Q_OS_WIN
    QTest::newRow("drive letter")
        << QString("file:///C:/tmp/file.txt") << Utils::FilePath::fromUserInput("C:/tmp/file.txt");
    QTest::newRow("unc path")
        << QString("file://host/share/file.txt")
        << Utils::FilePath::fromUserInput("//host/share/file.txt");
#else
    QTest::newRow("absolute") << QString("file:///tmp/file.txt")
                              << Utils::FilePath::fromUserInput("/tmp/file.txt");
#endif
}

void tst_LanguageServerProtocol::uriToPath()
{
    QFETCH(QString, uri);
    QFETCH(Utils::FilePath, expected);

    QCOMPARE(pathFromUri(uri), expected);
}

void tst_LanguageServerProtocol::uriRoundTrip_data()
{
    QTest::addColumn<Utils::FilePath>("path");

    QTest::newRow("home") << Utils::FilePath::fromUserInput(QDir::homePath());
    QTest::newRow("space") << Utils::FilePath::fromUserInput(QDir::homePath() + "/with space.txt");
    QTest::newRow("hash") << Utils::FilePath::fromUserInput(QDir::homePath() + "/with#hash.txt");
    QTest::newRow("non-ascii") << Utils::FilePath::fromUserInput(QDir::homePath() + "/ä.txt");
}

void tst_LanguageServerProtocol::uriRoundTrip()
{
    QFETCH(Utils::FilePath, path);

    const QString uri = uriFromPath(path);
    QVERIFY(uri.startsWith("file:"));
    QCOMPARE(pathFromUri(uri), path);
}

void tst_LanguageServerProtocol::cursorPositions()
{
    QTextDocument document("first line\nsecond line\nthird line");

    QTextCursor cursor(&document);
    cursor.setPosition(document.findBlockByNumber(1).position() + 3);

    const Position position = positionOf(cursor);
    QCOMPARE(position.line(), 1);
    QCOMPARE(position.character(), 3);

    QCOMPARE(positionInDocument(position, &document), cursor.position());
    QCOMPARE(toTextCursor(position, &document).position(), cursor.position());

    // A line the document does not have has no position in it.
    QCOMPARE(positionInDocument(Position().line(99).character(0), &document), -1);
}

void tst_LanguageServerProtocol::positionWithOffset()
{
    QTextDocument document("first line\nsecond line\nthird line");
    const Position start = Position().line(1).character(2);

    QCOMPARE(withOffset(start, 3, &document), Position().line(1).character(5));
    // An offset that leaves the line moves to the next one.
    QCOMPARE(withOffset(start, 12, &document), Position().line(2).character(2));
    QCOMPARE(withOffset(start, -12, &document), Position().line(0).character(1));
}

void tst_LanguageServerProtocol::selectionRange()
{
    QTextDocument document("first line\nsecond line\nthird line");

    QTextCursor cursor(&document);
    cursor.setPosition(document.findBlockByNumber(0).position() + 6);
    cursor.setPosition(document.findBlockByNumber(1).position() + 6, QTextCursor::KeepAnchor);

    const Range range = rangeOf(cursor);
    QCOMPARE(range.start(), Position().line(0).character(6));
    QCOMPARE(range.end(), Position().line(1).character(6));
    QVERIFY(!isEmpty(range));
    QVERIFY(contains(range, Position().line(1).character(0)));

    const QTextCursor selection = toSelection(range, &document);
    QCOMPARE(selection.selectionStart(), cursor.selectionStart());
    QCOMPARE(selection.selectionEnd(), cursor.selectionEnd());
    QCOMPARE(selection.selectedText(), cursor.selectedText());
}

QTEST_GUILESS_MAIN(tst_LanguageServerProtocol)

#include "tst_languageserverprotocol.moc"
