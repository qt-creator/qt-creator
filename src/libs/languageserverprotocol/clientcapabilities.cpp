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

#include "clientcapabilities.h"

namespace LanguageServerProtocol {

Utils::optional<QList<SymbolKind> > SymbolCapabilities::SymbolKindCapabilities::valueSet() const
{
    if (Utils::optional<QList<int>> array = optionalArray<int>(valueSetKey)) {
        return Utils::make_optional(Utils::transform(*array, [] (int value) {
            return static_cast<SymbolKind>(value);
        }));
    }
    return Utils::nullopt;
}

void SymbolCapabilities::SymbolKindCapabilities::setValueSet(const QList<SymbolKind> &valueSet)
{
    insert(valueSetKey, enumArrayToJsonArray<SymbolKind>(valueSet));
}

WorkspaceClientCapabilities::WorkspaceClientCapabilities()
{
    setWorkspaceFolders(true);
}

Utils::optional<Utils::variant<bool, QJsonObject>> SemanticTokensClientCapabilities::Requests::range()
    const
{
    using RetType = Utils::variant<bool, QJsonObject>;
    const QJsonValue &rangeOptions = value(rangeKey);
    if (rangeOptions.isBool())
        return RetType(rangeOptions.toBool());
    if (rangeOptions.isObject())
        return RetType(rangeOptions.toObject());
    return Utils::nullopt;
}

void SemanticTokensClientCapabilities::Requests::setRange(
    const Utils::variant<bool, QJsonObject> &range)
{
    insertVariant<bool, QJsonObject>(rangeKey, range);
}

Utils::optional<Utils::variant<bool, FullSemanticTokenOptions>>
SemanticTokensClientCapabilities::Requests::full() const
{
    using RetType = Utils::variant<bool, FullSemanticTokenOptions>;
    const QJsonValue &fullOptions = value(fullKey);
    if (fullOptions.isBool())
        return RetType(fullOptions.toBool());
    if (fullOptions.isObject())
        return RetType(FullSemanticTokenOptions(fullOptions.toObject()));
    return Utils::nullopt;
}

void SemanticTokensClientCapabilities::Requests::setFull(
    const Utils::variant<bool, FullSemanticTokenOptions> &full)
{
    insertVariant<bool, FullSemanticTokenOptions>(fullKey, full);
}

Utils::optional<SemanticTokensClientCapabilities> TextDocumentClientCapabilities::semanticTokens()
    const
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

} // namespace LanguageServerProtocol
