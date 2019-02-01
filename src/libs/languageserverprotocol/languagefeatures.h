/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    bool isValid(QStringList *error) const override
    { return check<QString>(error, languageKey) && check<QString>(error, valueKey); }
};

class MarkedString : public Utils::variant<MarkedLanguageString, QList<MarkedLanguageString>, MarkupContent>
{
public:
    MarkedString() = default;
    explicit MarkedString(const MarkedLanguageString &other) : variant(other) {}
    explicit MarkedString(const QList<MarkedLanguageString> &other) : variant(other) {}
    explicit MarkedString(const MarkupContent &other) : variant(other) {}
    explicit MarkedString(const QJsonValue &value);
    bool isValid(QStringList *errorHierarchy) const;
};

class LANGUAGESERVERPROTOCOL_EXPORT Hover : public JsonObject
{
public:
    using JsonObject::JsonObject;

    MarkedString content() const;
    void setContent(const MarkedString &content);

    Utils::optional<Range> range() const { return optionalValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }
    void clearRange() { remove(rangeKey); }

    bool isValid(QStringList *error) const override
    { return check<MarkedString>(error, contentKey) && checkOptional<Range>(error, rangeKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT HoverRequest
    : public Request<Hover, std::nullptr_t, TextDocumentPositionParams>
{
public:
    HoverRequest(const TextDocumentPositionParams &params = TextDocumentPositionParams());
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

    Utils::optional<MarkupOrString> documentation() const;
    void setDocumentation(const MarkupOrString &documentation)
    { insert(documentationKey, documentation.toJson()); }
    void clearDocumentation() { remove(documentationKey); }

    bool isValid(QStringList *error) const override
    {
        return check<QString>(error, labelKey)
                && checkOptional<MarkupOrString>(error, documentationKey);
    }
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

    Utils::optional<QList<ParameterInformation>> parameters() const
    { return optionalArray<ParameterInformation>(parametersKey); }
    void setParameters(const QList<ParameterInformation> &parameters)
    { insertArray(parametersKey, parameters); }
    void clearParameters() { remove(parametersKey); }
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
    Utils::optional<int> activeSignature() const { return optionalValue<int>(activeSignatureKey); }
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
    Utils::optional<int> activeParameter() const { return optionalValue<int>(activeParameterKey); }
    void setActiveParameter(int activeParameter) { insert(activeParameterKey, activeParameter); }
    void clearActiveParameter() { remove(activeParameterKey); }

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT SignatureHelpRequest
    : public Request<LanguageClientValue<SignatureHelp>, std::nullptr_t, TextDocumentPositionParams>
{
public:
    SignatureHelpRequest(const TextDocumentPositionParams &params = TextDocumentPositionParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/signatureHelp";
};

/// The result of a goto request can either be a location, a list of locations or null
class LANGUAGESERVERPROTOCOL_EXPORT GotoResult
        : public Utils::variant<Location, QList<Location>, std::nullptr_t>
{
public:
    GotoResult(const QJsonValue &value);
    using variant::variant;
};

class LANGUAGESERVERPROTOCOL_EXPORT GotoDefinitionRequest
    : public Request<GotoResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    GotoDefinitionRequest(const TextDocumentPositionParams &params = TextDocumentPositionParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/definition";
};

class LANGUAGESERVERPROTOCOL_EXPORT GotoTypeDefinitionRequest : public Request<
        GotoResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    GotoTypeDefinitionRequest(
            const TextDocumentPositionParams &params = TextDocumentPositionParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/typeDefinition";
};

class LANGUAGESERVERPROTOCOL_EXPORT GotoImplementationRequest : public Request<
        GotoResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    GotoImplementationRequest(
            const TextDocumentPositionParams &params = TextDocumentPositionParams());
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

        bool isValid(QStringList *error) const override
        { return check<bool>(error, includeDeclarationKey); }
    };

    ReferenceContext context() const { return typedValue<ReferenceContext>(contextKey); }
    void setContext(const ReferenceContext &context) { insert(contextKey, context); }

    bool isValid(QStringList *error) const override
    {
        return TextDocumentPositionParams::isValid(error)
                && check<ReferenceContext>(error, contextKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT FindReferencesRequest : public Request<
        LanguageClientArray<Location>, std::nullptr_t, ReferenceParams>
{
public:
    FindReferencesRequest(const ReferenceParams &params = ReferenceParams());
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

    Utils::optional<int> kind() const { return optionalValue<int>(kindKey); }
    void setKind(int kind) { insert(kindKey, kind); }
    void clearKind() { remove(kindKey); }

    bool isValid(QStringList *error) const override
    { return check<Range>(error, rangeKey) && checkOptional<int>(error, kindKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentHighlightsResult
        : public Utils::variant<QList<DocumentHighlight>, std::nullptr_t>
{
public:
    using variant::variant;
    DocumentHighlightsResult() : variant(nullptr) {}
    DocumentHighlightsResult(const QJsonValue &value);
    using variant::operator=;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentHighlightsRequest : public Request<
        DocumentHighlightsResult, std::nullptr_t, TextDocumentPositionParams>
{
public:
    DocumentHighlightsRequest(
            const TextDocumentPositionParams &params = TextDocumentPositionParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/documentHighlight";
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentParams : public JsonObject
{
public:
    TextDocumentParams();
    TextDocumentParams(const TextDocumentIdentifier &identifier);
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    bool isValid(QStringList *error) const override
    { return check<TextDocumentIdentifier>(error, textDocumentKey); }
};

using DocumentSymbolParams = TextDocumentParams;

class LANGUAGESERVERPROTOCOL_EXPORT DocumentSymbolsResult
        : public Utils::variant<QList<SymbolInformation>, QList<DocumentSymbol>, std::nullptr_t>
{
public:
    using variant::variant;
    DocumentSymbolsResult() : variant(nullptr) {}
    DocumentSymbolsResult(const QJsonValue &value);
    using variant::operator=;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentSymbolsRequest
        : public Request<DocumentSymbolsResult, std::nullptr_t, DocumentSymbolParams>
{
public:
    DocumentSymbolsRequest(const DocumentSymbolParams &params = DocumentSymbolParams());
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

        Utils::optional<QList<CodeActionKind>> only() const;
        void setOnly(const QList<CodeActionKind> &only);
        void clearOnly() { remove(onlyKey); }

        bool isValid(QStringList *error) const override;
    };

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    CodeActionContext context() const { return typedValue<CodeActionContext>(contextKey); }
    void setContext(const CodeActionContext &context) { insert(contextKey, context); }

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeAction : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString title() const { return typedValue<QString>(titleKey); }
    void setTitle(QString title) { insert(titleKey, title); }

    Utils::optional<CodeActionKind> kind() const { return optionalValue<CodeActionKind>(kindKey); }
    void setKind(const CodeActionKind &kind) { insert(kindKey, kind); }
    void clearKind() { remove(kindKey); }

    Utils::optional<QList<Diagnostic>> diagnostics() const
    { return optionalArray<Diagnostic>(diagnosticsKey); }
    void setDiagnostics(const QList<Diagnostic> &diagnostics)
    { insertArray(diagnosticsKey, diagnostics); }
    void clearDiagnostics() { remove(diagnosticsKey); }

    Utils::optional<WorkspaceEdit> edit() const { return optionalValue<WorkspaceEdit>(editKey); }
    void setEdit(const WorkspaceEdit &edit) { insert(editKey, edit); }
    void clearEdit() { remove(editKey); }

    Utils::optional<Command> command() const { return optionalValue<Command>(commandKey); }
    void setCommand(const Command &command) { insert(commandKey, command); }
    void clearCommand() { remove(commandKey); }

    bool isValid(QStringList *) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeActionResult
    : public Utils::variant<QList<Utils::variant<Command, CodeAction>>, std::nullptr_t>
{
public:
    using variant::variant;
    explicit CodeActionResult(const QJsonValue &val);
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeActionRequest : public Request<
        CodeActionResult, std::nullptr_t, CodeActionParams>
{
public:
    CodeActionRequest(const CodeActionParams &params = CodeActionParams());
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

    Utils::optional<Command> command() const { return optionalValue<Command>(commandKey); }
    void setCommand(const Command &command) { insert(commandKey, command); }
    void clearCommand() { remove(commandKey); }

    Utils::optional<QJsonValue> data() const { return optionalValue<QJsonValue>(dataKey); }
    void setData(const QJsonValue &data) { insert(dataKey, data); }
    void clearData() { remove(dataKey); }

    bool isValid(QStringList *error) const override
    { return check<Range>(error, rangeKey) && checkOptional<Command>(error, commandKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeLensRequest : public Request<
        LanguageClientArray<CodeLens>, std::nullptr_t, CodeLensParams>
{
public:
    CodeLensRequest(const CodeLensParams &params = CodeLensParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/codeLens";
};

class LANGUAGESERVERPROTOCOL_EXPORT CodeLensResolveRequest : public Request<
        CodeLens, std::nullptr_t, CodeLens>
{
public:
    CodeLensResolveRequest(const CodeLens &params = CodeLens());
    using Request::Request;
    constexpr static const char methodName[] = "codeLens/resolve";
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentLink : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    Utils::optional<DocumentUri> target() const;
    void setTarget(const DocumentUri &target) { insert(targetKey, target.toString()); }
    void clearTarget() { remove(targetKey); }

    Utils::optional<QJsonValue> data() const { return optionalValue<QJsonValue>(dataKey); }
    void setData(const QJsonValue &data) { insert(dataKey, data); }
    void clearData() { remove(dataKey); }

    bool isValid(QStringList *error) const override
    { return check<Range>(error, rangeKey) && checkOptional<QString>(error, targetKey); }
};

using DocumentLinkParams = TextDocumentParams;

class LANGUAGESERVERPROTOCOL_EXPORT DocumentLinkRequest : public Request<
        LanguageClientValue<DocumentLink>, std::nullptr_t, DocumentLinkParams>
{
public:
    DocumentLinkRequest(const DocumentLinkParams &params = DocumentLinkParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/documentLink";
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentLinkResolveRequest : public Request<
        DocumentLink, std::nullptr_t, DocumentLink>
{
public:
    DocumentLinkResolveRequest(const DocumentLink &params = DocumentLink());
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

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT ColorInformation : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(Range range) { insert(rangeKey, range); }

    Color color() const { return typedValue<Color>(colorKey); }
    void setColor(const Color &color) { insert(colorKey, color); }

    bool isValid(QStringList *error) const override
    { return check<Range>(error, rangeKey) && check<Color>(error, colorKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentColorRequest : public Request<
        QList<ColorInformation>, std::nullptr_t, DocumentColorParams>
{
public:
    DocumentColorRequest(const DocumentColorParams &params = DocumentColorParams());
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

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT ColorPresentation : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString label() const { return typedValue<QString>(labelKey); }
    void setLabel(const QString &label) { insert(labelKey, label); }

    Utils::optional<TextEdit> textEdit() const { return optionalValue<TextEdit>(textEditKey); }
    void setTextEdit(const TextEdit &textEdit) { insert(textEditKey, textEdit); }
    void clearTextEdit() { remove(textEditKey); }

    Utils::optional<QList<TextEdit>> additionalTextEdits() const
    { return optionalArray<TextEdit>(additionalTextEditsKey); }
    void setAdditionalTextEdits(const QList<TextEdit> &additionalTextEdits)
    { insertArray(additionalTextEditsKey, additionalTextEdits); }
    void clearAdditionalTextEdits() { remove(additionalTextEditsKey); }

    bool isValid(QStringList *error) const override
    {
        return check<QString>(error, labelKey)
                && checkOptional<TextEdit>(error, textEditKey)
                && checkOptionalArray<TextEdit>(error, additionalTextEditsKey);
    }
};

class LANGUAGESERVERPROTOCOL_EXPORT ColorPresentationRequest : public Request<
        QList<ColorPresentation>, std::nullptr_t, ColorPresentationParams>
{
public:
    ColorPresentationRequest(const ColorPresentationParams &params = ColorPresentationParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/colorPresentation";
};

class DocumentFormattingProperty : public Utils::variant<bool, double, QString>
{
public:
    DocumentFormattingProperty() = default;
    DocumentFormattingProperty(const QJsonValue &value);
    DocumentFormattingProperty(const DocumentFormattingProperty &other)
        : Utils::variant<bool, double, QString>(other) {}

    using variant::variant;
    using variant::operator=;

    bool isValid(QStringList *error) const;
};

class LANGUAGESERVERPROTOCOL_EXPORT FormattingOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    int tabSize() const { return typedValue<int>(tabSizeKey); }
    void setTabSize(int tabSize) { insert(tabSizeKey, tabSize); }

    bool insertSpace() const { return typedValue<bool>(insertSpaceKey); }
    void setInsertSpace(bool insertSpace) { insert(insertSpaceKey, insertSpace); }

    QHash<QString, DocumentFormattingProperty> properties() const;
    void setProperty(const QString &key, const DocumentFormattingProperty &property);
    void removeProperty(const QString &key) { remove(key); }

    bool isValid(QStringList *error) const override;
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

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentFormattingRequest : public Request<
        LanguageClientArray<TextEdit>, std::nullptr_t, DocumentFormattingParams>
{
public:
    DocumentFormattingRequest(const DocumentFormattingParams &params = DocumentFormattingParams());
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

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentRangeFormattingRequest : public Request<
        QList<TextEdit>, std::nullptr_t, DocumentFormattingParams>
{
public:
    DocumentRangeFormattingRequest(const DocumentFormattingParams &params = DocumentFormattingParams());
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

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentOnTypeFormattingRequest : public Request<
        QList<TextEdit>, std::nullptr_t, DocumentFormattingParams>
{
public:
    DocumentOnTypeFormattingRequest(
            const DocumentFormattingParams &params = DocumentFormattingParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/onTypeFormatting";
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

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT RenameRequest : public Request<
        WorkspaceEdit, std::nullptr_t, RenameParams>
{
public:
    RenameRequest(const RenameParams &params = RenameParams());
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/rename";
};

} // namespace LanguageClient
