// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

/**
 * MarkedString can be used to render human readable text. It is either a markdown string
 * or a code-block that provides a language and a code snippet. The language identifier
 * is semantically equal to the optional language identifier in fenced code blocks in GitHub
 * issues. See https://help.github.com/articles/creating-and-highlighting-code-blocks/#syntax-highlighting
 *
 * The pair of a language and a value is an equivalent to markdown:
 * ```${language}
 * ${value}
 * ```
 *
 * Note that markdown strings will be sanitized - that means html will be escaped.
* @deprecated use MarkupContent instead.
*/
class LANGUAGESERVERPROTOCOL_EXPORT MarkedLanguageString : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString language() const { return typedValue<QString>(languageKey); }
    void setLanguage(const QString &language) { insert(languageKey, language); }

    QString value() const { return typedValue<QString>(valueKey); }
    void setValue(const QString &value) { insert(valueKey, value); }

    bool isValid() const override { return contains(languageKey) && contains(valueKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT MarkedString
    : public std::variant<QString, MarkedLanguageString>
{
public:
    MarkedString() = default;
    explicit MarkedString(const MarkedLanguageString &other)
        : variant(other)
    {}
    explicit MarkedString(const QString &other)
        : variant(other)
    {}
    explicit MarkedString(const QJsonValue &value);

    bool isValid() const;
    operator QJsonValue() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT HoverContent
    : public std::variant<MarkedString, QList<MarkedString>, MarkupContent>
{
public:
    HoverContent() = default;
    explicit HoverContent(const MarkedString &other) : variant(other) {}
    explicit HoverContent(const QList<MarkedString> &other) : variant(other) {}
    explicit HoverContent(const MarkupContent &other) : variant(other) {}
    explicit HoverContent(const QJsonValue &value);
    bool isValid() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT Hover : public JsonObject
{
public:
    using JsonObject::JsonObject;

    HoverContent content() const;
    void setContent(const HoverContent &content);

    std::optional<Range> range() const { return optionalValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }
    void clearRange() { remove(rangeKey); }

    bool isValid() const override { return contains(contentsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT HoverResult : public std::variant<Hover, std::nullptr_t>
{
public:
    HoverResult() : variant(nullptr) {}
    explicit HoverResult(const Hover &hover) : variant(hover) {}
    explicit HoverResult(const QJsonValue &value);
    bool isValid() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT HoverRequest
    : public Request<HoverResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit HoverRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/hover";
};

/**
 * Represents a parameter of a callable-signature. A parameter can
 * have a label and a doc-comment.
 */
class LANGUAGESERVERPROTOCOL_EXPORT ParameterInformation : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString label() const { return typedValue<QString>(labelKey); }
    void setLabel(const QString &label) { insert(labelKey, label); }

    std::optional<MarkupOrString> documentation() const;
    void setDocumentation(const MarkupOrString &documentation)
    { insert(documentationKey, documentation.toJson()); }
    void clearDocumentation() { remove(documentationKey); }

    bool isValid() const override { return contains(labelKey); }
};

/**
 * Represents the signature of something callable. A signature
 * can have a label, like a function-name, a doc-comment, and
 * a set of parameters.
 */
class LANGUAGESERVERPROTOCOL_EXPORT SignatureInformation : public ParameterInformation
{
public:
    using ParameterInformation::ParameterInformation;

    std::optional<QList<ParameterInformation>> parameters() const
    { return optionalArray<ParameterInformation>(parametersKey); }
    void setParameters(const QList<ParameterInformation> &parameters)
    { insertArray(parametersKey, parameters); }
    void clearParameters() { remove(parametersKey); }

    std::optional<int> activeParameter() const { return optionalValue<int>(activeParameterKey); }
    void setActiveParameter(int activeParameter) { insert(activeParameterKey, activeParameter); }
    void clearActiveParameter() { remove(activeParameterKey); }
};

/**
 * Signature help represents the signature of something
 * callable. There can be multiple signature but only one
 * active and only one active parameter.
 */
class LANGUAGESERVERPROTOCOL_EXPORT SignatureHelp : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /// One or more signatures.
    QList<SignatureInformation> signatures() const
    { return array<SignatureInformation>(signaturesKey); }
    void setSignatures(const QList<SignatureInformation> &signatures)
    { insertArray(signaturesKey, signatures); }

    /**
     * The active signature. If omitted or the value lies outside the
     * range of `signatures` the value defaults to zero or is ignored if
     * `signatures.length === 0`. Whenever possible implementors should
     * make an active decision about the active signature and shouldn't
     * rely on a default value.
     * In future version of the protocol this property might become
     * mandatory to better express this.
     */
    std::optional<int> activeSignature() const { return optionalValue<int>(activeSignatureKey); }
    void setActiveSignature(int activeSignature) { insert(activeSignatureKey, activeSignature); }
    void clearActiveSignature() { remove(activeSignatureKey); }

    /**
     * The active parameter of the active signature. If omitted or the value
     * lies outside the range of `signatures[activeSignature].parameters`
     * defaults to 0 if the active signature has parameters. If
     * the active signature has no parameters it is ignored.
     * In future version of the protocol this property might become
     * mandatory to better express the active parameter if the
     * active signature does have any.
     */
    std::optional<int> activeParameter() const { return optionalValue<int>(activeParameterKey); }
    void setActiveParameter(int activeParameter) { insert(activeParameterKey, activeParameter); }
    void clearActiveParameter() { remove(activeParameterKey); }

    bool isValid() const override { return contains(signaturesKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SignatureHelpRequest
    : public Request<LanguageClientValue<SignatureHelp>, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit SignatureHelpRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/signatureHelp";
};

/// The result of a goto request can either be a location, a list of locations or null
class LANGUAGESERVERPROTOCOL_EXPORT GotoResult
        : public std::variant<Location, QList<Location>, std::nullptr_t>
{
public:
    explicit GotoResult(const QJsonValue &value);
    using variant::variant;
};

class LANGUAGESERVERPROTOCOL_EXPORT GotoDefinitionRequest
    : public Request<GotoResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit GotoDefinitionRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/definition";
};

class LANGUAGESERVERPROTOCOL_EXPORT GotoTypeDefinitionRequest : public Request<
        GotoResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit GotoTypeDefinitionRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/typeDefinition";
};

class LANGUAGESERVERPROTOCOL_EXPORT GotoImplementationRequest : public Request<
        GotoResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit GotoImplementationRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/implementation";
};

class LANGUAGESERVERPROTOCOL_EXPORT ReferenceParams : public TextDocumentPositionParams
{
public:
    using TextDocumentPositionParams::TextDocumentPositionParams;

    class ReferenceContext : public JsonObject
    {
    public:
        explicit ReferenceContext(bool includeDeclaration)
        { setIncludeDeclaration(includeDeclaration); }
        ReferenceContext() = default;
        using JsonObject::JsonObject;
        bool includeDeclaration() const { return typedValue<bool>(includeDeclarationKey); }
        void setIncludeDeclaration(bool includeDeclaration)
        { insert(includeDeclarationKey, includeDeclaration); }

        bool isValid() const override { return contains(includeDeclarationKey); }
    };

    ReferenceContext context() const { return typedValue<ReferenceContext>(contextKey); }
    void setContext(const ReferenceContext &context) { insert(contextKey, context); }

    bool isValid() const override
    { return TextDocumentPositionParams::isValid() && contains(contextKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT FindReferencesRequest : public Request<
        LanguageClientArray<Location>, std::nullptr_t, ReferenceParams>
{
public:
    explicit FindReferencesRequest(const ReferenceParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/references";
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentHighlight : public JsonObject
{
public:
    using JsonObject::JsonObject;

    enum DocumentHighlightKind {
        Text = 1,
        Read = 2,
        Write = 3
    };

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    std::optional<int> kind() const { return optionalValue<int>(kindKey); }
    void setKind(int kind) { insert(kindKey, kind); }
    void clearKind() { remove(kindKey); }

    bool isValid() const override { return contains(rangeKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentHighlightsResult
        : public std::variant<QList<DocumentHighlight>, std::nullptr_t>
{
public:
    using variant::variant;
    DocumentHighlightsResult() : variant(nullptr) {}
    explicit DocumentHighlightsResult(const QJsonValue &value);
    using variant::operator=;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentHighlightsRequest : public Request<
        DocumentHighlightsResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit DocumentHighlightsRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/documentHighlight";
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentParams : public JsonObject
{
public:
    TextDocumentParams();
    explicit TextDocumentParams(const TextDocumentIdentifier &identifier);
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    bool isValid() const override { return contains(textDocumentKey); }
};

using DocumentSymbolParams = TextDocumentParams;

class LANGUAGESERVERPROTOCOL_EXPORT DocumentSymbolsResult
        : public std::variant<QList<SymbolInformation>, QList<DocumentSymbol>, std::nullptr_t>
{
public:
    using variant::variant;
    DocumentSymbolsResult() : variant(nullptr) {}
    explicit DocumentSymbolsResult(const QJsonValue &value);
    DocumentSymbolsResult(const DocumentSymbolsResult &other) : variant(other) {}
    DocumentSymbolsResult(DocumentSymbolsResult &&other) : variant(std::move(other)) {}

    using variant::operator=;
    DocumentSymbolsResult &operator =(DocumentSymbolsResult &&other)
    {
        variant::operator=(std::move(other));
        return *this;
    }
    DocumentSymbolsResult &operator =(const DocumentSymbolsResult &other)
    {
        variant::operator=(other);
        return *this;
    }
    // Needed to make it usable in Qt 6 signals.
    bool operator<(const DocumentSymbolsResult &other) const = delete;
};


class LANGUAGESERVERPROTOCOL_EXPORT DocumentSymbolsRequest
        : public Request<DocumentSymbolsResult, std::nullptr_t, DocumentSymbolParams>
{
public:
    explicit DocumentSymbolsRequest(const DocumentSymbolParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/documentSymbol";
};

/**
 * The kind of a code action.
 *
 * Kinds are a hierarchical list of identifiers separated by `.`, e.g. `"refactor.extract.function"`.
 *
 * The set of kinds is open and client needs to announce the kinds it supports to the server during
 * initialization.
 */

using CodeActionKind = QString;

/**
 * A set of predefined code action kinds
 */

namespace CodeActionKinds {
constexpr char QuickFix[] = "quickfix";
constexpr char Refactor[] = "refactor";
constexpr char RefactorExtract[] = "refactor.extract";
constexpr char RefactorInline[] = "refactor.inline";
constexpr char RefactorRewrite[] = "refactor.rewrite";
constexpr char Source[] = "source";
constexpr char SourceOrganizeImports[] = "source.organizeImports";
}

class LANGUAGESERVERPROTOCOL_EXPORT CodeActionParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    class LANGUAGESERVERPROTOCOL_EXPORT CodeActionContext : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        QList<Diagnostic> diagnostics() const { return array<Diagnostic>(diagnosticsKey); }
        void setDiagnostics(const QList<Diagnostic> &diagnostics)
        { insertArray(diagnosticsKey, diagnostics); }

        std::optional<QList<CodeActionKind>> only() const;
        void setOnly(const QList<CodeActionKind> &only);
        void clearOnly() { remove(onlyKey); }

        bool isValid() const override { return contains(diagnosticsKey); }
    };

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    CodeActionContext context() const { return typedValue<CodeActionContext>(contextKey); }
    void setContext(const CodeActionContext &context) { insert(contextKey, context); }

    bool isValid() const override
    { return contains(textDocumentKey) && contains(rangeKey) && contains(contextKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeAction : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString title() const { return typedValue<QString>(titleKey); }
    void setTitle(QString title) { insert(titleKey, title); }

    std::optional<CodeActionKind> kind() const { return optionalValue<CodeActionKind>(kindKey); }
    void setKind(const CodeActionKind &kind) { insert(kindKey, kind); }
    void clearKind() { remove(kindKey); }

    std::optional<QList<Diagnostic>> diagnostics() const
    { return optionalArray<Diagnostic>(diagnosticsKey); }
    void setDiagnostics(const QList<Diagnostic> &diagnostics)
    { insertArray(diagnosticsKey, diagnostics); }
    void clearDiagnostics() { remove(diagnosticsKey); }

    std::optional<WorkspaceEdit> edit() const { return optionalValue<WorkspaceEdit>(editKey); }
    void setEdit(const WorkspaceEdit &edit) { insert(editKey, edit); }
    void clearEdit() { remove(editKey); }

    std::optional<Command> command() const { return optionalValue<Command>(commandKey); }
    void setCommand(const Command &command) { insert(commandKey, command); }
    void clearCommand() { remove(commandKey); }

    bool isValid() const override { return contains(titleKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeActionResult
    : public std::variant<QList<std::variant<Command, CodeAction>>, std::nullptr_t>
{
public:
    using variant::variant;
    explicit CodeActionResult(const QJsonValue &val);
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeActionRequest : public Request<
        CodeActionResult, std::nullptr_t, CodeActionParams>
{
public:
    explicit CodeActionRequest(const CodeActionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/codeAction";
};

using CodeLensParams = TextDocumentParams;

class LANGUAGESERVERPROTOCOL_EXPORT CodeLens : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    std::optional<Command> command() const { return optionalValue<Command>(commandKey); }
    void setCommand(const Command &command) { insert(commandKey, command); }
    void clearCommand() { remove(commandKey); }

    std::optional<QJsonValue> data() const;
    void setData(const QJsonValue &data) { insert(dataKey, data); }
    void clearData() { remove(dataKey); }

    bool isValid() const override { return contains(rangeKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeLensRequest : public Request<
        LanguageClientArray<CodeLens>, std::nullptr_t, CodeLensParams>
{
public:
    explicit CodeLensRequest(const CodeLensParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/codeLens";
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeLensResolveRequest : public Request<
        CodeLens, std::nullptr_t, CodeLens>
{
public:
    explicit CodeLensResolveRequest(const CodeLens &params);
    using Request::Request;
    constexpr static const char methodName[] = "codeLens/resolve";
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentLink : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    std::optional<DocumentUri> target() const;
    void setTarget(const DocumentUri &target) { insert(targetKey, target.toString()); }
    void clearTarget() { remove(targetKey); }

    std::optional<QJsonValue> data() const;
    void setData(const QJsonValue &data) { insert(dataKey, data); }
    void clearData() { remove(dataKey); }

    bool isValid() const override { return contains(rangeKey); }
};

using DocumentLinkParams = TextDocumentParams;

class LANGUAGESERVERPROTOCOL_EXPORT DocumentLinkRequest : public Request<
        LanguageClientValue<DocumentLink>, std::nullptr_t, DocumentLinkParams>
{
public:
    explicit DocumentLinkRequest(const DocumentLinkParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/documentLink";
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentLinkResolveRequest : public Request<
        DocumentLink, std::nullptr_t, DocumentLink>
{
public:
    explicit DocumentLinkResolveRequest(const DocumentLink &params);
    using Request::Request;
    constexpr static const char methodName[] = "documentLink/resolve";
};

using DocumentColorParams = TextDocumentParams;

class LANGUAGESERVERPROTOCOL_EXPORT Color : public JsonObject
{
public:
    using JsonObject::JsonObject;

    double red() const { return typedValue<double>(redKey); }
    void setRed(double red) { insert(redKey, red); }

    double green() const { return typedValue<double>(greenKey); }
    void setGreen(double green) { insert(greenKey, green); }

    double blue() const { return typedValue<double>(blueKey); }
    void setBlue(double blue) { insert(blueKey, blue); }

    double alpha() const { return typedValue<double>(alphaKey); }
    void setAlpha(double alpha) { insert(alphaKey, alpha); }

    bool isValid() const override
    { return contains(redKey) && contains(greenKey) && contains(blueKey) && contains(alphaKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ColorInformation : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(Range range) { insert(rangeKey, range); }

    Color color() const { return typedValue<Color>(colorKey); }
    void setColor(const Color &color) { insert(colorKey, color); }

    bool isValid() const override { return contains(rangeKey) && contains(colorKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentColorRequest : public Request<
        QList<ColorInformation>, std::nullptr_t, DocumentColorParams>
{
public:
    explicit DocumentColorRequest(const DocumentColorParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/documentColor";
};

class LANGUAGESERVERPROTOCOL_EXPORT ColorPresentationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    Color colorInfo() const { return typedValue<Color>(colorInfoKey); }
    void setColorInfo(const Color &colorInfo) { insert(colorInfoKey, colorInfo); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    bool isValid() const override
    { return contains(textDocumentKey) && contains(colorInfoKey) && contains(rangeKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ColorPresentation : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString label() const { return typedValue<QString>(labelKey); }
    void setLabel(const QString &label) { insert(labelKey, label); }

    std::optional<TextEdit> textEdit() const { return optionalValue<TextEdit>(textEditKey); }
    void setTextEdit(const TextEdit &textEdit) { insert(textEditKey, textEdit); }
    void clearTextEdit() { remove(textEditKey); }

    std::optional<QList<TextEdit>> additionalTextEdits() const
    { return optionalArray<TextEdit>(additionalTextEditsKey); }
    void setAdditionalTextEdits(const QList<TextEdit> &additionalTextEdits)
    { insertArray(additionalTextEditsKey, additionalTextEdits); }
    void clearAdditionalTextEdits() { remove(additionalTextEditsKey); }

    bool isValid() const override { return contains(labelKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ColorPresentationRequest : public Request<
        QList<ColorPresentation>, std::nullptr_t, ColorPresentationParams>
{
public:
    explicit ColorPresentationRequest(const ColorPresentationParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/colorPresentation";
};

class DocumentFormattingProperty : public std::variant<bool, double, QString>
{
public:
    DocumentFormattingProperty() = default;
    explicit DocumentFormattingProperty(const QJsonValue &value);
    explicit DocumentFormattingProperty(const DocumentFormattingProperty &other)
        : std::variant<bool, double, QString>(other) {}

    using variant::variant;
    using variant::operator=;
};

class LANGUAGESERVERPROTOCOL_EXPORT FormattingOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    int tabSize() const { return typedValue<int>(tabSizeKey); }
    void setTabSize(int tabSize) { insert(tabSizeKey, tabSize); }

    bool insertSpace() const { return typedValue<bool>(insertSpaceKey); }
    void setInsertSpace(bool insertSpace) { insert(insertSpaceKey, insertSpace); }

    std::optional<bool> trimTrailingWhitespace() const
    { return optionalValue<bool>(trimTrailingWhitespaceKey); }
    void setTrimTrailingWhitespace(bool trimTrailingWhitespace)
    { insert(trimTrailingWhitespaceKey, trimTrailingWhitespace); }
    void clearTrimTrailingWhitespace() { remove(trimTrailingWhitespaceKey); }

    std::optional<bool> insertFinalNewline() const
    { return optionalValue<bool>(insertFinalNewlineKey); }
    void setInsertFinalNewline(bool insertFinalNewline)
    { insert(insertFinalNewlineKey, insertFinalNewline); }
    void clearInsertFinalNewline() { remove(insertFinalNewlineKey); }

    std::optional<bool> trimFinalNewlines() const
    { return optionalValue<bool>(trimFinalNewlinesKey); }
    void setTrimFinalNewlines(bool trimFinalNewlines)
    { insert(trimFinalNewlinesKey, trimFinalNewlines); }
    void clearTrimFinalNewlines() { remove(trimFinalNewlinesKey); }

    QHash<QString, DocumentFormattingProperty> properties() const;
    void setProperty(const std::string_view key, const DocumentFormattingProperty &property);
    void removeProperty(const std::string_view &key) { remove(key); }

    bool isValid() const override { return contains(insertSpaceKey) && contains(tabSizeKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentFormattingParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    FormattingOptions options() const { return typedValue<FormattingOptions>(optionsKey); }
    void setOptions(const FormattingOptions &options) { insert(optionsKey, options); }

    bool isValid() const override { return contains(textDocumentKey) && contains(optionsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentFormattingRequest : public Request<
        LanguageClientArray<TextEdit>, std::nullptr_t, DocumentFormattingParams>
{
public:
    explicit DocumentFormattingRequest(const DocumentFormattingParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/formatting";
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentRangeFormattingParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    FormattingOptions options() const { return typedValue<FormattingOptions>(optionsKey); }
    void setOptions(const FormattingOptions &options) { insert(optionsKey, options); }

    bool isValid() const override
    { return contains(textDocumentKey) && contains(rangeKey) && contains(optionsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentRangeFormattingRequest : public Request<
        LanguageClientArray<TextEdit>, std::nullptr_t, DocumentRangeFormattingParams>
{
public:
    explicit DocumentRangeFormattingRequest(const DocumentRangeFormattingParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/rangeFormatting";
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentOnTypeFormattingParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    Position position() const { return typedValue<Position>(positionKey); }
    void setPosition(const Position &position) { insert(positionKey, position); }

    QString ch() const { return typedValue<QString>(chKey); }
    void setCh(const QString &ch) { insert(chKey, ch); }

    FormattingOptions options() const { return typedValue<FormattingOptions>(optionsKey); }
    void setOptions(const FormattingOptions &options) { insert(optionsKey, options); }

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentOnTypeFormattingRequest : public Request<
        LanguageClientArray<TextEdit>, std::nullptr_t, DocumentOnTypeFormattingParams>
{
public:
    explicit DocumentOnTypeFormattingRequest(const DocumentOnTypeFormattingParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/onTypeFormatting";
};

class PlaceHolderResult : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    QString placeHolder() const { return typedValue<QString>(placeHolderKey); }
    void setPlaceHolder(const QString &placeHolder) { insert(placeHolderKey, placeHolder); }

    bool isValid() const override { return contains(rangeKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT PrepareRenameResult
    : public std::variant<PlaceHolderResult, Range, std::nullptr_t>
{
public:
    PrepareRenameResult();
    PrepareRenameResult(const std::variant<PlaceHolderResult, Range, std::nullptr_t> &val);
    explicit PrepareRenameResult(const PlaceHolderResult &val);
    explicit PrepareRenameResult(const Range &val);
    explicit PrepareRenameResult(const QJsonValue &val);

    bool isValid() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT PrepareRenameRequest
    : public Request<PrepareRenameResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit PrepareRenameRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/prepareRename";
};

class LANGUAGESERVERPROTOCOL_EXPORT RenameParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    Position position() const { return typedValue<Position>(positionKey); }
    void setPosition(const Position &position) { insert(positionKey, position); }

    QString newName() const { return typedValue<QString>(newNameKey); }
    void setNewName(const QString &newName) { insert(newNameKey, newName); }

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT RenameRequest : public Request<
        WorkspaceEdit, std::nullptr_t, RenameParams>
{
public:
    explicit RenameRequest(const RenameParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/rename";
};

} // namespace LanguageClient
