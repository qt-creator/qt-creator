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

MarkedString LanguageServerProtocol::Hover::content() const
{
    return MarkedString(value(contentKey));
}

void Hover::setContent(const MarkedString &content)
{
    if (auto val = Utils::get_if<MarkedLanguageString>(&content))
        insert(contentKey, *val);
    else if (auto val = Utils::get_if<MarkupContent>(&content))
        insert(contentKey, *val);
    else if (auto val = Utils::get_if<QList<MarkedLanguageString>>(&content))
        insert(contentKey, LanguageClientArray<MarkedLanguageString>(*val).toJson());
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

bool SignatureHelp::isValid(QStringList *error) const
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

bool CodeActionParams::CodeActionContext::isValid(QStringList *error) const
{
    return checkArray<Diagnostic>(error, diagnosticsKey);
}

bool CodeActionParams::isValid(QStringList *error) const
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

bool Color::isValid(QStringList *error) const
{
    return check<int>(error, redKey)
            && check<int>(error, greenKey)
            && check<int>(error, blueKey)
            && check<int>(error, alphaKey);
}

bool ColorPresentationParams::isValid(QStringList *error) const
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

bool FormattingOptions::isValid(QStringList *error) const
{
    return Utils::allOf(keys(), [this, &error](auto key){
        return (key == tabSizeKey && this->check<int>(error, key))
                || (key == insertSpaceKey && this->check<bool>(error, key))
                || this->check<DocumentFormattingProperty>(error, key);
    });
}

bool DocumentFormattingParams::isValid(QStringList *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<FormattingOptions>(error, optionsKey);
}

DocumentFormattingRequest::DocumentFormattingRequest(const DocumentFormattingParams &params)
    : Request(methodName, params)
{ }

bool DocumentRangeFormattingParams::isValid(QStringList *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<Range>(error, rangeKey)
            && check<FormattingOptions>(error, optionsKey);
}

DocumentRangeFormattingRequest::DocumentRangeFormattingRequest(
        const DocumentFormattingParams &params)
    : Request(methodName, params)
{ }

bool DocumentOnTypeFormattingParams::isValid(QStringList *error) const
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

bool RenameParams::isValid(QStringList *error) const
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

DocumentSymbolsResult::DocumentSymbolsResult(const QJsonValue &value)
{
    if (value.isArray()) {
        QList<SymbolInformation> symbols;
        for (auto arrayValue : value.toArray()) {
            if (arrayValue.isObject())
                symbols.append(SymbolInformation(arrayValue.toObject()));
        }
        *this = symbols;
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
    if (value.isArray()) {
        emplace<QList<MarkedLanguageString>>(
            LanguageClientArray<MarkedLanguageString>(value).toList());
    } else if (value.isObject()) {
        const QJsonObject &object = value.toObject();
        MarkedLanguageString markedLanguageString(object);
        if (markedLanguageString.isValid(nullptr))
            emplace<MarkedLanguageString>(markedLanguageString);
        else
            emplace<MarkupContent>(MarkupContent(object));
    }
}

bool MarkedString::isValid(QStringList *errorHierarchy) const
{
    if (Utils::holds_alternative<MarkedLanguageString>(*this)
            || Utils::holds_alternative<MarkupContent>(*this)
            || Utils::holds_alternative<QList<MarkedLanguageString>>(*this)) {
        return true;
    }
    if (errorHierarchy) {
        *errorHierarchy << QCoreApplication::translate(
                               "LanguageServerProtocol::MarkedString",
                               "MarkedString should be either MarkedLanguageString, "
                               "MarkupContent, or QList<MarkedLanguageString>.");
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

bool DocumentFormattingProperty::isValid(QStringList *error) const
{
    if (Utils::holds_alternative<bool>(*this)
            || Utils::holds_alternative<double>(*this)
            || Utils::holds_alternative<QString>(*this)) {
        return true;
    }
    if (error) {
        *error << QCoreApplication::translate(
                      "LanguageServerProtocol::MarkedString",
                      "DocumentFormattingProperty should be either bool, double, or QString.");
    }
    return false;
}

SignatureHelpRequest::SignatureHelpRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{ }

} // namespace LanguageServerProtocol
