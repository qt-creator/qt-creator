// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/jsonkeys.h>
#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/lsptypes.h>

namespace Copilot {

class Completion : public LanguageServerProtocol::JsonObject
{
    static constexpr char displayTextKey[] = "displayText";
    static constexpr char uuidKey[] = "uuid";

public:
    using JsonObject::JsonObject;

    QString displayText() const { return typedValue<QString>(displayTextKey); }
    LanguageServerProtocol::Position position() const
    {
        return typedValue<LanguageServerProtocol::Position>(LanguageServerProtocol::positionKey);
    }
    LanguageServerProtocol::Range range() const
    {
        return typedValue<LanguageServerProtocol::Range>(LanguageServerProtocol::rangeKey);
    }
    QString text() const { return typedValue<QString>(LanguageServerProtocol::textKey); }
    void setText(const QString &text) { insert(LanguageServerProtocol::textKey, text); }
    QString uuid() const { return typedValue<QString>(uuidKey); }

    bool isValid() const override
    {
        return contains(LanguageServerProtocol::textKey)
               && contains(LanguageServerProtocol::rangeKey)
               && contains(LanguageServerProtocol::positionKey);
    }
};

class GetCompletionParams : public LanguageServerProtocol::JsonObject
{
public:
    static constexpr char docKey[] = "doc";

    GetCompletionParams(const LanguageServerProtocol::TextDocumentIdentifier &document,
                        int version,
                        const LanguageServerProtocol::Position &position)
    {
        setTextDocument(document);
        setVersion(version);
        setPosition(position);
    }
    using JsonObject::JsonObject;

    // The text document.
    LanguageServerProtocol::TextDocumentIdentifier textDocument() const
    {
        return typedValue<LanguageServerProtocol::TextDocumentIdentifier>(docKey);
    }
    void setTextDocument(const LanguageServerProtocol::TextDocumentIdentifier &id)
    {
        insert(docKey, id);
    }

    // The position inside the text document.
    LanguageServerProtocol::Position position() const
    {
        return LanguageServerProtocol::fromJsonValue<LanguageServerProtocol::Position>(
            value(docKey).toObject().value(LanguageServerProtocol::positionKey));
    }
    void setPosition(const LanguageServerProtocol::Position &position)
    {
        QJsonObject result = value(docKey).toObject();
        result[LanguageServerProtocol::positionKey] = (QJsonObject) position;
        insert(docKey, result);
    }

    // The version
    int version() const { return typedValue<int>(LanguageServerProtocol::versionKey); }
    void setVersion(int version)
    {
        QJsonObject result = value(docKey).toObject();
        result[LanguageServerProtocol::versionKey] = version;
        insert(docKey, result);
    }

    bool isValid() const override
    {
        return contains(docKey)
               && value(docKey).toObject().contains(LanguageServerProtocol::positionKey)
               && value(docKey).toObject().contains(LanguageServerProtocol::versionKey);
    }
};

class GetCompletionResponse : public LanguageServerProtocol::JsonObject
{
    static constexpr char completionKey[] = "completions";

public:
    using JsonObject::JsonObject;

    LanguageServerProtocol::LanguageClientArray<Completion> completions() const
    {
        return clientArray<Completion>(completionKey);
    }
};

class GetCompletionRequest
    : public LanguageServerProtocol::Request<GetCompletionResponse, std::nullptr_t, GetCompletionParams>
{
public:
    explicit GetCompletionRequest(const GetCompletionParams &params = {})
        : Request(methodName, params)
    {}
    using Request::Request;
    constexpr static const char methodName[] = "getCompletionsCycling";
};

} // namespace Copilot
