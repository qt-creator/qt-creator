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

#include <clangqueryexamplehighlightmarker.h>

namespace {
using testing::AllOf;
using testing::Contains;
using testing::IsEmpty;
using testing::Not;
using testing::InSequence;
using testing::_;

using SourceRange = ClangBackEnd::V2::SourceRangeContainer;
using SourceRanges = ClangBackEnd::SourceRangeWithTextContainers;
using Marker = ClangRefactoring::ClangQueryExampleHighlightMarker<MockSyntaxHighlighter>;
using TextFormats = std::array<QTextCharFormat, 5>;

class ClangQueryExampleHighlightMarker : public testing::Test
{
protected:
    void SetUp() override;

protected:
    TextFormats textFormats = {};
    MockSyntaxHighlighter highlighter;
    Marker marker{highlighter};

};

TEST_F(ClangQueryExampleHighlightMarker, NoCallForNotSourceRanges)
{
    SourceRanges sourceRanges;
    Marker marker(std::move(sourceRanges), highlighter, textFormats);

    EXPECT_CALL(highlighter, setFormat(_, _, _)).Times(0);

    marker.highlightBlock(1, "foo");
}

TEST_F(ClangQueryExampleHighlightMarker, SingleLineSourceRange)
{
    SourceRanges sourceRanges{{{1, 1}, 1, 3, 3, 1, 10, 10, "function"}};
    Marker marker(std::move(sourceRanges), highlighter, textFormats);

    EXPECT_CALL(highlighter, setFormat(2, 7, textFormats[0]));

    marker.highlightBlock(1, "   function");
}

TEST_F(ClangQueryExampleHighlightMarker, OtherSingleLineSourceRange)
{
    SourceRanges sourceRanges{{{1, 1}, 2, 5, 5, 2, 11, 11, "function"}};
    Marker marker(std::move(sourceRanges), highlighter, textFormats);
    marker.highlightBlock(1, "foo");

    EXPECT_CALL(highlighter, setFormat(4, 6, textFormats[0]));

    marker.highlightBlock(2, "void function");
}

TEST_F(ClangQueryExampleHighlightMarker, CascadedSingleLineSourceRanges)
{
    InSequence sequence;
    SourceRanges sourceRanges{{{1, 1}, 1, 2, 2, 1, 15, 15, "void function"},
                              {{1, 1}, 1, 2, 2, 1, 6, 6, "void"},
                              {{1, 1}, 1, 7, 7, 1, 15, 15, "function"}};
    Marker marker(std::move(sourceRanges), highlighter, textFormats);

    EXPECT_CALL(highlighter, setFormat(1, 13, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(1, 4, textFormats[1]));
    EXPECT_CALL(highlighter, setFormat(6, 8, textFormats[1]));

    marker.highlightBlock(1, "void function");
}

TEST_F(ClangQueryExampleHighlightMarker, DualLineSourceRanges)
{
    InSequence sequence;
    SourceRanges sourceRanges{{{1, 1}, 1, 2, 2, 2, 4, 20, "void f()\n {}"}};
    Marker marker(std::move(sourceRanges), highlighter, textFormats);

    EXPECT_CALL(highlighter, setFormat(1, 7, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(0, 3, textFormats[0]));

    marker.highlightBlock(1, "void f()");
    marker.highlightBlock(2, " {};");
}

TEST_F(ClangQueryExampleHighlightMarker, MultipleLineSourceRanges)
{
    InSequence sequence;
    SourceRanges sourceRanges{{{1, 1}, 1, 2, 2, 3, 3, 20, "void f()\n {\n }"}};
    Marker marker(std::move(sourceRanges), highlighter, textFormats);

    EXPECT_CALL(highlighter, setFormat(1, 7, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(0, 2, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(0, 2, textFormats[0]));

    marker.highlightBlock(1, "void f()");
    marker.highlightBlock(2, " {");
    marker.highlightBlock(3, " };");
}

TEST_F(ClangQueryExampleHighlightMarker, MoreMultipleLineSourceRanges)
{
    InSequence sequence;
    SourceRanges sourceRanges{{{1, 1}, 1, 1, 0, 4, 2, 0, ""},
                              {{1, 1}, 2, 2, 0, 2, 7, 0, ""},
                              {{1, 1}, 3, 2, 0, 3, 7, 0, ""}};
    Marker marker(std::move(sourceRanges), highlighter, textFormats);

    EXPECT_CALL(highlighter, setFormat(0, 10, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(0, 7, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(1, 5, textFormats[1]));
    EXPECT_CALL(highlighter, setFormat(0, 7, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(1, 5, textFormats[1]));
    EXPECT_CALL(highlighter, setFormat(0, 1, textFormats[0]));

    marker.highlightBlock(1, "void f() {");
    marker.highlightBlock(2, " int x;");
    marker.highlightBlock(3, " int y;");
    marker.highlightBlock(4, "};");
}

TEST_F(ClangQueryExampleHighlightMarker, CascadedMultipleLineSourceRanges)
{
    InSequence sequence;
    SourceRanges sourceRanges{{{1, 1}, 1, 1, 0, 4, 2, 0, ""},
                              {{1, 1}, 2, 2, 0, 3, 4, 0, ""},
                              {{1, 1}, 2, 11, 0, 2, 16, 0, ""}};
    Marker marker(std::move(sourceRanges), highlighter, textFormats);

    EXPECT_CALL(highlighter, setFormat(0, 9, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(0, 16, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(1, 15, textFormats[1]));
    EXPECT_CALL(highlighter, setFormat(10, 5, textFormats[2]));
    EXPECT_CALL(highlighter, setFormat(0, 3, textFormats[0]));
    EXPECT_CALL(highlighter, setFormat(0, 3, textFormats[1]));
    EXPECT_CALL(highlighter, setFormat(0, 1, textFormats[0]));

    marker.highlightBlock(1, "class X {");
    marker.highlightBlock(2, " int f() { int y");
    marker.highlightBlock(3, "  }");
    marker.highlightBlock(4, "};");
}

TEST_F(ClangQueryExampleHighlightMarker, FormatSingle)
{
    SourceRange sourceRange{{1, 1}, 1, 3, 3, 1, 10, 10};

    EXPECT_CALL(highlighter, setFormat(2, 7, textFormats[0]));

    marker.formatSourceRange(sourceRange, 1, 20, 0);
}

TEST_F(ClangQueryExampleHighlightMarker, FormatMultipleStart)
{
    SourceRange sourceRange{{1, 1}, 1, 3, 3, 2, 9, 20};

    EXPECT_CALL(highlighter, setFormat(2, 8, textFormats[0]));

    marker.formatSourceRange(sourceRange, 1, 10, 0);
}

TEST_F(ClangQueryExampleHighlightMarker, FormatMultipleEnd)
{
    SourceRange sourceRange{{1, 1}, 1, 3, 3, 2, 8, 20};

    EXPECT_CALL(highlighter, setFormat(0, 7, textFormats[1]));

    marker.formatSourceRange(sourceRange, 2, 10, 1);
}

TEST_F(ClangQueryExampleHighlightMarker, FormatMultipleMiddle)
{
    SourceRange sourceRange{{1, 1}, 1, 3, 3, 3, 8, 20};

    EXPECT_CALL(highlighter, setFormat(0, 10, textFormats[2]));

    marker.formatSourceRange(sourceRange, 2, 10, 2);
}

void ClangQueryExampleHighlightMarker::SetUp()
{
    textFormats[0].setFontItalic(true);
    textFormats[1].setFontCapitalization(QFont::Capitalize);
    textFormats[2].setFontItalic(true);
    textFormats[2].setFontCapitalization(QFont::Capitalize);
    textFormats[3].setFontOverline(true);
    textFormats[4].setFontCapitalization(QFont::Capitalize);
    textFormats[4].setFontOverline(true);

    marker.setTextFormats(TextFormats(textFormats));
}

}
