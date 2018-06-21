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

#include "googletest.h"

#include <clangstring.h>

#include <utf8string.h>

#include <clang-c/CXString.h>
#include <clang-c/Index.h>

namespace {

using ::testing::StrEq;
using ::testing::Eq;

using ClangBackEnd::ClangString;

TEST(ClangString, ConvertToUtf8String)
{
    const CXString cxString = { "text", 0};

    ASSERT_THAT(Utf8String(ClangString(cxString)), Utf8StringLiteral("text"));
}

TEST(ClangString, ConvertNullStringToUtf8String)
{
    const CXString cxString = { 0, 0};

    ASSERT_THAT(Utf8String(ClangString(cxString)), Utf8String());
}

TEST(ClangString, MoveContructor)
{
    ClangString text(CXString{ "text", 0});

    const ClangString text2 = std::move(text);

    ASSERT_TRUE(text.isNull());
    ASSERT_FALSE(text2.isNull());
}

TEST(ClangString, MoveAssigment)
{
    ClangString text(CXString{ "text", 0});

    ClangString text2 = std::move(text);
    text = std::move(text2);

    ASSERT_TRUE(text2.isNull());
    ASSERT_FALSE(text.isNull());
}

TEST(ClangString, MoveSelfAssigment)
{
    ClangString text(CXString{ "text", 0});

    text = std::move(text);

    ASSERT_FALSE(text.isNull());
}

TEST(ClangString, SpellingAsCString)
{
    ClangString text(CXString{"text", 0});

    ASSERT_THAT(text.cString(), StrEq("text"));
}

TEST(ClangString, EqualBetweenClangStrings)
{
    ClangString text(CXString{"text", 0});
    ClangString text2(CXString{"text", 0});

    bool textIsEqual = text == text2;

    ASSERT_TRUE(textIsEqual);
}

TEST(ClangString, NotEqualBetweenClangStrings)
{
    ClangString text(CXString{"text", 0});
    ClangString text2(CXString{"text ", 0});

    bool textIsNotEqual = text != text2;

    ASSERT_TRUE(textIsNotEqual);
}

TEST(ClangString, EqualClangStringAndCString)
{
    ClangString text(CXString{"text", 0});

    bool textIsEqual = text == "text";

    ASSERT_TRUE(textIsEqual);
}

TEST(ClangString, NotEqualClangStringAndCString)
{
    ClangString text(CXString{"text", 0});

    bool textIsNotEqual = text != "text ";

    ASSERT_TRUE(textIsNotEqual);
}

TEST(ClangString, EqualCStringAndClangString)
{
    ClangString text(CXString{"text", 0});

    bool textIsEqual = "text" == text;

    ASSERT_TRUE(textIsEqual);
}

TEST(ClangString, NotEqualCStringAndClangString)
{
    ClangString text(CXString{"text", 0});

    bool textIsNotEqual = "text " != text;

    ASSERT_TRUE(textIsNotEqual);
}

TEST(ClangString, EqualClangStringPointerAndCString)
{
    ClangString text(CXString{"text", 0});
    const char *cString = "text";

    bool textIsEqual = cString == text;

    ASSERT_TRUE(textIsEqual);
}

TEST(ClangString, NotEqualClangStringPointerAndCString)
{
    ClangString text(CXString{"text", 0});
    const char *cString = "text ";

    bool textIsNotEqual = cString != text;

    ASSERT_TRUE(textIsNotEqual);
}

TEST(ClangString, EqualCStringAndClangStringPointer)
{
    ClangString text(CXString{"text", 0});
    const char *cString = "text";

    bool textIsEqual = text == cString;

    ASSERT_TRUE(textIsEqual);
}

TEST(ClangString, NotEqualCStringAndClangStringPointer)
{
    ClangString text(CXString{"text", 0});
    const char *cString = "text ";

    bool textIsNotEqual = text != cString;

    ASSERT_TRUE(textIsNotEqual);
}

TEST(ClangString, NullStringHasNoContent)
{
    ClangString text(CXString{nullptr, 0});

    bool hasContent = text.hasContent();

    ASSERT_FALSE(hasContent);
}

TEST(ClangString, EmptyStringHasNoContent)
{
    ClangString text(CXString{"", 0});

    bool hasContent = text.hasContent();

    ASSERT_FALSE(hasContent);
}

TEST(ClangString, StringHasNoContent)
{
    ClangString text(CXString{"text", 0});

    bool hasContent = text.hasContent();

    ASSERT_TRUE(hasContent);
}

}
