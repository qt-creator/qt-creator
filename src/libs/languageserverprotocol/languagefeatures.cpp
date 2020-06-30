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

#include "languagefeatures.h"

#include <cstddef>

namespace LanguageServerProtocol {

constexpr const char HoverRequest::methodName[];
constexpr const char GotoDefinitionRequest::methodName[];
constexpr const char GotoTypeDefinitionRequest::methodName[];
constexpr const char GotoImplementationRequest::methodName[];
constexpr const char FindReferencesRequest::methodName[];
constexpr const char DocumentHighlightsRequest::methodName[];
constexpr const char DocumentSymbolsRequest::methodName[];
constexpr const char CodeActionRequest::methodName[];
constexpr const char CodeLensRequest::methodName[];
constexpr const char CodeLensResolveRequest::methodName[];
constexpr const char DocumentLinkRequest::methodName[];
constexpr const char DocumentLinkResolveRequest::methodName[];
constexpr const char DocumentColorRequest::methodName[];
constexpr const char ColorPresentationRequest::methodName[];
constexpr const char DocumentFormattingRequest::methodName[];
constexpr const char DocumentRangeFormattingRequest::methodName[];
constexpr const char DocumentOnTypeFormattingRequest::methodName[];
constexpr const char RenameRequest::methodName[];
constexpr const char SignatureHelpRequest::methodName[];
constexpr const char SemanticHighlightNotification::methodName[];
constexpr const char PrepareRenameRequest::methodName[];

HoverContent LanguageServerProtocol::Hover::content() const
{
    return HoverContent(value(contentsKey));
}

void Hover::setContent(const HoverContent &content)
{
    if (auto val = Utils::get_if<MarkedString>(&content))
        insert(contentsKey, *val);
    else if (auto val = Utils::get_if<MarkupContent>(&content))
        insert(contentsKey, *val);
    else if (auto val = Utils::get_if<QList<MarkedString>>(&content))
        insert(contentsKey, LanguageClientArray<MarkedString>(*val).toJson());
    else
        QTC_ASSERT_STRING("LanguageClient Using unknown type Hover::setContent");
}

HoverRequest::HoverRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

Utils::optional<MarkupOrString> ParameterInformation::documentation() const
{
    QJsonValue documentation = value(documentationKey);
    if (documentation.isUndefined())
        return Utils::nullopt;
    return MarkupOrString(documentation);
}

bool SignatureHelp::isValid(ErrorHierarchy *error) const
{
    return checkArray<SignatureInformation>(error, signaturesKey);
}

GotoDefinitionRequest::GotoDefinitionRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

GotoTypeDefinitionRequest::GotoTypeDefinitionRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

GotoImplementationRequest::GotoImplementationRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

FindReferencesRequest::FindReferencesRequest(const ReferenceParams &params)
    : Request(methodName, params)
{ }

DocumentHighlightsRequest::DocumentHighlightsRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

DocumentSymbolsRequest::DocumentSymbolsRequest(const DocumentSymbolParams &params)
    : Request(methodName, params)
{ }

Utils::optional<QList<CodeActionKind> > CodeActionParams::CodeActionContext::only() const
{
    return optionalArray<CodeActionKind>(onlyKey);
}

void CodeActionParams::CodeActionContext::setOnly(const QList<CodeActionKind> &only)
{
    insertArray(onlyKey, only);
}

bool CodeActionParams::CodeActionContext::isValid(ErrorHierarchy *error) const
{
    return checkArray<Diagnostic>(error, diagnosticsKey);
}

bool CodeActionParams::isValid(ErrorHierarchy *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<Range>(error, rangeKey)
            && check<CodeActionContext>(error, contextKey);
}

CodeActionRequest::CodeActionRequest(const CodeActionParams &params)
    : Request(methodName, params)
{ }

CodeLensRequest::CodeLensRequest(const CodeLensParams &params)
    : Request(methodName, params)
{ }

CodeLensResolveRequest::CodeLensResolveRequest(const CodeLens &params)
    : Request(methodName, params)
{ }

DocumentLinkRequest::DocumentLinkRequest(const DocumentLinkParams &params)
    : Request(methodName, params)
{ }

DocumentLinkResolveRequest::DocumentLinkResolveRequest(const DocumentLink &params)
    : Request(methodName, params)
{ }

DocumentColorRequest::DocumentColorRequest(const DocumentColorParams &params)
    : Request(methodName, params)
{ }

bool Color::isValid(ErrorHierarchy *error) const
{
    return check<int>(error, redKey)
            && check<int>(error, greenKey)
            && check<int>(error, blueKey)
            && check<int>(error, alphaKey);
}

bool ColorPresentationParams::isValid(ErrorHierarchy *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<Color>(error, colorInfoKey)
            && check<Range>(error, rangeKey);
}

ColorPresentationRequest::ColorPresentationRequest(const ColorPresentationParams &params)
    : Request(methodName, params)
{ }

QHash<QString, DocumentFormattingProperty> FormattingOptions::properties() const
{
    QHash<QString, DocumentFormattingProperty> ret;
    for (const QString &key : keys()) {
        if (key == tabSizeKey || key == insertSpaceKey)
            continue;
        QJsonValue property = value(key);
        if (property.isBool())
            ret[key] = property.toBool();
        if (property.isDouble())
            ret[key] = property.toDouble();
        if (property.isString())
            ret[key] = property.toString();
    }
    return ret;
}

void FormattingOptions::setProperty(const QString &key, const DocumentFormattingProperty &property)
{
    using namespace Utils;
    if (auto val = get_if<double>(&property))
        insert(key, *val);
    else if (auto val = get_if<QString>(&property))
        insert(key, *val);
    else if (auto val = get_if<bool>(&property))
        insert(key, *val);
}

bool FormattingOptions::isValid(ErrorHierarchy *error) const
{
    return Utils::allOf(keys(), [this, &error](auto key){
        return (key == tabSizeKey && this->check<int>(error, key))
                || (key == insertSpaceKey && this->check<bool>(error, key))
                || this->check<DocumentFormattingProperty>(error, key);
    });
}

bool DocumentFormattingParams::isValid(ErrorHierarchy *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<FormattingOptions>(error, optionsKey);
}

DocumentFormattingRequest::DocumentFormattingRequest(const DocumentFormattingParams &params)
    : Request(methodName, params)
{ }

bool DocumentRangeFormattingParams::isValid(ErrorHierarchy *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<Range>(error, rangeKey)
            && check<FormattingOptions>(error, optionsKey);
}

DocumentRangeFormattingRequest::DocumentRangeFormattingRequest(
        const DocumentRangeFormattingParams &params)
    : Request(methodName, params)
{ }

bool DocumentOnTypeFormattingParams::isValid(ErrorHierarchy *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<Position>(error, positionKey)
            && check<QString>(error, chKey)
            && check<FormattingOptions>(error, optionsKey);
}

DocumentOnTypeFormattingRequest::DocumentOnTypeFormattingRequest(
        const DocumentFormattingParams &params)
    : Request(methodName, params)
{ }

PrepareRenameRequest::PrepareRenameRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

bool RenameParams::isValid(ErrorHierarchy *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<Position>(error, positionKey)
            && check<QString>(error, newNameKey);
}

RenameRequest::RenameRequest(const RenameParams &params)
    : Request(methodName, params)
{ }

Utils::optional<DocumentUri> DocumentLink::target() const
{
    Utils::optional<QString> optionalTarget = optionalValue<QString>(targetKey);
    return optionalTarget.has_value()
            ? Utils::make_optional(DocumentUri::fromProtocol(optionalTarget.value()))
            : Utils::nullopt;
}

TextDocumentParams::TextDocumentParams()
    : TextDocumentParams(TextDocumentIdentifier())
{ }

TextDocumentParams::TextDocumentParams(const TextDocumentIdentifier &identifier)
    : JsonObject()
{
    setTextDocument(identifier);
}

GotoResult::GotoResult(const QJsonValue &value)
{
    if (value.isArray()) {
        QList<Location> locations;
        for (auto arrayValue : value.toArray()) {
            if (arrayValue.isObject())
                locations.append(Location(arrayValue.toObject()));
        }
        emplace<QList<Location>>(locations);
    } else if (value.isObject()) {
        emplace<Location>(value.toObject());
    } else {
        emplace<std::nullptr_t>(nullptr);
    }
}

template<typename Symbol>
QList<Symbol> documentSymbolsResultArray(const QJsonArray &array)
{
    QList<Symbol> ret;
    for (const auto &arrayValue : array) {
        if (arrayValue.isObject())
            ret << Symbol(arrayValue.toObject());
    }
    return ret;
}

DocumentSymbolsResult::DocumentSymbolsResult(const QJsonValue &value)
{
    if (value.isArray()) {
        QJsonArray array = value.toArray();
        if (array.isEmpty()) {
            *this = QList<SymbolInformation>();
        } else {
            QJsonObject arrayObject = array.first().toObject();
            if (arrayObject.contains(rangeKey))
                *this = documentSymbolsResultArray<DocumentSymbol>(array);
            else
                *this = documentSymbolsResultArray<SymbolInformation>(array);
        }
    } else {
        *this = nullptr;
    }
}

DocumentHighlightsResult::DocumentHighlightsResult(const QJsonValue &value)
{
    if (value.isArray()) {
        QList<DocumentHighlight> highlights;
        for (auto arrayValue : value.toArray()) {
            if (arrayValue.isObject())
                highlights.append(DocumentHighlight(arrayValue.toObject()));
        }
        *this = highlights;
    } else {
        *this = nullptr;
    }
}

MarkedString::MarkedString(const QJsonValue &value)
{
    if (value.isObject()) {
        MarkedLanguageString string(value.toObject());
        if (string.isValid(nullptr))
            emplace<MarkedLanguageString>(string);
    } else if (value.isString()) {
        emplace<QString>(value.toString());
    }
}

LanguageServerProtocol::MarkedString::operator QJsonValue() const
{
    if (auto val = Utils::get_if<QString>(this))
        return *val;
    if (auto val = Utils::get_if<MarkedLanguageString>(this))
        return QJsonValue(*val);
    return {};
}

HoverContent::HoverContent(const QJsonValue &value)
{
    if (value.isArray()) {
        emplace<QList<MarkedString>>(LanguageClientArray<MarkedString>(value).toList());
    } else if (value.isObject()) {
        const QJsonObject &object = value.toObject();
        MarkedLanguageString markedLanguageString(object);
        if (markedLanguageString.isValid(nullptr))
            emplace<MarkedString>(markedLanguageString);
        else
            emplace<MarkupContent>(MarkupContent(object));
    } else if (value.isString()) {
        emplace<MarkedString>(MarkedString(value.toString()));
    }
}

bool HoverContent::isValid(ErrorHierarchy *errorHierarchy) const
{
    if (Utils::holds_alternative<MarkedString>(*this)
            || Utils::holds_alternative<MarkupContent>(*this)
            || Utils::holds_alternative<QList<MarkedString>>(*this)) {
        return true;
    }
    if (errorHierarchy) {
        errorHierarchy->setError(
            QCoreApplication::translate("LanguageServerProtocol::HoverContent",
                                        "HoverContent should be either MarkedString, "
                                        "MarkupContent, or QList<MarkedString>."));
    }
    return false;
}

DocumentFormattingProperty::DocumentFormattingProperty(const QJsonValue &value)
{
    if (value.isBool())
        *this = value.toBool();
    if (value.isDouble())
        *this = value.toDouble();
    if (value.isString())
        *this = value.toString();
}

bool DocumentFormattingProperty::isValid(ErrorHierarchy *error) const
{
    if (Utils::holds_alternative<bool>(*this)
            || Utils::holds_alternative<double>(*this)
            || Utils::holds_alternative<QString>(*this)) {
        return true;
    }
    if (error) {
        error->setError(QCoreApplication::translate(
            "LanguageServerProtocol::MarkedString",
            "DocumentFormattingProperty should be either bool, double, or QString."));
    }
    return false;
}

SignatureHelpRequest::SignatureHelpRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

CodeActionResult::CodeActionResult(const QJsonValue &val)
{
    using ResultArray = QList<Utils::variant<Command, CodeAction>>;
    if (val.isArray()) {
        const QJsonArray array = val.toArray();
        ResultArray result;
        for (const QJsonValue &val : array) {
            Command command(val);
            if (command.isValid(nullptr))
                result << command;
            else
                result << CodeAction(val);
        }
        emplace<ResultArray>(result);
        return;
    }
    emplace<std::nullptr_t>(nullptr);
}

bool CodeAction::isValid(ErrorHierarchy *error) const
{
    return check<QString>(error, titleKey)
           && checkOptional<CodeActionKind>(error, codeActionKindKey)
           && checkOptionalArray<Diagnostic>(error, diagnosticsKey)
           && checkOptional<WorkspaceEdit>(error, editKey)
           && checkOptional<Command>(error, commandKey);
}

Utils::optional<QList<SemanticHighlightToken>> SemanticHighlightingInformation::tokens() const
{
    QList<SemanticHighlightToken> resultTokens;

    const QByteArray tokensByteArray = QByteArray::fromBase64(
        typedValue<QString>(tokensKey).toLocal8Bit());
    constexpr int tokensByteSize = 8;
    int index = 0;
    while (index + tokensByteSize <= tokensByteArray.size()) {
        resultTokens << SemanticHighlightToken(tokensByteArray.mid(index, tokensByteSize));
        index += tokensByteSize;
    }
    return Utils::make_optional(resultTokens);
}

void SemanticHighlightingInformation::setTokens(const QList<SemanticHighlightToken> &tokens)
{
    QByteArray byteArray;
    byteArray.reserve(8 * tokens.size());
    for (const SemanticHighlightToken &token : tokens)
        token.appendToByteArray(byteArray);
    insert(tokensKey, QString::fromLocal8Bit(byteArray.toBase64()));
}

SemanticHighlightToken::SemanticHighlightToken(const QByteArray &token)
{
    QTC_ASSERT(token.size() == 8, return );
    character = ( quint32(token.at(0)) << 24
                | quint32(token.at(1)) << 16
                | quint32(token.at(2)) << 8
                | quint32(token.at(3)));

    length = quint16(token.at(4) << 8 | token.at(5));

    scope = quint16(token.at(6) << 8 | token.at(7));
}

void SemanticHighlightToken::appendToByteArray(QByteArray &byteArray) const
{
    byteArray.append(char((character & 0xff000000) >> 24));
    byteArray.append(char((character & 0x00ff0000) >> 16));
    byteArray.append(char((character & 0x0000ff00) >> 8));
    byteArray.append(char((character & 0x000000ff)));
    byteArray.append(char((length & 0xff00) >> 8));
    byteArray.append(char((length & 0x00ff)));
    byteArray.append(char((scope & 0xff00) >> 8));
    byteArray.append(char((scope & 0x00ff)));
}

Utils::variant<VersionedTextDocumentIdentifier, TextDocumentIdentifier>
SemanticHighlightingParams::textDocument() const
{
    VersionedTextDocumentIdentifier textDocument = fromJsonValue<VersionedTextDocumentIdentifier>(
        value(textDocumentKey));
    ErrorHierarchy error;
    if (!textDocument.isValid(&error)) {
        return TextDocumentIdentifier(textDocument);
    } else {
        return textDocument;
    }
}

bool SemanticHighlightingParams::isValid(ErrorHierarchy *error) const
{
    return checkVariant<VersionedTextDocumentIdentifier, TextDocumentIdentifier>(error,
                                                                                 textDocumentKey)
           && checkArray<SemanticHighlightingInformation>(error, linesKey);
}

PrepareRenameResult::PrepareRenameResult()
    : Utils::variant<PlaceHolderResult, Range, std::nullptr_t>(nullptr)
{}

PrepareRenameResult::PrepareRenameResult(
    const Utils::variant<PlaceHolderResult, Range, std::nullptr_t> &val)
    : Utils::variant<PlaceHolderResult, Range, std::nullptr_t>(val)
{}

PrepareRenameResult::PrepareRenameResult(const PlaceHolderResult &val)
    : Utils::variant<PlaceHolderResult, Range, std::nullptr_t>(val)

{}

PrepareRenameResult::PrepareRenameResult(const Range &val)
    : Utils::variant<PlaceHolderResult, Range, std::nullptr_t>(val)
{}

PrepareRenameResult::PrepareRenameResult(const QJsonValue &val)
{
    if (val.isNull()) {
        emplace<std::nullptr_t>(nullptr);
    } else if (val.isObject()) {
        const QJsonObject object = val.toObject();
        if (object.keys().contains(rangeKey))
            emplace<PlaceHolderResult>(PlaceHolderResult(object));
        else
            emplace<Range>(Range(object));
    }
}

SemanticHighlightNotification::SemanticHighlightNotification(const SemanticHighlightingParams &params)
    : Notification(methodName, params)
{}

} // namespace LanguageServerProtocol
