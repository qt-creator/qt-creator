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

#include <QString>

#include <utils/smallstring.h>
#include <utils/smallstringio.h>
#include <utils/smallstringvector.h>

using namespace ::testing;

using Utils::PathString;
using Utils::SmallString;
using Utils::SmallStringLiteral;
using Utils::SmallStringView;

TEST(SmallString, BasicStringEqual)
{
    ASSERT_THAT(SmallString("text"), Eq(SmallString("text")));
}

TEST(SmallString, BasicSmallStringUnequal)
{
    ASSERT_THAT(SmallString("text"), Ne(SmallString("other text")));
}

TEST(SmallString, NullSmallStringIsEqualToEmptySmallString)
{
    ASSERT_THAT(SmallString(), Eq(SmallString("")));
}

TEST(SmallString, ShortSmallStringLiteralIsShortSmallString)
{
    constexpr SmallStringLiteral shortText("short string");

#if __cpp_constexpr >= 201304
    ASSERT_TRUE(shortText.isShortString());
#else
    ASSERT_TRUE(shortText.isReadOnlyReference());
#endif
}

TEST(SmallString, ShortSmallStringIsShortSmallString)
{
    SmallString shortText("short string");

#if __cpp_constexpr >= 201304
    ASSERT_TRUE(shortText.isShortString());
#else
    ASSERT_TRUE(shortText.isReadOnlyReference());
#endif
}

TEST(SmallString, CreateFromCStringIterators)
{
    char sourceText[] = "this is very very very very very much text";

    SmallString text(sourceText, &sourceText[sizeof(sourceText) - 1]);

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, CreateFromQByteArrayIterators)
{
    QByteArray sourceText = "this is very very very very very much text";

    SmallString text(sourceText.begin(), sourceText.end());

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, CreateFromSmallStringIterators)
{
    SmallString sourceText = "this is very very very very very much text";

    SmallString text(sourceText.begin(), sourceText.end());

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, CreateFromStringView)
{
    SmallStringView sourceText = "this is very very very very very much text";

    SmallString text(sourceText);

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, ShortSmallStringIsReference)
{
    SmallString longText("very very very very very long text");

    ASSERT_TRUE(longText.isReadOnlyReference());
}

TEST(SmallString, SmallStringContructorIsNotReference)
{
    const char *shortCSmallString = "short string";
    auto shortText = SmallString(shortCSmallString);

    ASSERT_TRUE(shortText.isShortString());
}

TEST(SmallString, ShortSmallStringIsNotReference)
{
    const char *shortCSmallString = "short string";
    auto shortText = SmallString::fromUtf8(shortCSmallString);

    ASSERT_FALSE(shortText.isReadOnlyReference());
}

TEST(SmallString, LongSmallStringConstrutorIsAllocated)
{
    const char *longCSmallString = "very very very very very long text";
    auto longText = SmallString(longCSmallString);

    ASSERT_TRUE(longText.hasAllocatedMemory());
}

TEST(SmallString, MaximumShortSmallString)
{
    SmallString maximumShortText("very very very very short text", 30);

    ASSERT_THAT(maximumShortText.constData(), StrEq("very very very very short text"));
}

TEST(SmallString, LongConstExpressionSmallStringIsReference)
{
    SmallString longText("very very very very very very very very very very very long string");

    ASSERT_TRUE(longText.isReadOnlyReference());
}

TEST(SmallString, CloneShortSmallString)
{
    SmallString shortText("short string");

    auto clonedText = shortText.clone();

    ASSERT_THAT(clonedText, Eq("short string"));
}

TEST(SmallString, CloneLongSmallString)
{
    SmallString longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto clonedText = longText.clone();

    ASSERT_THAT(clonedText, Eq("very very very very very very very very very very very long string"));
}

TEST(SmallString, ClonedLongSmallStringDataPointerIsDifferent)
{
    SmallString longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto clonedText = longText.clone();

    ASSERT_THAT(clonedText.data(), Ne(longText.data()));
}

TEST(SmallString, CopyShortConstExpressionSmallStringIsShortSmallString)
{
    SmallString shortText("short string");

    auto shortTextCopy = shortText;

#if __cpp_constexpr >= 201304
    ASSERT_TRUE(shortTextCopy.isShortString());
#else
    ASSERT_TRUE(shortTextCopy.isReadOnlyReference());
#endif
}

TEST(SmallString, CopyLongConstExpressionSmallStringIsLongSmallString)
{
    SmallString longText("very very very very very very very very very very very long string");

    auto longTextCopy = longText;

    ASSERT_FALSE(longTextCopy.isShortString());
}

TEST(SmallString, ShortPathStringIsShortString)
{
    const char *rawText = "very very very very very very very very very very very long path which fits in the short memory";

    PathString text(rawText);

    ASSERT_TRUE(text.isShortString());
}

TEST(SmallString, SmallStringFromCharacterArrayIsReference)
{
    const char longCString[] = "very very very very very very very very very very very long string";

    SmallString longString(longCString);

    ASSERT_TRUE(longString.isReadOnlyReference());
}

