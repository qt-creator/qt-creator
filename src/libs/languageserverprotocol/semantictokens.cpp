// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "semantictokens.h"

namespace LanguageServerProtocol {

bool SemanticTokensLegend::isValid() const
{
    return contains(tokenTypesKey) && contains(tokenModifiersKey);
}

QMap<QString, int> SemanticTokens::defaultTokenTypesMap()
{
    QMap<QString, int> map;
    map.insert("namespace", namespaceToken);
    map.insert("type", typeToken);
    map.insert("class", classToken);
    map.insert("enum", enumToken);
    map.insert("interface", interfaceToken);
    map.insert("struct", structToken);
    map.insert("typeParameter", typeParameterToken);
    map.insert("parameter", parameterToken);
    map.insert("variable", variableToken);
    map.insert("property", propertyToken);
    map.insert("enumMember", enumMemberToken);
    map.insert("event", eventToken);
    map.insert("function", functionToken);
    map.insert("method", methodToken);
    map.insert("macro", macroToken);
    map.insert("keyword", keywordToken);
    map.insert("modifier", modifierToken);
    map.insert("comment", commentToken);
    map.insert("string", stringToken);
    map.insert("number", numberToken);
    map.insert("regexp", regexpToken);
    map.insert("operator", operatorToken);
    return map;
}

QMap<QString, int> SemanticTokens::defaultTokenModifiersMap()
{
    QMap<QString, int> map;
    map.insert("declaration", declarationModifier);
    map.insert("definition", definitionModifier);
    map.insert("readonly", readonlyModifier);
    map.insert("static", staticModifier);
    map.insert("deprecated", deprecatedModifier);
    map.insert("abstract", abstractModifier);
    map.insert("async", asyncModifier);
    map.insert("modification", modificationModifier);
    map.insert("documentation", documentationModifier);
    map.insert("defaultLibrary", defaultLibraryModifier);
    return map;
}

static int convertModifiers(int modifiersData, const QList<int> &tokenModifiers)
{
    int result = 0;
    for (int i = 0; i < tokenModifiers.size() && modifiersData > 0; ++i) {
        if (modifiersData & 0x1) {
            const int modifier = tokenModifiers[i];
            if (modifier > 0)
                result |= modifier;
        }
        modifiersData = modifiersData >> 1;
    }
    return result;
}

QList<SemanticToken> SemanticTokens::toTokens(const QList<int> &tokenTypes,
                                              const QList<int> &tokenModifiers) const
{
    const QList<int> &data = this->data();
    if (data.size() % 5 != 0)
        return {};
    QList<SemanticToken> tokens;
    tokens.reserve(int(data.size() / 5));
    auto end = data.end();
    for (auto it = data.begin(); it != end; it += 5) {
        SemanticToken token;
        token.deltaLine = *(it);
        token.deltaStart = *(it + 1);
        token.length = *(it + 2);
        token.tokenIndex = *(it + 3);
        token.tokenType = tokenTypes.value(token.tokenIndex, -1);
        token.rawTokenModifiers = *(it + 4);
        token.tokenModifiers = convertModifiers(token.rawTokenModifiers, tokenModifiers);
        tokens << token;
    }
    return tokens;
}

bool SemanticTokensRangeParams::isValid() const
{
    return SemanticTokensParams::isValid() && contains(rangeKey);
}

SemanticTokensFullRequest::SemanticTokensFullRequest(const SemanticTokensParams &params)
    : Request(methodName, params)
{}

SemanticTokensRangeRequest::SemanticTokensRangeRequest(const SemanticTokensRangeParams &params)
    : Request(methodName, params)
{}

SemanticTokensResult::SemanticTokensResult(const QJsonValue &value)
{
    if (value.isObject())
        emplace<SemanticTokens>(SemanticTokens(value.toObject()));
    else
        emplace<std::nullptr_t>(nullptr);
}

SemanticTokensFullDeltaRequest::SemanticTokensFullDeltaRequest(const SemanticTokensDeltaParams &params)
    : Request(methodName, params)
{}

bool SemanticTokensDeltaParams::isValid() const
{
    return SemanticTokensParams::isValid() && contains(previousResultIdKey);
}

SemanticTokensDeltaResult::SemanticTokensDeltaResult(const QJsonValue &value)
{
    if (value.isObject()) {
        QJsonObject object = value.toObject();
        if (object.contains(editsKey))
            emplace<SemanticTokensDelta>(object);
        else
            emplace<SemanticTokens>(object);
    } else {
        emplace<std::nullptr_t>(nullptr);
    }
}

SemanticTokensRefreshRequest::SemanticTokensRefreshRequest()
    : Request(methodName, nullptr)
{}

} // namespace LanguageServerProtocol
