/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    if (!arg.empty() && arg.front().messages().empty()) {
        *result_listener << "no messages";
        return  false;
    }

    auto message = arg.front().messages().front();
    auto sourceRange = message.sourceRange();

    return message.errorTypeText() == errorTypeText
        && sourceRange.start().line() == uint(startLine)
        && sourceRange.start().column() == uint(startColumn)
        && sourceRange.end().line() == uint(endLine)
        && sourceRange.end().column() == uint(endColumn);
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
    if (!arg.empty() && arg.front().messages().empty()) {
        *result_listener << "no context";
        return  false;
    }

    auto context = arg.front().contexts().front();
    auto sourceRange = context.sourceRange();

    return context.contextTypeText() == contextTypeText
        && sourceRange.start().line() == uint(startLine)
        && sourceRange.start().column() == uint(startColumn)
        && sourceRange.end().line() == uint(endLine)
        && sourceRange.end().column() == uint(endColumn);
}

}
