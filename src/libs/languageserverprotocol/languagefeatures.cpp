// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
constexpr const char PrepareRenameRequest::methodName[];

HoverContent LanguageServerProtocol::Hover::content() const
{
    return HoverContent(value(contentsKey));
}

void Hover::setContent(const HoverContent &content)
{
    if (auto val = std::get_if<MarkedString>(&content))
        insert(contentsKey, *val);
    else if (auto val = std::get_if<MarkupContent>(&content))
        insert(contentsKey, *val);
    else if (auto val = std::get_if<QList<MarkedString>>(&content))
        insert(contentsKey, LanguageClientArray<MarkedString>(*val).toJson());
    else
        QTC_ASSERT_STRING("LanguageClient Using unknown type Hover::setContent");
}

HoverRequest::HoverRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

std::optional<MarkupOrString> ParameterInformation::documentation() const
{
    QJsonValue documentation = value(documentationKey);
    if (documentation.isUndefined())
        return std::nullopt;
    return MarkupOrString(documentation);
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

std::optional<QList<CodeActionKind> > CodeActionParams::CodeActionContext::only() const
{
    return optionalArray<CodeActionKind>(onlyKey);
}

void CodeActionParams::CodeActionContext::setOnly(const QList<CodeActionKind> &only)
{
    insertArray(onlyKey, only);
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

ColorPresentationRequest::ColorPresentationRequest(const ColorPresentationParams &params)
    : Request(methodName, params)
{ }

QHash<QString, DocumentFormattingProperty> FormattingOptions::properties() const
{
    QHash<QString, DocumentFormattingProperty> ret;
    for (const QString &key : keys()) {
        if (key == tabSizeKey || key == insertSpaceKey)
            continue;
        QJsonValue property = value(key.toStdString());
        if (property.isBool())
            ret[key] = property.toBool();
        if (property.isDouble())
            ret[key] = property.toDouble();
        if (property.isString())
            ret[key] = property.toString();
    }
    return ret;
}

void FormattingOptions::setProperty(const std::string_view key, const DocumentFormattingProperty &property)
{
    using namespace std;
    if (auto val = get_if<double>(&property))
        insert(key, *val);
    else if (auto val = get_if<QString>(&property))
        insert(key, *val);
    else if (auto val = get_if<bool>(&property))
        insert(key, *val);
}

DocumentFormattingRequest::DocumentFormattingRequest(const DocumentFormattingParams &params)
    : Request(methodName, params)
{ }

DocumentRangeFormattingRequest::DocumentRangeFormattingRequest(
        const DocumentRangeFormattingParams &params)
    : Request(methodName, params)
{ }

bool DocumentOnTypeFormattingParams::isValid() const
{
    return contains(textDocumentKey) && contains(positionKey) && contains(chKey)
           && contains(optionsKey);
}

DocumentOnTypeFormattingRequest::DocumentOnTypeFormattingRequest(
        const DocumentOnTypeFormattingParams &params)
    : Request(methodName, params)
{ }

PrepareRenameRequest::PrepareRenameRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

bool RenameParams::isValid() const
{
    return contains(textDocumentKey) && contains(positionKey) && contains(newNameKey);
}

RenameRequest::RenameRequest(const RenameParams &params)
    : Request(methodName, params)
{ }

std::optional<DocumentUri> DocumentLink::target() const
{
    if (std::optional<QString> optionalTarget = optionalValue<QString>(targetKey))
        return std::make_optional(DocumentUri::fromProtocol(*optionalTarget));
    return std::nullopt;
}

std::optional<QJsonValue> DocumentLink::data() const
{
    return contains(dataKey) ? std::make_optional(value(dataKey)) : std::nullopt;
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
    if (value.isObject())
        emplace<MarkedLanguageString>(MarkedLanguageString(value.toObject()));
    else
        emplace<QString>(value.toString());
}

bool MarkedString::isValid() const
{
    if (auto markedLanguageString = std::get_if<MarkedLanguageString>(this))
        return markedLanguageString->isValid();
    return true;
}

LanguageServerProtocol::MarkedString::operator QJsonValue() const
{
    if (auto val = std::get_if<QString>(this))
        return *val;
    if (auto val = std::get_if<MarkedLanguageString>(this))
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
        if (markedLanguageString.isValid())
            emplace<MarkedString>(markedLanguageString);
        else
            emplace<MarkupContent>(MarkupContent(object));
    } else if (value.isString()) {
        emplace<MarkedString>(MarkedString(value.toString()));
    }
}

bool HoverContent::isValid() const
{
    if (std::holds_alternative<MarkedString>(*this))
        return std::get<MarkedString>(*this).isValid();
    return true;
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

SignatureHelpRequest::SignatureHelpRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

CodeActionResult::CodeActionResult(const QJsonValue &val)
{
    using ResultArray = QList<std::variant<Command, CodeAction>>;
    if (val.isArray()) {
        const QJsonArray array = val.toArray();
        ResultArray result;
        for (const QJsonValue &val : array) {
            if (val.toObject().value(commandKey).isString()) {
                const Command command(val);
                if (command.isValid())
                    result << command;
            } else {
                const CodeAction action(val);
                if (action.isValid())
                    result << action;
            }
        }
        emplace<ResultArray>(result);
        return;
    }
    emplace<std::nullptr_t>(nullptr);
}

PrepareRenameResult::PrepareRenameResult()
    : std::variant<PlaceHolderResult, Range, std::nullptr_t>(nullptr)
{}

PrepareRenameResult::PrepareRenameResult(
    const std::variant<PlaceHolderResult, Range, std::nullptr_t> &val)
    : std::variant<PlaceHolderResult, Range, std::nullptr_t>(val)
{}

PrepareRenameResult::PrepareRenameResult(const PlaceHolderResult &val)
    : std::variant<PlaceHolderResult, Range, std::nullptr_t>(val)

{}

PrepareRenameResult::PrepareRenameResult(const Range &val)
    : std::variant<PlaceHolderResult, Range, std::nullptr_t>(val)
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

std::optional<QJsonValue> CodeLens::data() const
{
    return contains(dataKey) ? std::make_optional(value(dataKey)) : std::nullopt;
}

HoverResult::HoverResult(const QJsonValue &value)
{
    if (value.isObject())
        emplace<Hover>(Hover(value.toObject()));
    else
        emplace<std::nullptr_t>(nullptr);
}

bool HoverResult::isValid() const
{
    if (auto hover = std::get_if<Hover>(this))
        return hover->isValid();
    return true;
}

} // namespace LanguageServerProtocol
