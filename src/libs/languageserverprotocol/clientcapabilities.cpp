// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clientcapabilities.h"

namespace LanguageServerProtocol {

std::optional<QList<SymbolKind>> SymbolCapabilities::SymbolKindCapabilities::valueSet() const
{
    if (std::optional<QList<int>> array = optionalArray<int>(valueSetKey)) {
        return std::make_optional(
            Utils::transform(*array, [](int value) { return static_cast<SymbolKind>(value); }));
    }
    return std::nullopt;
}

void SymbolCapabilities::SymbolKindCapabilities::setValueSet(const QList<SymbolKind> &valueSet)
{
    insert(valueSetKey, enumArrayToJsonArray<SymbolKind>(valueSet));
}

WorkspaceClientCapabilities::WorkspaceClientCapabilities()
{
    setWorkspaceFolders(true);
}

std::optional<std::variant<bool, QJsonObject>> SemanticTokensClientCapabilities::Requests::range()
    const
{
    using RetType = std::variant<bool, QJsonObject>;
    const QJsonValue &rangeOptions = value(rangeKey);
    if (rangeOptions.isBool())
        return RetType(rangeOptions.toBool());
    if (rangeOptions.isObject())
        return RetType(rangeOptions.toObject());
    return std::nullopt;
}

void SemanticTokensClientCapabilities::Requests::setRange(
    const std::variant<bool, QJsonObject> &range)
{
    insertVariant<bool, QJsonObject>(rangeKey, range);
}

std::optional<std::variant<bool, FullSemanticTokenOptions>>
SemanticTokensClientCapabilities::Requests::full() const
{
    using RetType = std::variant<bool, FullSemanticTokenOptions>;
    const QJsonValue &fullOptions = value(fullKey);
    if (fullOptions.isBool())
        return RetType(fullOptions.toBool());
    if (fullOptions.isObject())
        return RetType(FullSemanticTokenOptions(fullOptions.toObject()));
    return std::nullopt;
}

void SemanticTokensClientCapabilities::Requests::setFull(
    const std::variant<bool, FullSemanticTokenOptions> &full)
{
    insertVariant<bool, FullSemanticTokenOptions>(fullKey, full);
}

std::optional<SemanticTokensClientCapabilities> TextDocumentClientCapabilities::semanticTokens() const
{
    return optionalValue<SemanticTokensClientCapabilities>(semanticTokensKey);
}

void TextDocumentClientCapabilities::setSemanticTokens(
    const SemanticTokensClientCapabilities &semanticTokens)
{
    insert(semanticTokensKey, semanticTokens);
}

bool SemanticTokensClientCapabilities::isValid() const
{
    return contains(requestsKey) && contains(tokenTypesKey) && contains(tokenModifiersKey)
           && contains(formatsKey);
}

const char resourceOperationCreate[] = "create";
const char resourceOperationRename[] = "rename";
const char resourceOperationDelete[] = "delete";

std::optional<QList<WorkspaceClientCapabilities::WorkspaceEditCapabilities::ResourceOperationKind>>
WorkspaceClientCapabilities::WorkspaceEditCapabilities::resourceOperations() const
{
    if (!contains(resourceOperationsKey))
        return std::nullopt;
    QList<ResourceOperationKind> result;
    for (const QJsonValue &value : this->value(resourceOperationsKey).toArray()) {
        const QString str = value.toString();
        if (str == resourceOperationCreate)
            result << ResourceOperationKind::Create;
        else if (str == resourceOperationRename)
            result << ResourceOperationKind::Rename;
        else if (str == resourceOperationDelete)
            result << ResourceOperationKind::Delete;
    }
    return result;
}

void WorkspaceClientCapabilities::WorkspaceEditCapabilities::setResourceOperations(
    const QList<ResourceOperationKind> &resourceOperations)
{
    QJsonArray array;
    for (const auto &kind : resourceOperations) {
        switch (kind) {
        case ResourceOperationKind::Create:
            array << resourceOperationCreate;
            break;
        case ResourceOperationKind::Rename:
            array << resourceOperationRename;
            break;
        case ResourceOperationKind::Delete:
            array << resourceOperationDelete;
            break;
        }
    }
    insert(resourceOperationsKey, array);
}

} // namespace LanguageServerProtocol
