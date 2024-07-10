// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "typehierarchy.h"

namespace LanguageServerProtocol {

bool TypeHierarchyItem::isValid() const
{
    return contains(nameKey) && contains(kindKey) && contains(uriKey) && contains(rangeKey)
           && contains(selectionRangeKey);
}
std::optional<QList<SymbolTag>> TypeHierarchyItem::symbolTags() const
{
    return Internal::getSymbolTags(*this);
}

PrepareTypeHierarchyRequest::PrepareTypeHierarchyRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{}
TypeHierarchySupertypesRequest::TypeHierarchySupertypesRequest(const TypeHierarchyParams &params)
    : Request(methodName, params)
{}
TypeHierarchySubtypesRequest::TypeHierarchySubtypesRequest(const TypeHierarchyParams &params)
    : Request(methodName, params)
{}

} // namespace LanguageServerProtocol