TEST(SmallString, SmallStringFromCharacterPointerIsNotReference)
{
    const char *longCString = "very very very very very very very very very very very long string";

    SmallString longString = SmallString::fromUtf8(longCString);

    ASSERT_FALSE(longString.isReadOnlyReference());
}

TEST(SmallString, CopyStringFromReference)
{
    SmallString longText("very very very very very very very very very very very long string");
    SmallString longTextCopy;

    longTextCopy = longText;

    ASSERT_TRUE(longTextCopy.isReadOnlyReference());
}

TEST(SmallString, SmallStringLiteralShortSmallStringDataAccess)
{
    SmallStringLiteral literalText("very very very very very very very very very very very long string");

    ASSERT_THAT(literalText.data(), StrEq("very very very very very very very very very very very long string"));
}

TEST(SmallString, SmallStringLiteralLongSmallStringDataAccess)
{
    SmallStringLiteral literalText("short string");

    ASSERT_THAT(literalText.data(), StrEq("short string"));
}

TEST(SmallString, ReferenceDataAccess)
{
    SmallString literalText("short string");

    ASSERT_THAT(literalText.constData(), StrEq("short string"));
}

TEST(SmallString, ShortDataAccess)
{
    const char *shortCString = "short string";
    auto shortText = SmallString::fromUtf8(shortCString);

    ASSERT_THAT(shortText.constData(), StrEq("short string"));
}

TEST(SmallString, LongDataAccess)
{
    const char *longCString = "very very very very very very very very very very very long string";
    auto longText = SmallString::fromUtf8(longCString);

    ASSERT_THAT(longText.constData(), StrEq(longCString));
}

TEST(SmallString, LongSmallStringHasShortSmallStringSizeZero)
{
    auto longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    ASSERT_THAT(longText.shortStringSize(), 0);
}

TEST(SmallString, SmallStringBeginIsEqualEndForEmptySmallString)
{
    SmallString text;

    ASSERT_THAT(text.begin(), Eq(text.end()));
}

TEST(SmallString, SmallStringBeginIsNotEqualEndForNonEmptySmallString)
{
    SmallString text("x");

    ASSERT_THAT(text.begin(), Ne(text.end()));
}

TEST(SmallString, SmallStringBeginPlusOneIsEqualEndForSmallStringWidthSizeOne)
{
    SmallString text("x");

    auto beginPlusOne = text.begin() + std::size_t(1);

    ASSERT_THAT(beginPlusOne, Eq(text.end()));
}

