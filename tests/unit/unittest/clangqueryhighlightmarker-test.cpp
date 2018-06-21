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
using testing::ElementsAre;
using testing::Pointwise;
using testing::IsEmpty;
using testing::Not;
using testing::InSequence;
using testing::_;

using SourceRange = ClangBackEnd::V2::SourceRangeContainer;
using Message = ClangBackEnd::DynamicASTMatcherDiagnosticMessageContainer;
using Context = ClangBackEnd::DynamicASTMatcherDiagnosticContextContainer;
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
    Messages messages{{{{0, 1}, 1, 5, 0, 1, 10, 0}, ErrorType::RegistryMatcherNotFound, {}},
                      {{{0, 1}, 1, 30, 0, 1, 40, 0}, ErrorType::RegistryMatcherNotFound, {}}};
    Contexts contexts{{{{0, 1}, 1, 2, 0, 1, 15, 0}, ContextType::MatcherArg, {}},
                      {{{0, 1}, 1, 20, 0, 1, 50, 0}, ContextType::MatcherArg, {}}};
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
    Messages messages{{{{0, 1}, 1, 5, 0, 3, 3, 0}, ErrorType::RegistryMatcherNotFound, {}}};
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
    Messages messages{{{{1, 1}, 1, 5, 0, 3, 3, 0}, ErrorType::RegistryMatcherNotFound, {}}};
    Contexts contexts{{{{1, 1}, 1, 2, 0, 3, 4, 0}, ContextType::MatcherArg, {}}};
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

TEST_F(ClangQueryHighlightMarker, NoMessagesIfEmpty)
{
    Messages messages;
    Contexts contexts;
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundMessages = marker.messagesForLineAndColumn(1, 1);

    ASSERT_THAT(foundMessages, IsEmpty());
}

TEST_F(ClangQueryHighlightMarker, NoMessagesForBeforePosition)
{
    Messages messages{{{{0, 1}, 1, 5, 0, 3, 3, 0},
            ErrorType::RegistryMatcherNotFound,
            {"foo"}}};
    Contexts contexts;
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundMessages = marker.messagesForLineAndColumn(1, 4);

    ASSERT_THAT(foundMessages, IsEmpty());
}

TEST_F(ClangQueryHighlightMarker, NoMessagesForAfterPosition)
{
    Messages messages{{{{0, 1}, 1, 5, 0, 3, 3, 0},
            ErrorType::RegistryMatcherNotFound,
            {"foo"}}};
    Contexts contexts;
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundMessages = marker.messagesForLineAndColumn(3, 4);

    ASSERT_THAT(foundMessages, IsEmpty());
}

TEST_F(ClangQueryHighlightMarker, OneMessagesForInsidePosition)
{
    Message message{{{0, 1}, 1, 5, 0, 3, 3, 0},
                    ErrorType::RegistryMatcherNotFound,
                    {"foo"}};
    Messages messages{message.clone()};
    Contexts contexts;
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundMessages = marker.messagesForLineAndColumn(2, 3);

    ASSERT_THAT(foundMessages, ElementsAre(message));
}

TEST_F(ClangQueryHighlightMarker, NoMessagesForOutsidePosition)
{
    Message message{{{0, 1}, 1, 5, 0, 3, 3, 0},
                    ErrorType::RegistryMatcherNotFound,
                    {"foo"}};
    Messages messages{message.clone()};
    Contexts contexts;
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundMessages = marker.messagesForLineAndColumn(3, 4);

    ASSERT_THAT(foundMessages, IsEmpty());
}

TEST_F(ClangQueryHighlightMarker, AfterStartColumnBeforeLine)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isAfterStartColumn = marker.isInsideRange(sourceRange, 1, 6);

    ASSERT_FALSE(isAfterStartColumn);
}

TEST_F(ClangQueryHighlightMarker, AfterStartColumnBeforeColumn)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isAfterStartColumn = marker.isInsideRange(sourceRange, 2, 4);

    ASSERT_FALSE(isAfterStartColumn);
}

TEST_F(ClangQueryHighlightMarker, AfterStartColumnAtColumn)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isAfterStartColumn = marker.isInsideRange(sourceRange, 2, 5);

    ASSERT_TRUE(isAfterStartColumn);
}

TEST_F(ClangQueryHighlightMarker, AfterStartColumnAfterColumn)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isAfterStartColumn = marker.isInsideRange(sourceRange, 2, 6);

    ASSERT_TRUE(isAfterStartColumn);
}

