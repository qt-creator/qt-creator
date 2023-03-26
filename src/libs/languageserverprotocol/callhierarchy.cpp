// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "callhierarchy.h"

namespace LanguageServerProtocol {

bool CallHierarchyItem::isValid() const
{
    return contains(nameKey) && contains(symbolKindKey) && contains(rangeKey) && contains(uriKey)
           && contains(selectionRangeKey);
}

PrepareCallHierarchyRequest::PrepareCallHierarchyRequest(const TextDocumentPositionParams &params)
    : Request(methodName, params)
{}

CallHierarchyIncomingCallsRequest::CallHierarchyIncomingCallsRequest(
    const CallHierarchyCallsParams &params)
    : Request(methodName, params)
{}

CallHierarchyOutgoingCallsRequest::CallHierarchyOutgoingCallsRequest(
    const CallHierarchyCallsParams &params)
    : Request(methodName, params)
{}

} // namespace LanguageServerProtocol