TEST(SmallString, SmallStringRBeginIsEqualREndForEmptySmallString)
{
    SmallString text;

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TEST(SmallString, SmallStringRBeginIsNotEqualREndForNonEmptySmallString)
{
    SmallString text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TEST(SmallString, SmallStringRBeginPlusOneIsEqualREndForSmallStringWidthSizeOne)
{
    SmallString text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TEST(SmallString, SmallStringConstRBeginIsEqualREndForEmptySmallString)
{
    const SmallString text;

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TEST(SmallString, SmallStringConstRBeginIsNotEqualREndForNonEmptySmallString)
{
    const SmallString text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TEST(SmallString, SmallStringSmallStringConstRBeginPlusOneIsEqualREndForSmallStringWidthSizeOne)
{
    const SmallString text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TEST(SmallString, SmallStringDistanceBetweenBeginAndEndIsZeroForEmptyText)
{
    SmallString text("");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, SmallStringDistanceBetweenBeginAndEndIsOneForOneSign)
{
    SmallString text("x");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, SmallStringDistanceBetweenRBeginAndREndIsZeroForEmptyText)
{
    SmallString text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, SmallStringDistanceBetweenRBeginAndREndIsOneForOneSign)
{
    SmallString text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, SmallStringBeginPointsToX)
{
    SmallString text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, SmallStringRBeginPointsToX)
{
    SmallString text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, ConstSmallStringBeginPointsToX)
{
    const SmallString text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, ConstSmallStringRBeginPointsToX)
{
    const SmallString text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, SmallStringViewBeginIsEqualEndForEmptySmallString)
{
    SmallStringView text{""};

    ASSERT_THAT(text.begin(), Eq(text.end()));
}

TEST(SmallString, SmallStringViewBeginIsNotEqualEndForNonEmptySmallString)
{
    SmallStringView text("x");

    ASSERT_THAT(text.begin(), Ne(text.end()));
}

TEST(SmallString, SmallStringViewBeginPlusOneIsEqualEndForSmallStringWidthSizeOne)
{
    SmallStringView text("x");

    auto beginPlusOne = text.begin() + std::size_t(1);

    ASSERT_THAT(beginPlusOne, Eq(text.end()));
}

TEST(SmallString, SmallStringViewRBeginIsEqualREndForEmptySmallString)
{
    SmallStringView text{""};

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TEST(SmallString, SmallStringViewRBeginIsNotEqualREndForNonEmptySmallString)
{
    SmallStringView text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TEST(SmallString, SmallStringViewRBeginPlusOneIsEqualREndForSmallStringWidthSizeOne)
{
    SmallStringView text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TEST(SmallString, SmallStringViewConstRBeginIsEqualREndForEmptySmallString)
{
    const SmallStringView text{""};

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TEST(SmallString, SmallStringViewConstRBeginIsNotEqualREndForNonEmptySmallString)
{
    const SmallStringView text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TEST(SmallString, SmallStringViewConstRBeginPlusOneIsEqualREndForSmallStringWidthSizeOne)
{
    const SmallStringView text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TEST(SmallString, SmallStringViewDistanceBetweenBeginAndEndIsZeroForEmptyText)
{
    SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, SmallStringViewDistanceBetweenBeginAndEndIsOneForOneSign)
{
    SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, SmallStringViewDistanceBetweenRBeginAndREndIsZeroForEmptyText)
{
    SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, SmallStringViewDistanceBetweenRBeginAndREndIsOneForOneSign)
{
    SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, ConstSmallStringViewDistanceBetweenBeginAndEndIsZeroForEmptyText)
{
    const SmallStringView text("");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, ConstSmallStringViewDistanceBetweenBeginAndEndIsOneForOneSign)
{
    const SmallStringView text("x");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, ConstSmallStringViewDistanceBetweenRBeginAndREndIsZeroForEmptyText)
{
    const SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, ConstSmallStringViewDistanceBetweenRBeginAndREndIsOneForOneSign)
{
    const SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, SmallStringViewBeginPointsToX)
{
    SmallStringView text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, SmallStringViewRBeginPointsToX)
{
    SmallStringView text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, ConstSmallStringViewBeginPointsToX)
{
    const SmallStringView text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, ConstSmallStringViewRBeginPointsToX)
{
    const SmallStringView text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, SmallStringLiteralViewRBeginPointsToX)
{
    SmallStringLiteral text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, ConstSmallStringLiteralViewRBeginPointsToX)
{
    const SmallStringLiteral text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, ConstructorStandardString)
{
    std::string stdStringText = "short string";

    auto text = SmallString(stdStringText);

    ASSERT_THAT(text, SmallString("short string"));
}

TEST(SmallString, ToQString)
{
    SmallString text("short string");

    auto qStringText = text;

    ASSERT_THAT(qStringText, QStringLiteral("short string"));
}

TEST(SmallString, FromQString)
{
    QString qStringText = QStringLiteral("short string");

    auto text = SmallString::fromQString(qStringText);

    ASSERT_THAT(text, SmallString("short string"));
}


TEST(SmallString, FromQByteArray)
{
    QByteArray qByteArray = QByteArrayLiteral("short string");

    auto text = SmallString::fromQByteArray(qByteArray);

    ASSERT_THAT(text, SmallString("short string"));
}

TEST(SmallString, MidOneParameter)
{
    SmallString text("some text");

    auto midString = text.mid(5);

    ASSERT_THAT(midString, Eq(SmallString("text")));
}

TEST(SmallString, MidTwoParameter)
{
    SmallString text("some text and more");

    auto midString = text.mid(5, 4);

    ASSERT_THAT(midString, Eq(SmallString("text")));
}

TEST(SmallString, SizeOfEmptyStringl)
{
    SmallString emptyString;

    auto size = emptyString.size();

    ASSERT_THAT(size, 0);
}

TEST(SmallString, SizeShortSmallStringLiteral)
{
    SmallStringLiteral shortText("text");

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TEST(SmallString, SizeLongSmallStringLiteral)
{
    auto longText = SmallStringLiteral("very very very very very very very very very very very long string");

    auto size = longText.size();

    ASSERT_THAT(size, 66);
}

TEST(SmallString, SizeReference)
{
    SmallString shortText("text");

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TEST(SmallString, SizeShortSmallString)
{
    SmallString shortText("text", 4);

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TEST(SmallString, SizeShortPathString)
{
    SmallString shortPath("very very very very very very very very very very very long path which fits in the short memory");

    auto size = shortPath.size();

    ASSERT_THAT(size, 95);
}

TEST(SmallString, SizeLongSmallString)
{
    auto longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto size = longText.size();

    ASSERT_THAT(size, 66);
}

TEST(SmallString, CapacityReference)
{
    SmallString shortText("very very very very very very very long string");

    auto capacity = shortText.capacity();

    ASSERT_THAT(capacity, 0);
}

TEST(SmallString, CapacityShortSmallString)
{
    SmallString shortText("text", 4);

    auto capacity = shortText.capacity();

    ASSERT_THAT(capacity, 30);
}

TEST(SmallString, CapacityLongSmallString)
{
    auto longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto capacity = longText.capacity();

    ASSERT_THAT(capacity, 66);
}

TEST(SmallString, FitsNotInCapacityBecauseNullSmallStringIsAShortSmallString)
{
    SmallString text;

    ASSERT_FALSE(text.fitsNotInCapacity(30));
}

TEST(SmallString, FitsNotInCapacityBecauseItIsReference)
{
    SmallString text("very very very very very very very long string");

    ASSERT_TRUE(text.fitsNotInCapacity(1));
}

TEST(SmallString, FitsInShortSmallStringCapacity)
{
    SmallString text("text", 4);

    ASSERT_FALSE(text.fitsNotInCapacity(30));
}

TEST(SmallString, FitsInNotShortSmallStringCapacity)
{
    SmallString text("text", 4);

    ASSERT_TRUE(text.fitsNotInCapacity(31));
}

TEST(SmallString, FitsInLongSmallStringCapacity)
{
    SmallString text = SmallString::fromUtf8("very very very very very very long string");

    ASSERT_FALSE(text.fitsNotInCapacity(33)) << text.capacity();
}

TEST(SmallString, FitsNotInLongSmallStringCapacity)
{
    SmallString text = SmallString::fromUtf8("very very very very very very long string");

    ASSERT_TRUE(text.fitsNotInCapacity(65)) << text.capacity();
}

TEST(SmallString, AppendNullSmallString)
{
    SmallString text("text");

    text += SmallString();

    ASSERT_THAT(text, SmallString("text"));
}

TEST(SmallString, AppendEmptySmallString)
{
    SmallString text("text");

    text += SmallString("");

    ASSERT_THAT(text, SmallString("text"));
}

TEST(SmallString, AppendShortSmallString)
{
    SmallString text("some ");

    text += SmallString("text");

    ASSERT_THAT(text, SmallString("some text"));
}

TEST(SmallString, AppendLongSmallStringToShortSmallString)
{
    SmallString text("some ");

    text += SmallString("very very very very very long string");

    ASSERT_THAT(text, SmallString("some very very very very very long string"));
}

TEST(SmallString, AppendLongSmallString)
{
    SmallString longText("some very very very very very very very very very very very long string");
    longText+= SmallString(" text");

    ASSERT_THAT(longText, SmallString("some very very very very very very very very very very very long string text"));
}

TEST(SmallString, AppendInitializerList)
{
    SmallString text("some text");

    text += {" and", " some", " other", " text"};

    ASSERT_THAT(text, Eq("some text and some other text"));
}

TEST(SmallString, AppendEmptyInitializerList)
{
    SmallString text("some text");

    text += {};

    ASSERT_THAT(text, Eq("some text"));
}

TEST(SmallString, ToByteArray)
{
    SmallString text("some text");

    ASSERT_THAT(text.toQByteArray(), QByteArrayLiteral("some text"));
}

TEST(SmallString, Contains)
{
    SmallString text("some text");

    ASSERT_TRUE(text.contains(SmallString("text")));
    ASSERT_TRUE(text.contains("text"));
    ASSERT_TRUE(text.contains('x'));
}

TEST(SmallString, NotContains)
{
    SmallString text("some text");

    ASSERT_FALSE(text.contains(SmallString("textTwo")));
    ASSERT_FALSE(text.contains("foo"));
    ASSERT_FALSE(text.contains('q'));
}

TEST(SmallString, EqualSmallStringOperator)
{
    ASSERT_TRUE(SmallString() == SmallString(""));
    ASSERT_FALSE(SmallString() == SmallString("text"));
    ASSERT_TRUE(SmallString("text") == SmallString("text"));
    ASSERT_FALSE(SmallString("text") == SmallString("text2"));
}

TEST(SmallString, EqualSmallStringOperatorWithDifferenceClassSizes)
{
    ASSERT_TRUE(SmallString() == PathString(""));
    ASSERT_FALSE(SmallString() == PathString("text"));
    ASSERT_TRUE(SmallString("text") == PathString("text"));
    ASSERT_FALSE(SmallString("text") == PathString("text2"));
}

TEST(SmallString, EqualCStringArrayOperator)
{
    ASSERT_TRUE(SmallString() == "");
    ASSERT_FALSE(SmallString() == "text");
    ASSERT_TRUE(SmallString("text") == "text");
    ASSERT_FALSE(SmallString("text") == "text2");
}

TEST(SmallString, EqualCStringPointerOperator)
{
    ASSERT_TRUE(SmallString("text") == SmallString("text").data());
    ASSERT_FALSE(SmallString("text") == SmallString("text2").data());
}

TEST(SmallString, EqualSmallStringViewOperator)
{
    ASSERT_TRUE(SmallString("text") == SmallStringView("text"));
    ASSERT_FALSE(SmallString("text") == SmallStringView("text2"));
}

TEST(SmallString, EqualSmallStringViewsOperator)
{
    ASSERT_TRUE(SmallStringView("text") == SmallStringView("text"));
    ASSERT_FALSE(SmallStringView("text") == SmallStringView("text2"));
}

TEST(SmallString, UnequalOperator)
{
    ASSERT_FALSE(SmallString("text") != SmallString("text"));
    ASSERT_TRUE(SmallString("text") != SmallString("text2"));
}

TEST(SmallString, UnequalCStringArrayOperator)
{
    ASSERT_FALSE(SmallString("text") != "text");
    ASSERT_TRUE(SmallString("text") != "text2");
}

TEST(SmallString, UnequalCStringPointerOperator)
{
    ASSERT_FALSE(SmallString("text") != SmallString("text").data());
    ASSERT_TRUE(SmallString("text") != SmallString("text2").data());
}

TEST(SmallString, UnequalSmallStringViewArrayOperator)
{
    ASSERT_FALSE(SmallString("text") != SmallStringView("text"));
    ASSERT_TRUE(SmallString("text") != SmallStringView("text2"));
}

TEST(SmallString, UnequalSmallStringViewsArrayOperator)
{
    ASSERT_FALSE(SmallStringView("text") != SmallStringView("text"));
    ASSERT_TRUE(SmallStringView("text") != SmallStringView("text2"));
}

TEST(SmallString, SmallerOperator)
{
    ASSERT_TRUE(SmallString() < SmallString("text"));
    ASSERT_TRUE(SmallString("some") < SmallString("text"));
    ASSERT_TRUE(SmallString("text") < SmallString("texta"));
    ASSERT_FALSE(SmallString("texta") < SmallString("text"));
    ASSERT_FALSE(SmallString("text") < SmallString("some"));
    ASSERT_FALSE(SmallString("text") < SmallString("text"));
}

TEST(SmallString, SmallerOperatorWithStringViewRight)
{
    ASSERT_TRUE(SmallString() < SmallStringView("text"));
    ASSERT_TRUE(SmallString("some") < SmallStringView("text"));
    ASSERT_TRUE(SmallString("text") < SmallStringView("texta"));
    ASSERT_FALSE(SmallString("texta") < SmallStringView("text"));
    ASSERT_FALSE(SmallString("text") < SmallStringView("some"));
    ASSERT_FALSE(SmallString("text") < SmallStringView("text"));
}

TEST(SmallString, SmallerOperatorWithStringViewLeft)
{
    ASSERT_TRUE(SmallStringView("") < SmallString("text"));
    ASSERT_TRUE(SmallStringView("some") < SmallString("text"));
    ASSERT_TRUE(SmallStringView("text") < SmallString("texta"));
    ASSERT_FALSE(SmallStringView("texta") < SmallString("text"));
    ASSERT_FALSE(SmallStringView("text") < SmallString("some"));
    ASSERT_FALSE(SmallStringView("text") < SmallString("text"));
}

TEST(SmallString, SmallerOperatorForDifferenceClassSizes)
{
    ASSERT_TRUE(SmallString() < PathString("text"));
    ASSERT_TRUE(SmallString("some") < PathString("text"));
    ASSERT_TRUE(SmallString("text") < PathString("texta"));
    ASSERT_FALSE(SmallString("texta") < PathString("text"));
    ASSERT_FALSE(SmallString("text") < PathString("some"));
    ASSERT_FALSE(SmallString("text") < PathString("text"));
}

TEST(SmallString, IsEmpty)
{
    ASSERT_FALSE(SmallString("text").isEmpty());
    ASSERT_TRUE(SmallString("").isEmpty());
    ASSERT_TRUE(SmallString().isEmpty());
}

TEST(SmallString, StringViewIsEmpty)
{
    ASSERT_FALSE(SmallStringView("text").isEmpty());
    ASSERT_TRUE(SmallStringView("").isEmpty());
}

TEST(SmallString, StringViewEmpty)
{
    ASSERT_FALSE(SmallStringView("text").empty());
    ASSERT_TRUE(SmallStringView("").empty());
}

TEST(SmallString, HasContent)
{
    ASSERT_TRUE(SmallString("text").hasContent());
    ASSERT_FALSE(SmallString("").hasContent());
    ASSERT_FALSE(SmallString().hasContent());
}

TEST(SmallString, Clear)
{
    SmallString text("text");

    text.clear();

    ASSERT_TRUE(text.isEmpty());
}

TEST(SmallString, NoOccurrencesForEmptyText)
{
    SmallString text;

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 0);
}

TEST(SmallString, NoOccurrencesInText)
{
    SmallString text("here is some text, here is some text, here is some text");

    auto occurrences = text.countOccurrence("texts");

    ASSERT_THAT(occurrences, 0);
}

TEST(SmallString, SomeOccurrences)
{
    SmallString text("here is some text, here is some text, here is some text");

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 3);
}

TEST(SmallString, SomeMoreOccurrences)
{
    SmallString text("texttexttext");

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 3);
}

TEST(SmallString, ReplaceWithCharacter)
{
    SmallString text("here is some text, here is some text, here is some text");

    text.replace('s', 'x');

    ASSERT_THAT(text, SmallString("here ix xome text, here ix xome text, here ix xome text"));
}

TEST(SmallString, ReplaceWithEqualSizedText)
{
    SmallString text("here is some text");

    text.replace("some", "much");

    ASSERT_THAT(text, SmallString("here is much text"));
}

TEST(SmallString, ReplaceWithEqualSizedTextOnEmptyText)
{
    SmallString text;

    text.replace("some", "much");

    ASSERT_THAT(text, SmallString());
}

TEST(SmallString, ReplaceWithShorterText)
{
    SmallString text("here is some text");

    text.replace("some", "any");

    ASSERT_THAT(text, SmallString("here is any text"));
}

TEST(SmallString, ReplaceWithShorterTextOnEmptyText)
{
    SmallString text;

    text.replace("some", "any");

    ASSERT_THAT(text, SmallString());
}

TEST(SmallString, ReplaceWithLongerText)
{
    SmallString text("here is some text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text"));
}

TEST(SmallString, ReplaceWithLongerTextOnEmptyText)
{
    SmallString text;

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString());
}

TEST(SmallString, ReplaceShortSmallStringWithLongerText)
{
    SmallString text = SmallString::fromUtf8("here is some text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text"));
}

TEST(SmallString, ReplaceLongSmallStringWithLongerText)
{
    SmallString text = SmallString::fromUtf8("some very very very very very very very very very very very long string");

    text.replace("long", "much much much much much much much much much much much much much much much much much much more");

    ASSERT_THAT(text, "some very very very very very very very very very very very much much much much much much much much much much much much much much much much much much more string");
}

TEST(SmallString, MultipleReplaceSmallStringWithLongerText)
{
    SmallString text = SmallString("here is some text with some longer text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text with much more longer text"));
}

TEST(SmallString, MultipleReplaceSmallStringWithShorterText)
{
    SmallString text = SmallString("here is some text with some longer text");

    text.replace("some", "a");

    ASSERT_THAT(text, SmallString("here is a text with a longer text"));
}

TEST(SmallString, DontReplaceReplacedText)
{
    SmallString text("here is some foo text");

    text.replace("foo", "foofoo");

    ASSERT_THAT(text, SmallString("here is some foofoo text"));
}

TEST(SmallString, DontReserveIfNothingIsReplacedForLongerReplacementText)
{
    SmallString text("here is some text with some longer text");

    text.replace("bar", "foofoo");

    ASSERT_TRUE(text.isReadOnlyReference());
}

TEST(SmallString, DontReserveIfNothingIsReplacedForShorterReplacementText)
{
    SmallString text("here is some text with some longer text");

    text.replace("foofoo", "bar");

    ASSERT_TRUE(text.isReadOnlyReference());
}

TEST(SmallString, StartsWith)
{
    SmallString text("$column");

    ASSERT_FALSE(text.startsWith("$columnxxx"));
    ASSERT_TRUE(text.startsWith("$column"));
    ASSERT_TRUE(text.startsWith("$col"));
    ASSERT_FALSE(text.startsWith("col"));
    ASSERT_TRUE(text.startsWith('$'));
    ASSERT_FALSE(text.startsWith('@'));
}

TEST(SmallString, StartsWithStringView)
{
    SmallStringView text("$column");

    ASSERT_FALSE(text.startsWith("$columnxxx"));
    ASSERT_TRUE(text.startsWith("$column"));
    ASSERT_TRUE(text.startsWith("$col"));
    ASSERT_FALSE(text.startsWith("col"));
    ASSERT_TRUE(text.startsWith('$'));
    ASSERT_FALSE(text.startsWith('@'));
}

TEST(SmallString, EndsWith)
{
    SmallString text("/my/path");

    ASSERT_TRUE(text.endsWith("/my/path"));
    ASSERT_TRUE(text.endsWith("path"));
    ASSERT_FALSE(text.endsWith("paths"));
    ASSERT_TRUE(text.endsWith('h'));
    ASSERT_FALSE(text.endsWith('x'));
}

TEST(SmallString, EndsWithSmallString)
{
    SmallString text("/my/path");

    ASSERT_TRUE(text.endsWith(SmallString("path")));
    ASSERT_TRUE(text.endsWith('h'));
}

TEST(SmallString, ReserveSmallerThanShortStringCapacity)
{
    SmallString text("text");

    text.reserve(2);

    ASSERT_THAT(text.capacity(), AnyOf(30, 4));
}

TEST(SmallString, ReserveSmallerThanShortStringCapacityIsShortString)
{
    SmallString text("text");

    text.reserve(2);

    ASSERT_TRUE(text.isShortString());
}

TEST(SmallString, ReserveSmallerThanReference)
{
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(35);

    ASSERT_THAT(text.capacity(), 71);
}

TEST(SmallString, ReserveBiggerThanShortStringCapacity)
{
    SmallString text("text");

    text.reserve(10);

    ASSERT_THAT(text.capacity(), AnyOf(30, 10));
}

TEST(SmallString, ReserveBiggerThanReference)
{
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(35);

    ASSERT_THAT(text.capacity(), 71);
}

TEST(SmallString, ReserveMuchBiggerThanShortStringCapacity)
{
    SmallString text("text");

    text.reserve(100);

    ASSERT_THAT(text.capacity(), 100);
}

TEST(SmallString, TextIsCopiedAfterReserveFromShortToLongString)
{
    SmallString text("text");

    text.reserve(100);

    ASSERT_THAT(text, "text");
}

TEST(SmallString, TextIsCopiedAfterReserveReferenceToLongString)
{
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(100);

    ASSERT_THAT(text, "some very very very very very very very very very very very long string");
}

TEST(SmallString, ReserveSmallerThanShortSmallString)
{
    SmallString text = SmallString::fromUtf8("text");

    text.reserve(10);

    ASSERT_THAT(text.capacity(), 30);
}

TEST(SmallString, ReserveBiggerThanShortSmallString)
{
    SmallString text = SmallString::fromUtf8("text");

    text.reserve(100);

    ASSERT_THAT(text.capacity(), 100);
}

TEST(SmallString, ReserveBiggerThanLongSmallString)
{
    SmallString text = SmallString::fromUtf8("some very very very very very very very very very very very long string");

    text.reserve(100);

    ASSERT_THAT(text.capacity(), 100);
}

TEST(SmallString, OptimalHeapCacheLineForSize)
{
    ASSERT_THAT(SmallString::optimalHeapCapacity(64), 64);
    ASSERT_THAT(SmallString::optimalHeapCapacity(65), 128);
    ASSERT_THAT(SmallString::optimalHeapCapacity(127), 128);
    ASSERT_THAT(SmallString::optimalHeapCapacity(128), 128);
    ASSERT_THAT(SmallString::optimalHeapCapacity(129), 192);
    ASSERT_THAT(SmallString::optimalHeapCapacity(191), 192);
    ASSERT_THAT(SmallString::optimalHeapCapacity(193), 256);
    ASSERT_THAT(SmallString::optimalHeapCapacity(255), 256);
    ASSERT_THAT(SmallString::optimalHeapCapacity(256), 256);
    ASSERT_THAT(SmallString::optimalHeapCapacity(257), 320);
    ASSERT_THAT(SmallString::optimalHeapCapacity(383), 384);
    ASSERT_THAT(SmallString::optimalHeapCapacity(385), 448);
    ASSERT_THAT(SmallString::optimalHeapCapacity(4095), 4096);
    ASSERT_THAT(SmallString::optimalHeapCapacity(4096), 4096);
    ASSERT_THAT(SmallString::optimalHeapCapacity(4097), 4160);
}

TEST(SmallString, OptimalCapacityForSize)
{
    SmallString text;

    ASSERT_THAT(text.optimalCapacity(0), 0);
    ASSERT_THAT(text.optimalCapacity(30), 30);
    ASSERT_THAT(text.optimalCapacity(31), 63);
    ASSERT_THAT(text.optimalCapacity(63), 63);
    ASSERT_THAT(text.optimalCapacity(64), 127);
    ASSERT_THAT(text.optimalCapacity(128), 191);
}

TEST(SmallString, DataStreamData)
{
    SmallString inputText("foo");
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::ReadWrite);

    out << inputText;

    ASSERT_TRUE(byteArray.endsWith("foo"));
}

TEST(SmallString, ReadDataStreamSize)
{
    SmallString outputText("foo");
    QByteArray byteArray;
    quint32 size;
    {
        QDataStream out(&byteArray, QIODevice::WriteOnly);
        out << outputText;
    }
    QDataStream in(&byteArray, QIODevice::ReadOnly);

    in >> size;

    ASSERT_THAT(size, 3);
}

TEST(SmallString, ReadDataStreamData)
{
    SmallString outputText("foo");
    QByteArray byteArray;
    SmallString outputString;
    {
        QDataStream out(&byteArray, QIODevice::WriteOnly);
        out << outputText;
    }
    QDataStream in(&byteArray, QIODevice::ReadOnly);

    in >> outputString;

    ASSERT_THAT(outputString, SmallString("foo"));
}

TEST(SmallString, ShortSmallStringCopyConstuctor)
{
    SmallString text("text");

    auto copy = text;

    ASSERT_THAT(copy, text);
}

TEST(SmallString, LongSmallStringCopyConstuctor)
{
    SmallString text("this is a very very very very long text");

    auto copy = text;

    ASSERT_THAT(copy, text);
}

TEST(SmallString, ShortSmallStringMoveConstuctor)
{
    SmallString text("text");

    auto copy = std::move(text);

    ASSERT_TRUE(text.isEmpty());
    ASSERT_THAT(copy, SmallString("text"));
}

TEST(SmallString, LongSmallStringMoveConstuctor)
{
    SmallString text("this is a very very very very long text");

    auto copy = std::move(text);

    ASSERT_TRUE(text.isEmpty());
    ASSERT_THAT(copy, SmallString("this is a very very very very long text"));
}

TEST(SmallString, ShortSmallStringCopyAssignment)
{
    SmallString text("text");
    SmallString copy("more text");

    copy = text;

    ASSERT_THAT(copy, text);
}

TEST(SmallString, LongSmallStringCopyAssignment)
{
    SmallString text("this is a very very very very long text");
    SmallString copy("more text");

    copy = text;

    ASSERT_THAT(copy, text);
}

TEST(SmallString, LongSmallStringCopySelfAssignment)
{
    SmallString text("this is a very very very very long text");

    text = text;

    ASSERT_THAT(text, SmallString("this is a very very very very long text"));
}

TEST(SmallString, ShortSmallStringMoveAssignment)
{
    SmallString text("text");
    SmallString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(text, IsEmpty());
    ASSERT_THAT(copy, SmallString("text"));
}

TEST(SmallString, LongSmallStringMoveAssignment)
{
    SmallString text("this is a very very very very long text");
    SmallString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(text, IsEmpty());
    ASSERT_THAT(copy, SmallString("this is a very very very very long text"));
}

TEST(SmallString, ReplaceByPositionShorterWithLongerText)
{
    SmallString text("this is a very very very very long text");

    text.replace(8, 1, "some");

    ASSERT_THAT(text, SmallString("this is some very very very very long text"));
}

TEST(SmallString, ReplaceByPositionLongerWithShortText)
{
    SmallString text("this is some very very very very long text");

    text.replace(8, 4, "a");

    ASSERT_THAT(text, SmallString("this is a very very very very long text"));
}

TEST(SmallString, ReplaceByPositionEqualSizedTexts)
{
    SmallString text("this is very very very very very long text");

    text.replace(33, 4, "much");

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, CompareTextWithDifferentLineEndings)
{
    SmallString unixText("some \ntext");
    SmallString windowsText("some \n\rtext");

    auto convertedText = windowsText.toCarriageReturnsStripped();

    ASSERT_THAT(unixText, convertedText);
}

TEST(SmallString, ConstSubscriptOperator)
{
    const SmallString text = {"some text"};

    auto &&sign = text[5];

    ASSERT_THAT(sign, 't');
}

TEST(SmallString, NonConstSubscriptOperator)
{
    SmallString text = {"some text"};

    auto &&sign = text[5];

    ASSERT_THAT(sign, 't');
}

TEST(SmallString, ManipulateConstSubscriptOperator)
{
    const SmallString text = {"some text"};
    auto &&sign = text[5];

    sign = 'q';

    ASSERT_THAT(text, SmallString{"some text"});
}

TEST(SmallString, ManipulateNonConstSubscriptOperator)
{
    char rawText[] = "some text";
    SmallString text{rawText};
    auto &&sign = text[5];

    sign = 'q';

    ASSERT_THAT(text, SmallString{"some qext"});
}

TEST(SmallString, EmptyInitializerListContent)
{
    SmallString text = {};

    ASSERT_THAT(text, SmallString());
}

TEST(SmallString, EmptyInitializerListSize)
{
    SmallString text = {};

    ASSERT_THAT(text, SizeIs(0));
}

TEST(SmallString, EmptyInitializerListNullTerminated)
{
    auto end = SmallString{{}}[0];

    ASSERT_THAT(end, '\0');
}

TEST(SmallString, InitializerListContent)
{
    SmallString text = {"some", " ", "text"};

    ASSERT_THAT(text, SmallString("some text"));
}

TEST(SmallString, InitializerListSize)
{
    SmallString text = {"some", " ", "text"};

    ASSERT_THAT(text, SizeIs(9));
}

TEST(SmallString, InitializerListNullTerminated)
{
    auto end = SmallString{"some", " ", "text"}[9];

    ASSERT_THAT(end, '\0');
}

TEST(SmallString, NumberToString)
{
    ASSERT_THAT(SmallString::number(-0), "0");
    ASSERT_THAT(SmallString::number(1), "1");
    ASSERT_THAT(SmallString::number(-1), "-1");
    ASSERT_THAT(SmallString::number(std::numeric_limits<int>::max()), "2147483647");
    ASSERT_THAT(SmallString::number(std::numeric_limits<int>::min()), "-2147483648");
    ASSERT_THAT(SmallString::number(std::numeric_limits<long long int>::max()), "9223372036854775807");
    ASSERT_THAT(SmallString::number(std::numeric_limits<long long int>::min()), "-9223372036854775808");
    ASSERT_THAT(SmallString::number(1.2), "1.200000");
    ASSERT_THAT(SmallString::number(-1.2), "-1.200000");
}

TEST(SmallString, StringViewPlusOperator)
{
    SmallStringView text = "text";

    auto result = text + " and more text";

    ASSERT_THAT(result, "text and more text");
}

TEST(SmallString, StringPlusOperator)
{
    SmallString text = "text";

    auto result = text + " and more text";

    ASSERT_THAT(result, "text and more text");
}

TEST(SmallString, ShortStringCapacity)
{
    ASSERT_THAT(SmallString().shortStringCapacity(), 30);
    ASSERT_THAT(PathString().shortStringCapacity(), 189);
}

TEST(SmallString, ToView)
{
    SmallString text = "text";

    auto view = text.toView();

    ASSERT_THAT(view, "text");

}

TEST(SmallString, Compare)
{
    const char longText[] = "textfoo";

    ASSERT_THAT(Utils::compare("", ""), Eq(0));
    ASSERT_THAT(Utils::compare("text", "text"), Eq(0));
    ASSERT_THAT(Utils::compare("text", Utils::SmallStringView(longText, 4)), Eq(0));
    ASSERT_THAT(Utils::compare("", "text"), Le(0));
    ASSERT_THAT(Utils::compare("textx", "text"), Gt(0));
    ASSERT_THAT(Utils::compare("text", "textx"), Le(0));
    ASSERT_THAT(Utils::compare("textx", "texta"), Gt(0));
    ASSERT_THAT(Utils::compare("texta", "textx"), Le(0));
}

TEST(SmallString, ReverseCompare)
{
    const char longText[] = "textfoo";

    ASSERT_THAT(Utils::reverseCompare("", ""), Eq(0));
    ASSERT_THAT(Utils::reverseCompare("text", "text"), Eq(0));
    ASSERT_THAT(Utils::reverseCompare("text", Utils::SmallStringView(longText, 4)), Eq(0));
    ASSERT_THAT(Utils::reverseCompare("", "text"), Le(0));
    ASSERT_THAT(Utils::reverseCompare("textx", "text"), Gt(0));
    ASSERT_THAT(Utils::reverseCompare("text", "textx"), Le(0));
    ASSERT_THAT(Utils::reverseCompare("textx", "texta"), Gt(0));
    ASSERT_THAT(Utils::reverseCompare("texta", "textx"), Le(0));
}