TEST_F(ClangQueryHighlightMarker, BeforeEndColumnAfterLine)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isBeforeEndColumn = marker.isInsideRange(sourceRange, 4, 1);

    ASSERT_FALSE(isBeforeEndColumn);
}

TEST_F(ClangQueryHighlightMarker, BeforeEndColumnAfterColumn)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isBeforeEndColumn = marker.isInsideRange(sourceRange, 3, 4);

    ASSERT_FALSE(isBeforeEndColumn);
}

TEST_F(ClangQueryHighlightMarker, BeforeEndColumnAtColumn)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isBeforeEndColumn = marker.isInsideRange(sourceRange, 3, 3);

    ASSERT_TRUE(isBeforeEndColumn);
}

TEST_F(ClangQueryHighlightMarker, BeforeEndColumnBeforeColumn)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isBeforeEndColumn = marker.isInsideRange(sourceRange, 3, 2);

    ASSERT_TRUE(isBeforeEndColumn);
}

TEST_F(ClangQueryHighlightMarker, InBetweenLineBeforeLine)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 3, 3, 0};

    bool isInBetween = marker.isInsideRange(sourceRange, 1, 6);

    ASSERT_FALSE(isInBetween);
}

TEST_F(ClangQueryHighlightMarker, InBetweenLineAfterLine)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 4, 3, 0};

    bool isInBetween = marker.isInsideRange(sourceRange, 5, 1);

    ASSERT_FALSE(isInBetween);
}

TEST_F(ClangQueryHighlightMarker, InBetweenLine)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 4, 3, 0};

    bool isInBetween = marker.isInsideRange(sourceRange, 3, 1);

    ASSERT_TRUE(isInBetween);
}

TEST_F(ClangQueryHighlightMarker, SingleLineBefore)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 2, 10, 0};

    bool isInRange = marker.isInsideRange(sourceRange, 2, 4);

    ASSERT_FALSE(isInRange);
}

TEST_F(ClangQueryHighlightMarker, SingleLineAfter)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 2, 10, 0};

    bool isInRange = marker.isInsideRange(sourceRange, 2, 11);

    ASSERT_FALSE(isInRange);
}

TEST_F(ClangQueryHighlightMarker, SingleLineInRange)
{
    SourceRange sourceRange{{0, 1}, 2, 5, 0, 2, 10, 0};

    bool isInRange = marker.isInsideRange(sourceRange, 2, 6);

    ASSERT_TRUE(isInRange);
}

TEST_F(ClangQueryHighlightMarker, NoContextsIfEmpty)
{
    Messages messages;
    Contexts contexts;
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundContexts = marker.contextsForLineAndColumn(1, 1);

    ASSERT_THAT(foundContexts, IsEmpty());
}

TEST_F(ClangQueryHighlightMarker, NoContextsForBeforePosition)
{
    Messages messages;
    Contexts contexts{{{{0, 1}, 1, 5, 0, 3, 3, 0},
                      ContextType::MatcherArg,
                      {"foo"}}};
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundContexts = marker.contextsForLineAndColumn(1, 4);

    ASSERT_THAT(foundContexts, IsEmpty());
}

TEST_F(ClangQueryHighlightMarker, NoContextsForAfterPosition)
{
    Messages messages;
    Contexts contexts{{{{0, 1}, 1, 5, 0, 3, 3, 0},
                      ContextType::MatcherArg,
                      {"foo"}}};
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundContexts = marker.contextsForLineAndColumn(3, 4);

    ASSERT_THAT(foundContexts, IsEmpty());
}

TEST_F(ClangQueryHighlightMarker, OneContextsForInsidePosition)
{
    Context context{{{0, 1}, 1, 5, 0, 3, 3, 0},
                    ContextType::MatcherArg,
                    {"foo"}};
    Messages messages;
    Contexts contexts{context.clone()};
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundContexts = marker.contextsForLineAndColumn(2, 3);

    ASSERT_THAT(foundContexts, ElementsAre(context));
}

TEST_F(ClangQueryHighlightMarker, NoContextsForOutsidePosition)
{
    Context context{{{0, 1}, 1, 5, 0, 3, 3, 0},
                    ContextType::MatcherArg,
                    {"foo"}};
    Messages messages;
    Contexts contexts{context.clone()};
    marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    auto foundContexts = marker.contextsForLineAndColumn(3, 4);

    ASSERT_THAT(foundContexts, IsEmpty());
}

void ClangQueryHighlightMarker::SetUp()
{
    messageTextFormat.setFontItalic(true);
    contextTextFormat.setFontCapitalization(QFont::Capitalize);

    marker.setTextFormats(messageTextFormat, contextTextFormat);
}

}
