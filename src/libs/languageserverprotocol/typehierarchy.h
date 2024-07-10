// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT TypeHierarchyItem : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    SymbolKind symbolKind() const { return SymbolKind(typedValue<int>(kindKey)); }
    void setSymbolKind(const SymbolKind &symbolKind) { insert(kindKey, int(symbolKind)); }

    std::optional<QList<SymbolTag>> symbolTags() const;

    std::optional<QString> detail() const { return optionalValue<QString>(detailKey); }
    void setDetail(const QString &detail) { insert(detailKey, detail); }
    void clearDetail() { remove(detailKey); }

    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    Range selectionRange() const { return typedValue<Range>(selectionRangeKey); }
    void setSelectionRange(Range selectionRange) { insert(selectionRangeKey, selectionRange); }

    /*
     * A data entry field that is preserved between a type hierarchy prepare and
     * supertypes or subtypes requests. It could also be used to identify the
     * type hierarchy in the server, helping improve the performance on
     * resolving supertypes and subtypes.
     */
    std::optional<QJsonValue> data() const { return optionalValue<QJsonValue>(dataKey); }

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT PrepareTypeHierarchyRequest : public Request<
        LanguageClientArray<TypeHierarchyItem>, std::nullptr_t, TextDocumentPositionParams>
{
public:
    explicit PrepareTypeHierarchyRequest(const TextDocumentPositionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/prepareTypeHierarchy";
};

class LANGUAGESERVERPROTOCOL_EXPORT TypeHierarchyParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    TypeHierarchyItem item() const { return typedValue<TypeHierarchyItem>(itemKey); }
    void setItem(const TypeHierarchyItem &item) { insert(itemKey, item); }

    bool isValid() const override { return contains(itemKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TypeHierarchySupertypesRequest : public Request<
        LanguageClientArray<TypeHierarchyItem>, std::nullptr_t, TypeHierarchyParams>
{
public:
    explicit TypeHierarchySupertypesRequest(const TypeHierarchyParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "typeHierarchy/supertypes";
};

class LANGUAGESERVERPROTOCOL_EXPORT TypeHierarchySubtypesRequest
    : public Request<LanguageClientArray<TypeHierarchyItem>, std::nullptr_t, TypeHierarchyParams>
{
public:
    explicit TypeHierarchySubtypesRequest(const TypeHierarchyParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "typeHierarchy/subtypes";
};

} // namespace LanguageServerProtocol
