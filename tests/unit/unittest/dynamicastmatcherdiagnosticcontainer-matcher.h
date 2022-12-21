// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "googletest.h"

using testing::PrintToString;

namespace {

MATCHER_P5(HasDiagnosticMessage, errorTypeText, startLine, startColumn, endLine, endColumn,
           std::string(negation ? "isn't " : "is ")
           + "{" + PrintToString(errorTypeText)
           + ": (" + PrintToString(startLine)
           + ", " + PrintToString(startColumn)
           + "), (" + PrintToString(endLine)
           + ", " + PrintToString(endColumn)
           + ")}"
           )
{
    if (!arg.empty() && arg.front().messages.empty()) {
        *result_listener << "no messages";
        return  false;
    }

    auto message = arg.front().messages.front();
    auto sourceRange = message.sourceRange;

    return message.errorTypeText() == errorTypeText
        && sourceRange.start.line == uint(startLine)
        && sourceRange.start.column == uint(startColumn)
        && sourceRange.end.line == uint(endLine)
        && sourceRange.end.column == uint(endColumn);
}

MATCHER_P5(HasDiagnosticContext, contextTypeText, startLine, startColumn, endLine, endColumn,
           std::string(negation ? "isn't " : "is ")
            + "{" + PrintToString(contextTypeText)
           + ": (" + PrintToString(startLine)
           + ", " + PrintToString(startColumn)
           + "), (" + PrintToString(endLine)
           + ", " + PrintToString(endColumn)
           + ")}"
           )
{
    if (!arg.empty() && arg.front().messages.empty()) {
        *result_listener << "no context";
        return  false;
    }

    auto context = arg.front().contexts.front();
    auto sourceRange = context.sourceRange;

    return context.contextTypeText() == contextTypeText
        && sourceRange.start.line == uint(startLine)
        && sourceRange.start.column == uint(startColumn)
        && sourceRange.end.line == uint(endLine)
        && sourceRange.end.column == uint(endColumn);
}

}
