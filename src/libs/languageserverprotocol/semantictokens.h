// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonkeys.h"
#include "jsonobject.h"
#include "jsonrpcmessages.h"
#include "languageserverprotocol_global.h"
#include "lsptypes.h"

namespace LanguageServerProtocol {

struct LANGUAGESERVERPROTOCOL_EXPORT SemanticToken
{
    int deltaLine = 0;
    int deltaStart = 0;
    int length = 0;
    int tokenIndex = 0;
    int tokenType = 0;
    int rawTokenModifiers = 0;
    int tokenModifiers = 0;
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensLegend : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The token types a server uses.
    QList<QString> tokenTypes() const { return array<QString>(tokenTypesKey); }
    void setTokenTypes(const QList<QString> &tokenTypes) { insertArray(tokenTypesKey, tokenTypes); }

    // The token modifiers a server uses.
    QList<QString> tokenModifiers() const { return array<QString>(tokenModifiersKey); }
    void setTokenModifiers(const QList<QString> &value) { insertArray(tokenModifiersKey, value); }

    bool isValid() const override;
};

enum SemanticTokenTypes {
    namespaceToken,
    typeToken,
    classToken,
    enumToken,
    interfaceToken,
    structToken,
    typeParameterToken,
    parameterToken,
    variableToken,
    propertyToken,
    enumMemberToken,
    eventToken,
    functionToken,
    methodToken,
    macroToken,
    keywordToken,
    modifierToken,
    commentToken,
    stringToken,
    numberToken,
    regexpToken,
    operatorToken
};

enum SemanticTokenModifiers {
    declarationModifier = 0x1,
    definitionModifier = 0x2,
    readonlyModifier = 0x4,
    staticModifier = 0x8,
    deprecatedModifier = 0x10,
    abstractModifier = 0x20,
    asyncModifier = 0x40,
    modificationModifier = 0x80,
    documentationModifier = 0x100,
    defaultLibraryModifier = 0x200
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    bool isValid() const override { return contains(textDocumentKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensDeltaParams : public SemanticTokensParams
{
public:
    using SemanticTokensParams::SemanticTokensParams;

    QString previousResultId() const { return typedValue<QString>(previousResultIdKey); }
    void setPreviousResultId(const QString &previousResultId)
    {
        insert(previousResultIdKey, previousResultId);
    }

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensRangeParams : public SemanticTokensParams
{
public:
    using SemanticTokensParams::SemanticTokensParams;

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokens : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /**
     * An optional result id. If provided and clients support delta updating
     * the client will include the result id in the next semantic token request.
     * A server can then instead of computing all semantic tokens again simply
     * send a delta.
     */
    std::optional<QString> resultId() const { return optionalValue<QString>(resultIdKey); }
    void setResultId(const QString &resultId) { insert(resultIdKey, resultId); }
    void clearResultId() { remove(resultIdKey); }

    /// The actual tokens.
    QList<int> data() const { return array<int>(dataKey); }
    void setData(const QList<int> &value) { insertArray(dataKey, value); }

    bool isValid() const override { return contains(dataKey); }

    QList<SemanticToken> toTokens(const QList<int> &tokenTypes,
                                  const QList<int> &tokenModifiers) const;
    static QMap<QString, int> defaultTokenTypesMap();
    static QMap<QString, int> defaultTokenModifiersMap();
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensResult
    : public std::variant<SemanticTokens, std::nullptr_t>
{
public:
    using variant::variant;
    explicit SemanticTokensResult(const QJsonValue &value);
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensFullRequest
    : public Request<SemanticTokensResult, std::nullptr_t, SemanticTokensParams>
{
public:
    explicit SemanticTokensFullRequest(const SemanticTokensParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/semanticTokens/full";
};

class SemanticTokensEdit : public JsonObject
{
public:
    using JsonObject::JsonObject;

    int start() const { return typedValue<int>(startKey); }
    void setStart(int start) { insert(startKey, start); }

    int deleteCount() const { return typedValue<int>(deleteCountKey); }
    void setDeleteCount(int deleteCount) { insert(deleteCountKey, deleteCount); }

    std::optional<QList<int>> data() const { return optionalArray<int>(dataKey); }
    void setData(const QList<int> &value) { insertArray(dataKey, value); }
    void clearData() { remove(dataKey); }

    int dataSize() const { return contains(dataKey) ? value(dataKey).toArray().size() : 0; }

    bool isValid() const override { return contains(dataKey) && contains(deleteCountKey); }
};

class SemanticTokensDelta : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString resultId() const { return typedValue<QString>(resultIdKey); }
    void setResultId(const QString &resultId) { insert(resultIdKey, resultId); }

    QList<SemanticTokensEdit> edits() const { return array<SemanticTokensEdit>(editsKey); }
    void setEdits(const QList<SemanticTokensEdit> &edits) { insertArray(editsKey, edits); }

    bool isValid() const override { return contains(resultIdKey) && contains(editsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensDeltaResult
    : public std::variant<SemanticTokens, SemanticTokensDelta, std::nullptr_t>
{
public:
    using variant::variant;
    explicit SemanticTokensDeltaResult(const QJsonValue &value);
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensFullDeltaRequest
    : public Request<SemanticTokensDeltaResult, std::nullptr_t, SemanticTokensDeltaParams>
{
public:
    explicit SemanticTokensFullDeltaRequest(const SemanticTokensDeltaParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/semanticTokens/full/delta";
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensRangeRequest
    : public Request<SemanticTokensResult, std::nullptr_t, SemanticTokensRangeParams>
{
public:
    explicit SemanticTokensRangeRequest(const SemanticTokensRangeParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/semanticTokens/range";
};

class LANGUAGESERVERPROTOCOL_EXPORT SemanticTokensRefreshRequest
    : public Request<std::nullptr_t, std::nullptr_t, std::nullptr_t>
{
public:
    explicit SemanticTokensRefreshRequest();
    using Request::Request;
    constexpr static const char methodName[] = "workspace/semanticTokens/refresh";
};

} // namespace LanguageServerProtocol
