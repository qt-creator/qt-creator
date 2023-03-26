// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT CallHierarchyItem : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    SymbolKind symbolKind() const { return SymbolKind(typedValue<int>(symbolKindKey)); }
    void setSymbolKind(const SymbolKind &symbolKind) { insert(symbolKindKey, int(symbolKind)); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    Range selectionRange() const { return typedValue<Range>(selectionRangeKey); }
    void setSelectionRange(Range selectionRange) { insert(selectionRangeKey, selectionRange); }

    std::optional<QString> detail() const { return optionalValue<QString>(detailKey); }
    void setDetail(const QString &detail) { insert(detailKey, detail); }
    void clearDetail() { remove(detailKey); }

    std::optional<QList<DocumentSymbol>> children() const
    { return optionalArray<DocumentSymbol>(childrenKey); }
    void setChildren(const QList<DocumentSymbol> &children) { insertArray(childrenKey, children); }
    void clearChildren() { remove(childrenKey); }

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT PrepareCallHierarchyRequest : public Request<
        LanguageClientArray<CallHierarchyItem>, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit PrepareCallHierarchyRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/prepareCallHierarchy";
};

class LANGUAGESERVERPROTOCOL_EXPORT CallHierarchyCallsParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    CallHierarchyItem item() const { return typedValue<CallHierarchyItem>(itemKey); }
    void setItem(const CallHierarchyItem &item) { insert(itemKey, item); }

    bool isValid() const override { return contains(itemKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT CallHierarchyIncomingCall : public JsonObject
{
public:
    using JsonObject::JsonObject;

    CallHierarchyItem from() const { return typedValue<CallHierarchyItem>(fromKey); }
    void setFrom(const CallHierarchyItem &from) { insert(fromKey, from); }

    QList<Range> fromRanges() const { return array<Range>(fromRangesKey); }
    void setFromRanges(const QList<Range> &fromRanges) { insertArray(fromRangesKey, fromRanges); }

    bool isValid() const override { return contains(fromRangesKey) && contains(fromRangesKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT CallHierarchyIncomingCallsRequest : public Request<
        LanguageClientArray<CallHierarchyIncomingCall>, std::nullptr_t, CallHierarchyCallsParams>
{
public:
    explicit CallHierarchyIncomingCallsRequest(const CallHierarchyCallsParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "callHierarchy/incomingCalls";
};

class LANGUAGESERVERPROTOCOL_EXPORT CallHierarchyOutgoingCall : public JsonObject
{
public:
    using JsonObject::JsonObject;

    CallHierarchyItem to() const { return typedValue<CallHierarchyItem>(toKey); }
    void setTo(const CallHierarchyItem &to) { insert(toKey, to); }

    QList<Range> fromRanges() const { return array<Range>(fromRangesKey); }
    void setFromRanges(const QList<Range> &fromRanges) { insertArray(fromRangesKey, fromRanges); }

    bool isValid() const override { return contains(fromRangesKey) && contains(fromRangesKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT CallHierarchyOutgoingCallsRequest : public Request<
        LanguageClientArray<CallHierarchyOutgoingCall>, std::nullptr_t, CallHierarchyCallsParams>
{
public:
    explicit CallHierarchyOutgoingCallsRequest(const CallHierarchyCallsParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "callHierarchy/outgoingCalls";
};

} // namespace LanguageServerProtocol
