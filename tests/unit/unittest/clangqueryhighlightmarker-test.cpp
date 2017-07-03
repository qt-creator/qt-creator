/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "googletest.h"
#include "filesystem-utilities.h"
#include "mocksyntaxhighligher.h"

#include <clangqueryhighlightmarker.h>

namespace {
using testing::AllOf;
using testing::Contains;
using testing::IsEmpty;
using testing::Not;
using testing::InSequence;
using testing::_;

using SourceRange = ClangBackEnd::V2::SourceRangeContainer;
using Messages = ClangBackEnd::DynamicASTMatcherDiagnosticMessageContainers;
using Contexts = ClangBackEnd::DynamicASTMatcherDiagnosticContextContainers;
using Marker = ClangRefactoring::ClangQueryHighlightMarker<MockSyntaxHighlighter>;
using ErrorType = ClangBackEnd::ClangQueryDiagnosticErrorType;
using ContextType = ClangBackEnd::ClangQueryDiagnosticContextType;

class ClangQueryHighlightMarker : public testing::Test
{
protected:
    void SetUp() override;

protected:
    QTextCharFormat messageTextFormat;
    QTextCharFormat contextTextFormat;
    MockSyntaxHighlighter highlighter;
    Marker marker{highlighter};

};

TEST_F(ClangQueryHighlightMarker, NoCallForNoMessagesAndContexts)
{
    Messages messages;
    Contexts contexts;
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    EXPECT_CALL(highlighter, setFormat(_, _, _)).Times(0);

    marker.highlightBlock(1, "foo");
}

TEST_F(ClangQueryHighlightMarker, CallForMessagesAndContextsForASingleLine)
{
    InSequence sequence;
    Messages messages{{{1, 1, 5, 0, 1, 10, 0}, ErrorType::RegistryMatcherNotFound, {}},
                      {{1, 1, 30, 0, 1, 40, 0}, ErrorType::RegistryMatcherNotFound, {}}};
    Contexts contexts{{{1, 1, 2, 0, 1, 15, 0}, ContextType::MatcherArg, {}},
                      {{1, 1, 20, 0, 1, 50, 0}, ContextType::MatcherArg, {}}};
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    EXPECT_CALL(highlighter, setFormat(1, 13, contextTextFormat));
    EXPECT_CALL(highlighter, setFormat(4, 5, messageTextFormat));
    EXPECT_CALL(highlighter, setFormat(19, 30, contextTextFormat));
    EXPECT_CALL(highlighter, setFormat(29, 10, messageTextFormat));

    marker.highlightBlock(1, "foo");
}

TEST_F(ClangQueryHighlightMarker, CallForMessagesForAMultiLine)
{
    InSequence sequence;
    Messages messages{{{1, 1, 5, 0, 3, 3, 0}, ErrorType::RegistryMatcherNotFound, {}}};
    Contexts contexts;
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    EXPECT_CALL(highlighter, setFormat(4, 8, messageTextFormat));
    EXPECT_CALL(highlighter, setFormat(0, 8, messageTextFormat));
    EXPECT_CALL(highlighter, setFormat(0, 2, messageTextFormat));

    marker.highlightBlock(1, "declFunction(");
    marker.highlightBlock(2, "  decl()");
    marker.highlightBlock(3, "  )");
}

TEST_F(ClangQueryHighlightMarker, CallForMessagesAndContextForAMultiLine)
{
    InSequence sequence;
    Messages messages{{{1, 1, 5, 0, 3, 3, 0}, ErrorType::RegistryMatcherNotFound, {}}};
    Contexts contexts{{{1, 1, 2, 0, 3, 4, 0}, ContextType::MatcherArg, {}}};
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    EXPECT_CALL(highlighter, setFormat(1, 11, contextTextFormat));
    EXPECT_CALL(highlighter, setFormat(4, 8, messageTextFormat));
    EXPECT_CALL(highlighter, setFormat(0, 8, contextTextFormat));
    EXPECT_CALL(highlighter, setFormat(0, 8, messageTextFormat));
    EXPECT_CALL(highlighter, setFormat(0, 3, contextTextFormat));
    EXPECT_CALL(highlighter, setFormat(0, 2, messageTextFormat));

    marker.highlightBlock(1, "declFunction(");
    marker.highlightBlock(2, "  decl()");
    marker.highlightBlock(3, "  )  ");
}

void ClangQueryHighlightMarker::SetUp()
{
    messageTextFormat.setFontItalic(true);
    contextTextFormat.setFontCapitalization(QFont::Capitalize);

    marker.setTextFormats(messageTextFormat, contextTextFormat);
}

}
