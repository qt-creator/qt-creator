// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <QString>

#include <utils/smallstring.h>
#include <utils/smallstringio.h>
#include <utils/smallstringvector.h>

#include <random>

using Utils::PathString;
using Utils::SmallString;
using Utils::SmallStringLiteral;
using Utils::SmallStringView;

static_assert(32 == sizeof(Utils::BasicSmallString<31>));
static_assert(64 == sizeof(Utils::BasicSmallString<63>));
static_assert(192 == sizeof(Utils::BasicSmallString<190>));

static_assert(16 == alignof(Utils::BasicSmallString<31>));
static_assert(16 == alignof(Utils::BasicSmallString<63>));
static_assert(16 == alignof(Utils::BasicSmallString<190>));

TEST(SmallString, basic_string_equal)
{
    ASSERT_THAT(SmallString("text"), Eq(SmallString("text")));
}

TEST(SmallString, basic_small_string_unequal)
{
    ASSERT_THAT(SmallString("text"), Ne(SmallString("other text")));
}

TEST(SmallString, null_small_string_is_equal_to_empty_small_string)
{
    ASSERT_THAT(SmallString(), Eq(SmallString("")));
}

TEST(SmallString, short_small_string_literal_is_short_small_string)
{
    // constexpr
    SmallStringLiteral shortText("short string");

    ASSERT_TRUE(shortText.isShortString());
}

TEST(SmallString, short_small_string_is_short_small_string)
{
    SmallString shortText("short string");

    ASSERT_TRUE(shortText.isShortString());
}

TEST(SmallString, create_from_c_string_iterators)
{
    char sourceText[] = "this is very very very very very much text";

    SmallString text(sourceText, &sourceText[sizeof(sourceText) - 1]);

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, create_from_q_byte_array_iterators)
{
    QByteArray sourceText = "this is very very very very very much text";

    SmallString text(sourceText.begin(), sourceText.end());

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, create_from_small_string_iterators)
{
    SmallString sourceText = "this is very very very very very much text";

    SmallString text(sourceText.begin(), sourceText.end());

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, create_from_string_view)
{
    SmallStringView sourceText = "this is very very very very very much text";

    SmallString text(sourceText);

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, short_small_string_is_reference)
{
    SmallString longText("very very very very very long text");

    ASSERT_TRUE(longText.isReadOnlyReference());
}

TEST(SmallString, small_string_contructor_is_not_reference)
{
    const char *shortCSmallString = "short string";
    auto shortText = SmallString(shortCSmallString);

    ASSERT_TRUE(shortText.isShortString());
}

TEST(SmallString, short_small_string_is_not_reference)
{
    const char *shortCSmallString = "short string";
    auto shortText = SmallString::fromUtf8(shortCSmallString);

    ASSERT_FALSE(shortText.isReadOnlyReference());
}

TEST(SmallString, long_small_string_construtor_is_allocated)
{
    const char *longCSmallString = "very very very very very long text";
    auto longText = SmallString(longCSmallString);

    ASSERT_TRUE(longText.hasAllocatedMemory());
}

TEST(SmallString, maximum_short_small_string)
{
    SmallString maximumShortText("very very very very short text", 30);

    ASSERT_THAT(maximumShortText, StrEq("very very very very short text"));
}

TEST(SmallString, long_const_expression_small_string_is_reference)
{
    SmallString longText("very very very very very very very very very very very long string");

    ASSERT_TRUE(longText.isReadOnlyReference());
}

TEST(SmallString, clone_short_small_string)
{
    SmallString shortText("short string");

    auto clonedText = shortText.clone();

    ASSERT_THAT(clonedText, Eq("short string"));
}

TEST(SmallString, clone_long_small_string)
{
    SmallString longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto clonedText = longText.clone();

    ASSERT_THAT(clonedText, Eq("very very very very very very very very very very very long string"));
}

TEST(SmallString, cloned_long_small_string_data_pointer_is_different)
{
    SmallString longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto clonedText = longText.clone();

    ASSERT_THAT(clonedText.data(), Ne(longText.data()));
}

TEST(SmallString, copy_short_const_expression_small_string_is_short_small_string)
{
    SmallString shortText("short string");

    auto shortTextCopy = shortText;

    ASSERT_TRUE(shortTextCopy.isShortString());
}

TEST(SmallString, copy_long_const_expression_small_string_is_long_small_string)
{
    SmallString longText("very very very very very very very very very very very long string");

    auto longTextCopy = longText;

    ASSERT_FALSE(longTextCopy.isShortString());
}

TEST(SmallString, short_path_string_is_short_string)
{
    const char *rawText = "very very very very very very very very very very very long path which fits in the short memory";

    PathString text(rawText);

    ASSERT_TRUE(text.isShortString());
}

TEST(SmallString, small_string_from_character_array_is_reference)
{
    const char longCString[] = "very very very very very very very very very very very long string";

    SmallString longString(longCString);

    ASSERT_TRUE(longString.isReadOnlyReference());
}

TEST(SmallString, small_string_from_character_pointer_is_not_reference)
{
    const char *longCString = "very very very very very very very very very very very long string";

    SmallString longString = SmallString::fromUtf8(longCString);

    ASSERT_FALSE(longString.isReadOnlyReference());
}

TEST(SmallString, copy_string_from_reference)
{
    SmallString longText("very very very very very very very very very very very long string");
    SmallString longTextCopy;

    longTextCopy = longText;

    ASSERT_TRUE(longTextCopy.isReadOnlyReference());
}

TEST(SmallString, small_string_literal_short_small_string_data_access)
{
    SmallStringLiteral literalText("very very very very very very very very very very very long string");

    ASSERT_THAT(literalText, StrEq("very very very very very very very very very very very long string"));
}

TEST(SmallString, small_string_literal_long_small_string_data_access)
{
    SmallStringLiteral literalText("short string");

    ASSERT_THAT(literalText, StrEq("short string"));
}

TEST(SmallString, reference_data_access)
{
    SmallString literalText("short string");

    ASSERT_THAT(literalText, StrEq("short string"));
}

TEST(SmallString, short_data_access)
{
    const char *shortCString = "short string";
    auto shortText = SmallString::fromUtf8(shortCString);

    ASSERT_THAT(shortText, StrEq("short string"));
}

TEST(SmallString, long_data_access)
{
    const char *longCString = "very very very very very very very very very very very long string";
    auto longText = SmallString::fromUtf8(longCString);

    ASSERT_THAT(longText, StrEq(longCString));
}

TEST(SmallString, long_small_string_has_short_small_string_size_zero)
{
    auto longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    ASSERT_THAT(longText.shortStringSize(), 0);
}

TEST(SmallString, small_string_begin_is_equal_end_for_empty_small_string)
{
    SmallString text;

    ASSERT_THAT(text.begin(), Eq(text.end()));
}

TEST(SmallString, small_string_begin_is_not_equal_end_for_non_empty_small_string)
{
    SmallString text("x");

    ASSERT_THAT(text.begin(), Ne(text.end()));
}

TEST(SmallString, small_string_begin_plus_one_is_equal_end_for_small_string_width_size_one)
{
    SmallString text("x");

    auto beginPlusOne = text.begin() + std::size_t(1);

    ASSERT_THAT(beginPlusOne, Eq(text.end()));
}

TEST(SmallString, small_string_r_begin_is_equal_r_end_for_empty_small_string)
{
    SmallString text;

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TEST(SmallString, small_string_r_begin_is_not_equal_r_end_for_non_empty_small_string)
{
    SmallString text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TEST(SmallString, small_string_r_begin_plus_one_is_equal_r_end_for_small_string_width_size_one)
{
    SmallString text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TEST(SmallString, small_string_const_r_begin_is_equal_r_end_for_empty_small_string)
{
    const SmallString text;

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TEST(SmallString, small_string_const_r_begin_is_not_equal_r_end_for_non_empty_small_string)
{
    const SmallString text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TEST(SmallString, small_string_small_string_const_r_begin_plus_one_is_equal_r_end_for_small_string_width_size_one)
{
    const SmallString text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TEST(SmallString, small_string_distance_between_begin_and_end_is_zero_for_empty_text)
{
    SmallString text("");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, small_string_distance_between_begin_and_end_is_one_for_one_sign)
{
    SmallString text("x");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, small_string_distance_between_r_begin_and_r_end_is_zero_for_empty_text)
{
    SmallString text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, small_string_distance_between_r_begin_and_r_end_is_one_for_one_sign)
{
    SmallString text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, small_string_begin_points_to_x)
{
    SmallString text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, small_string_r_begin_points_to_x)
{
    SmallString text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, const_small_string_begin_points_to_x)
{
    const SmallString text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, const_small_string_r_begin_points_to_x)
{
    const SmallString text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, small_string_view_begin_is_equal_end_for_empty_small_string)
{
    SmallStringView text{""};

    ASSERT_THAT(text.begin(), Eq(text.end()));
}

TEST(SmallString, small_string_view_begin_is_not_equal_end_for_non_empty_small_string)
{
    SmallStringView text("x");

    ASSERT_THAT(text.begin(), Ne(text.end()));
}

TEST(SmallString, small_string_view_begin_plus_one_is_equal_end_for_small_string_width_size_one)
{
    SmallStringView text("x");

    auto beginPlusOne = text.begin() + std::size_t(1);

    ASSERT_THAT(beginPlusOne, Eq(text.end()));
}

TEST(SmallString, small_string_view_r_begin_is_equal_r_end_for_empty_small_string)
{
    SmallStringView text{""};

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TEST(SmallString, small_string_view_r_begin_is_not_equal_r_end_for_non_empty_small_string)
{
    SmallStringView text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TEST(SmallString, small_string_view_r_begin_plus_one_is_equal_r_end_for_small_string_width_size_one)
{
    SmallStringView text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TEST(SmallString, small_string_view_const_r_begin_is_equal_r_end_for_empty_small_string)
{
    const SmallStringView text{""};

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TEST(SmallString, small_string_view_const_r_begin_is_not_equal_r_end_for_non_empty_small_string)
{
    const SmallStringView text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TEST(SmallString, small_string_view_const_r_begin_plus_one_is_equal_r_end_for_small_string_width_size_one)
{
    const SmallStringView text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TEST(SmallString, small_string_view_distance_between_begin_and_end_is_zero_for_empty_text)
{
    SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, small_string_view_distance_between_begin_and_end_is_one_for_one_sign)
{
    SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, small_string_view_distance_between_r_begin_and_r_end_is_zero_for_empty_text)
{
    SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, small_string_view_distance_between_r_begin_and_r_end_is_one_for_one_sign)
{
    SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, const_small_string_view_distance_between_begin_and_end_is_zero_for_empty_text)
{
    const SmallStringView text("");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, const_small_string_view_distance_between_begin_and_end_is_one_for_one_sign)
{
    const SmallStringView text("x");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, const_small_string_view_distance_between_r_begin_and_r_end_is_zero_for_empty_text)
{
    const SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TEST(SmallString, const_small_string_view_distance_between_r_begin_and_r_end_is_one_for_one_sign)
{
    const SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TEST(SmallString, small_string_view_begin_points_to_x)
{
    SmallStringView text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, small_string_view_r_begin_points_to_x)
{
    SmallStringView text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, const_small_string_view_begin_points_to_x)
{
    const SmallStringView text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, const_small_string_view_r_begin_points_to_x)
{
    const SmallStringView text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, small_string_literal_view_r_begin_points_to_x)
{
    SmallStringLiteral text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, const_small_string_literal_view_r_begin_points_to_x)
{
    const SmallStringLiteral text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TEST(SmallString, constructor_standard_string)
{
    std::string stdStringText = "short string";

    auto text = SmallString(stdStringText);

    ASSERT_THAT(text, SmallString("short string"));
}

TEST(SmallString, to_q_string)
{
    SmallString text("short string");

    auto qStringText = text;

    ASSERT_THAT(qStringText, QStringLiteral("short string"));
}

class FromQString : public testing::TestWithParam<qsizetype>
{
protected:
    QString exampleString(qsizetype size)
    {
        static constexpr char16_t characters[] = u"0123456789"
                                                 "abcdefghijklmnopqrstuvwxyz"
                                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        QString string;

        string.reserve(size);

        std::size_t index = 0;
        std::generate_n(std::back_inserter(string), size, [&]() {
            if (index >= std::size(characters))
                index = 0;

            return characters[index++];
        });

        return string;
    }

protected:
    qsizetype size = GetParam();
};

INSTANTIATE_TEST_SUITE_P(SmallString, FromQString, testing::Range<qsizetype>(0, 10000, 300));

TEST_P(FromQString, from_qstring)
{
    const QString qStringText = exampleString(size);

    auto text = SmallString(qStringText);

    ASSERT_THAT(text, qStringText.toStdString());
}

TEST(SmallString, from_q_byte_array)
{
    QByteArray qByteArray = QByteArrayLiteral("short string");

    auto text = SmallString::fromQByteArray(qByteArray);

    ASSERT_THAT(text, SmallString("short string"));
}

TEST(SmallString, mid_one_parameter)
{
    SmallString text("some text");

    auto midString = text.mid(5);

    ASSERT_THAT(midString, Eq(SmallString("text")));
}

TEST(SmallString, mid_two_parameter)
{
    SmallString text("some text and more");

    auto midString = text.mid(5, 4);

    ASSERT_THAT(midString, Eq(SmallString("text")));
}

TEST(SmallString, small_string_view_mid_one_parameter)
{
    SmallStringView text("some text");

    auto midString = text.mid(5);

    ASSERT_THAT(midString, Eq(SmallStringView("text")));
}

TEST(SmallString, small_string_view_mid_two_parameter)
{
    SmallStringView text("some text and more");

    auto midString = text.mid(5, 4);

    ASSERT_THAT(midString, Eq(SmallStringView("text")));
}

TEST(SmallString, size_of_empty_stringl)
{
    SmallString emptyString;

    auto size = emptyString.size();

    ASSERT_THAT(size, 0);
}

TEST(SmallString, size_short_small_string_literal)
{
    SmallStringLiteral shortText("text");

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TEST(SmallString, size_long_small_string_literal)
{
    auto longText = SmallStringLiteral("very very very very very very very very very very very long string");

    auto size = longText.size();

    ASSERT_THAT(size, 66);
}

TEST(SmallString, size_reference)
{
    SmallString shortText("text");

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TEST(SmallString, size_short_small_string)
{
    SmallString shortText("text", 4);

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TEST(SmallString, size_short_path_string)
{
    SmallString shortPath("very very very very very very very very very very very long path which fits in the short memory");

    auto size = shortPath.size();

    ASSERT_THAT(size, 95);
}

TEST(SmallString, size_long_small_string)
{
    auto longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto size = longText.size();

    ASSERT_THAT(size, 66);
}

TEST(SmallString, capacity_reference)
{
    SmallString shortText("very very very very very very very long string");

    auto capacity = shortText.capacity();

    ASSERT_THAT(capacity, 0);
}

TEST(SmallString, capacity_short_small_string)
{
    SmallString shortText("text", 4);

    auto capacity = shortText.capacity();

    ASSERT_THAT(capacity, 31);
}

TEST(SmallString, capacity_long_small_string)
{
    auto longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto capacity = longText.capacity();

    ASSERT_THAT(capacity, 66);
}

TEST(SmallString, fits_not_in_capacity_because_null_small_string_is_a_short_small_string)
{
    SmallString text;

    ASSERT_FALSE(text.fitsNotInCapacity(30));
}

TEST(SmallString, fits_not_in_capacity_because_it_is_reference)
{
    SmallString text("very very very very very very very long string");

    ASSERT_TRUE(text.fitsNotInCapacity(1));
}

TEST(SmallString, fits_in_short_small_string_capacity)
{
    SmallString text("text", 4);

    ASSERT_FALSE(text.fitsNotInCapacity(30));
}

TEST(SmallString, fits_in_not_short_small_string_capacity)
{
    SmallString text("text", 4);

    ASSERT_TRUE(text.fitsNotInCapacity(32));
}

TEST(SmallString, fits_in_long_small_string_capacity)
{
    SmallString text = SmallString::fromUtf8("very very very very very very long string");

    ASSERT_FALSE(text.fitsNotInCapacity(33)) << text.capacity();
}

TEST(SmallString, fits_not_in_long_small_string_capacity)
{
    SmallString text = SmallString::fromUtf8("very very very very very very long string");

    ASSERT_TRUE(text.fitsNotInCapacity(65)) << text.capacity();
}

TEST(SmallString, append_null_small_string)
{
    SmallString text("text");

    text += SmallString();

    ASSERT_THAT(text, SmallString("text"));
}

TEST(SmallString, append_null_q_string)
{
    SmallString text("text");

    text += QString();

    ASSERT_THAT(text, SmallString("text"));
}

TEST(SmallString, append_empty_small_string)
{
    SmallString text("text");

    text += SmallString("");

    ASSERT_THAT(text, SmallString("text"));
}

TEST(SmallString, append_empty_q_string)
{
    SmallString text("text");

    text += QString("");

    ASSERT_THAT(text, SmallString("text"));
}

TEST(SmallString, append_short_small_string)
{
    SmallString text("some ");

    text += SmallString("text");

    ASSERT_THAT(text, SmallString("some text"));
}

TEST(SmallString, append_short_q_string)
{
    SmallString text("some ");

    text += QString("text");

    ASSERT_THAT(text, SmallString("some text"));
}

TEST(SmallString, append_long_small_string_to_short_small_string)
{
    SmallString text("some ");

    text += SmallString("very very very very very long string");

    ASSERT_THAT(text, SmallString("some very very very very very long string"));
}

TEST(SmallString, append_long_q_string_to_short_small_string)
{
    SmallString text("some ");

    text += QString("very very very very very long string");

    ASSERT_THAT(text, SmallString("some very very very very very long string"));
}

TEST(SmallString, append_long_small_string)
{
    SmallString longText("some very very very very very very very very very very very long string");

    longText += SmallString(" text");

    ASSERT_THAT(longText, SmallString("some very very very very very very very very very very very long string text"));
}

TEST(SmallString, append_long_q_string)
{
    SmallString longText("some very very very very very very very very very very very long string");

    longText += QString(" text");

    ASSERT_THAT(
        longText,
        SmallString(
            "some very very very very very very very very very very very long string text"));
}

TEST(SmallString, append_initializer_list)
{
    SmallString text("some text");

    text += {" and", " some", " other", " text"};

    ASSERT_THAT(text, Eq("some text and some other text"));
}

TEST(SmallString, append_empty_initializer_list)
{
    SmallString text("some text");

    text += {};

    ASSERT_THAT(text, Eq("some text"));
}

TEST(SmallString, to_byte_array)
{
    SmallString text("some text");

    ASSERT_THAT(text.toQByteArray(), QByteArrayLiteral("some text"));
}

TEST(SmallString, contains)
{
    SmallString text("some text");

    ASSERT_TRUE(text.contains(SmallString("text")));
    ASSERT_TRUE(text.contains("text"));
    ASSERT_TRUE(text.contains('x'));
}

TEST(SmallString, not_contains)
{
    SmallString text("some text");

    ASSERT_FALSE(text.contains(SmallString("textTwo")));
    ASSERT_FALSE(text.contains("foo"));
    ASSERT_FALSE(text.contains('q'));
}

TEST(SmallString, equal_small_string_operator)
{
    ASSERT_TRUE(SmallString() == SmallString(""));
    ASSERT_FALSE(SmallString() == SmallString("text"));
    ASSERT_TRUE(SmallString("text") == SmallString("text"));
    ASSERT_FALSE(SmallString("text") == SmallString("text2"));
}

TEST(SmallString, equal_small_string_operator_with_difference_class_sizes)
{
    ASSERT_TRUE(SmallString() == PathString(""));
    ASSERT_FALSE(SmallString() == PathString("text"));
    ASSERT_TRUE(SmallString("text") == PathString("text"));
    ASSERT_FALSE(SmallString("text") == PathString("text2"));
}

TEST(SmallString, equal_c_string_array_operator)
{
    ASSERT_TRUE(SmallString() == "");
    ASSERT_FALSE(SmallString() == "text");
    ASSERT_TRUE(SmallString("text") == "text");
    ASSERT_FALSE(SmallString("text") == "text2");
}

TEST(SmallString, equal_c_string_pointer_operator)
{
    ASSERT_TRUE(SmallString("text") == std::string("text").data());
    ASSERT_FALSE(SmallString("text") == std::string("text2").data());
}

TEST(SmallString, equal_small_string_view_operator)
{
    ASSERT_TRUE(SmallString("text") == SmallStringView("text"));
    ASSERT_FALSE(SmallString("text") == SmallStringView("text2"));
}

TEST(SmallString, equal_small_string_views_operator)
{
    ASSERT_TRUE(SmallStringView("text") == SmallStringView("text"));
    ASSERT_FALSE(SmallStringView("text") == SmallStringView("text2"));
}

TEST(SmallString, unequal_operator)
{
    ASSERT_FALSE(SmallString("text") != SmallString("text"));
    ASSERT_TRUE(SmallString("text") != SmallString("text2"));
}

TEST(SmallString, unequal_c_string_array_operator)
{
    ASSERT_FALSE(SmallString("text") != "text");
    ASSERT_TRUE(SmallString("text") != "text2");
}

TEST(SmallString, unequal_c_string_pointer_operator)
{
    ASSERT_FALSE(SmallString("text") != std::string("text").data());
    ASSERT_TRUE(SmallString("text") != std::string("text2").data());
}

TEST(SmallString, unequal_small_string_view_array_operator)
{
    ASSERT_FALSE(SmallString("text") != SmallStringView("text"));
    ASSERT_TRUE(SmallString("text") != SmallStringView("text2"));
}

TEST(SmallString, unequal_small_string_views_array_operator)
{
    ASSERT_FALSE(SmallStringView("text") != SmallStringView("text"));
    ASSERT_TRUE(SmallStringView("text") != SmallStringView("text2"));
}

TEST(SmallString, smaller_operator)
{
    ASSERT_TRUE(SmallString() < SmallString("text"));
    ASSERT_TRUE(SmallString("some") < SmallString("text"));
    ASSERT_TRUE(SmallString("text") < SmallString("texta"));
    ASSERT_FALSE(SmallString("texta") < SmallString("text"));
    ASSERT_FALSE(SmallString("text") < SmallString("some"));
    ASSERT_FALSE(SmallString("text") < SmallString("text"));
}

TEST(SmallString, smaller_operator_with_string_view_right)
{
    ASSERT_TRUE(SmallString() < SmallStringView("text"));
    ASSERT_TRUE(SmallString("some") < SmallStringView("text"));
    ASSERT_TRUE(SmallString("text") < SmallStringView("texta"));
    ASSERT_FALSE(SmallString("texta") < SmallStringView("text"));
    ASSERT_FALSE(SmallString("text") < SmallStringView("some"));
    ASSERT_FALSE(SmallString("text") < SmallStringView("text"));
}

TEST(SmallString, smaller_operator_with_string_view_left)
{
    ASSERT_TRUE(SmallStringView("") < SmallString("text"));
    ASSERT_TRUE(SmallStringView("some") < SmallString("text"));
    ASSERT_TRUE(SmallStringView("text") < SmallString("texta"));
    ASSERT_FALSE(SmallStringView("texta") < SmallString("text"));
    ASSERT_FALSE(SmallStringView("text") < SmallString("some"));
    ASSERT_FALSE(SmallStringView("text") < SmallString("text"));
}

TEST(SmallString, smaller_operator_for_difference_class_sizes)
{
    ASSERT_TRUE(SmallString() < PathString("text"));
    ASSERT_TRUE(SmallString("some") < PathString("text"));
    ASSERT_TRUE(SmallString("text") < PathString("texta"));
    ASSERT_FALSE(SmallString("texta") < PathString("text"));
    ASSERT_FALSE(SmallString("text") < PathString("some"));
    ASSERT_FALSE(SmallString("text") < PathString("text"));
}

TEST(SmallString, is_empty)
{
    ASSERT_FALSE(SmallString("text").isEmpty());
    ASSERT_TRUE(SmallString("").isEmpty());
    ASSERT_TRUE(SmallString().isEmpty());
}

TEST(SmallString, string_view_is_empty)
{
    ASSERT_FALSE(SmallStringView("text").isEmpty());
    ASSERT_TRUE(SmallStringView("").isEmpty());
}

TEST(SmallString, string_view_empty)
{
    ASSERT_FALSE(SmallStringView("text").empty());
    ASSERT_TRUE(SmallStringView("").empty());
}

TEST(SmallString, has_content)
{
    ASSERT_TRUE(SmallString("text").hasContent());
    ASSERT_FALSE(SmallString("").hasContent());
    ASSERT_FALSE(SmallString().hasContent());
}

TEST(SmallString, clear)
{
    SmallString text("text");

    text.clear();

    ASSERT_TRUE(text.isEmpty());
}

TEST(SmallString, no_occurrences_for_empty_text)
{
    SmallString text;

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 0);
}

TEST(SmallString, no_occurrences_in_text)
{
    SmallString text("here is some text, here is some text, here is some text");

    auto occurrences = text.countOccurrence("texts");

    ASSERT_THAT(occurrences, 0);
}

TEST(SmallString, some_occurrences)
{
    SmallString text("here is some text, here is some text, here is some text");

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 3);
}

TEST(SmallString, some_more_occurrences)
{
    SmallString text("texttexttext");

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 3);
}

TEST(SmallString, replace_with_character)
{
    SmallString text("here is some text, here is some text, here is some text");

    text.replace('s', 'x');

    ASSERT_THAT(text, SmallString("here ix xome text, here ix xome text, here ix xome text"));
}

TEST(SmallString, replace_with_equal_sized_text)
{
    SmallString text("here is some text");

    text.replace("some", "much");

    ASSERT_THAT(text, SmallString("here is much text"));
}

TEST(SmallString, replace_with_equal_sized_text_on_empty_text)
{
    SmallString text;

    text.replace("some", "much");

    ASSERT_THAT(text, SmallString());
}

TEST(SmallString, replace_with_shorter_text)
{
    SmallString text("here is some text");

    text.replace("some", "any");

    ASSERT_THAT(text, SmallString("here is any text"));
}

TEST(SmallString, replace_with_shorter_text_on_empty_text)
{
    SmallString text;

    text.replace("some", "any");

    ASSERT_THAT(text, SmallString());
}

TEST(SmallString, replace_with_longer_text)
{
    SmallString text("here is some text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text"));
}

TEST(SmallString, replace_with_longer_text_on_empty_text)
{
    SmallString text;

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString());
}

TEST(SmallString, replace_short_small_string_with_longer_text)
{
    SmallString text = SmallString::fromUtf8("here is some text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text"));
}

TEST(SmallString, replace_long_small_string_with_longer_text)
{
    SmallString text = SmallString::fromUtf8("some very very very very very very very very very very very long string");

    text.replace("long", "much much much much much much much much much much much much much much much much much much more");

    ASSERT_THAT(text, "some very very very very very very very very very very very much much much much much much much much much much much much much much much much much much more string");
}

TEST(SmallString, multiple_replace_small_string_with_longer_text)
{
    SmallString text = SmallString("here is some text with some longer text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text with much more longer text"));
}

TEST(SmallString, multiple_replace_small_string_with_shorter_text)
{
    SmallString text = SmallString("here is some text with some longer text");

    text.replace("some", "a");

    ASSERT_THAT(text, SmallString("here is a text with a longer text"));
}

TEST(SmallString, dont_replace_replaced_text)
{
    SmallString text("here is some foo text");

    text.replace("foo", "foofoo");

    ASSERT_THAT(text, SmallString("here is some foofoo text"));
}

TEST(SmallString, dont_reserve_if_nothing_is_replaced_for_longer_replacement_text)
{
    SmallString text("here is some text with some longer text");

    text.replace("bar", "foofoo");

    ASSERT_TRUE(text.isReadOnlyReference());
}

TEST(SmallString, dont_reserve_if_nothing_is_replaced_for_shorter_replacement_text)
{
    SmallString text("here is some text with some longer text");

    text.replace("foofoo", "bar");

    ASSERT_TRUE(text.isReadOnlyReference());
}

TEST(SmallString, starts_with)
{
    SmallString text("$column");

    ASSERT_FALSE(text.startsWith("$columnxxx"));
    ASSERT_TRUE(text.startsWith("$column"));
    ASSERT_TRUE(text.startsWith("$col"));
    ASSERT_FALSE(text.startsWith("col"));
    ASSERT_TRUE(text.startsWith('$'));
    ASSERT_FALSE(text.startsWith('@'));
}

TEST(SmallString, starts_with_string_view)
{
    SmallStringView text("$column");

    ASSERT_FALSE(text.startsWith("$columnxxx"));
    ASSERT_TRUE(text.startsWith("$column"));
    ASSERT_TRUE(text.startsWith("$col"));
    ASSERT_FALSE(text.startsWith("col"));
    ASSERT_TRUE(text.startsWith('$'));
    ASSERT_FALSE(text.startsWith('@'));
}

TEST(SmallString, ends_with)
{
    SmallString text("/my/path");

    ASSERT_TRUE(text.endsWith("/my/path"));
    ASSERT_TRUE(text.endsWith("path"));
    ASSERT_FALSE(text.endsWith("paths"));
    ASSERT_TRUE(text.endsWith('h'));
    ASSERT_FALSE(text.endsWith('x'));
}

TEST(SmallString, ends_with_string_view)
{
    SmallStringView text("/my/path");

    ASSERT_TRUE(text.endsWith("/my/path"));
    ASSERT_TRUE(text.endsWith("path"));
    ASSERT_FALSE(text.endsWith("paths"));
}

TEST(SmallString, ends_with_small_string)
{
    SmallString text("/my/path");

    ASSERT_TRUE(text.endsWith(SmallString("path")));
    ASSERT_TRUE(text.endsWith('h'));
}

TEST(SmallString, reserve_smaller_than_short_string_capacity)
{
    SmallString text("text");

    text.reserve(2);

    ASSERT_THAT(text.capacity(), 31);
}

TEST(SmallString, reserve_smaller_than_short_string_capacity_is_short_string)
{
    SmallString text("text");

    text.reserve(2);

    ASSERT_TRUE(text.isShortString());
}

TEST(SmallString, reserve_smaller_than_reference)
{
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(35);

    ASSERT_THAT(text.capacity(), 71);
}

TEST(SmallString, reserve_bigger_than_short_string_capacity)
{
    SmallString text("text");

    text.reserve(10);

    ASSERT_THAT(text.capacity(), 31);
}

TEST(SmallString, reserve_bigger_than_reference)
{
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(35);

    ASSERT_THAT(text.capacity(), 71);
}

TEST(SmallString, reserve_much_bigger_than_short_string_capacity)
{
    SmallString text("text");

    text.reserve(100);

    ASSERT_THAT(text.capacity(), 100);
}

TEST(SmallString, text_is_copied_after_reserve_from_short_to_long_string)
{
    SmallString text("text");

    text.reserve(100);

    ASSERT_THAT(text, "text");
}

TEST(SmallString, text_is_copied_after_reserve_reference_to_long_string)
{
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(100);

    ASSERT_THAT(text, "some very very very very very very very very very very very long string");
}

TEST(SmallString, reserve_smaller_than_short_small_string)
{
    SmallString text = SmallString::fromUtf8("text");

    text.reserve(10);

    ASSERT_THAT(text.capacity(), 31);
}

TEST(SmallString, reserve_bigger_than_short_small_string)
{
    SmallString text = SmallString::fromUtf8("text");

    text.reserve(100);

    ASSERT_THAT(text.capacity(), 100);
}

TEST(SmallString, reserve_bigger_than_long_small_string)
{
    SmallString text = SmallString::fromUtf8("some very very very very very very very very very very very long string");

    text.reserve(100);

    ASSERT_THAT(text.capacity(), 100);
}

TEST(SmallString, optimal_heap_cache_line_for_size)
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

TEST(SmallString, optimal_capacity_for_size)
{
    SmallString text;

    ASSERT_THAT(text.optimalCapacity(0), 0);
    ASSERT_THAT(text.optimalCapacity(31), 31);
    ASSERT_THAT(text.optimalCapacity(32), 64);
    ASSERT_THAT(text.optimalCapacity(64), 64);
    ASSERT_THAT(text.optimalCapacity(65), 128);
    ASSERT_THAT(text.optimalCapacity(129), 192);
}

TEST(SmallString, data_stream_data)
{
    SmallString inputText("foo");
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::ReadWrite);

    out << inputText;

    ASSERT_TRUE(byteArray.endsWith("foo"));
}

TEST(SmallString, read_data_stream_size)
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

TEST(SmallString, read_data_stream_data)
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

TEST(SmallString, short_small_string_copy_constuctor)
{
    SmallString text("text");

    auto copy = text;

    ASSERT_THAT(copy, text);
}

TEST(SmallString, long_small_string_copy_constuctor)
{
    SmallString text("this is a very very very very long text");

    auto copy = text;

    ASSERT_THAT(copy, text);
}

TEST(SmallString, short_small_string_move_constuctor)
{
    SmallString text("text");

    auto copy = std::move(text);

    ASSERT_THAT(copy, SmallString("text"));
}

TEST(SmallString, long_small_string_move_constuctor)
{
    SmallString text("this is a very very very very long text");

    auto copy = std::move(text);

    ASSERT_THAT(copy, SmallString("this is a very very very very long text"));
}

TEST(SmallString, short_path_string_move_constuctor)
{
    PathString text("text");

    auto copy = std::move(text);

    ASSERT_THAT(copy, SmallString("text"));
}

TEST(SmallString, long_path_string_move_constuctor)
{
    PathString text(
        "this is a very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very long text");

    auto copy = std::move(text);

    ASSERT_THAT(
        copy,
        PathString(
            "this is a very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very long "
            "text"));
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wpragmas")
QT_WARNING_DISABLE_GCC("-Wself-move")
QT_WARNING_DISABLE_CLANG("-Wself-move")

TEST(SmallString, short_small_string_move_constuctor_to_self)
{
    SmallString text("text");

    text = std::move(text);

    ASSERT_THAT(text, SmallString("text"));
}

TEST(SmallString, long_small_string_move_constuctor_to_self)
{
    SmallString text("this is a very very very very long text");

    text = std::move(text);

    ASSERT_THAT(text, SmallString("this is a very very very very long text"));
}

TEST(SmallString, short_path_string_move_constuctor_to_self)
{
    PathString text("text");

    text = std::move(text);

    ASSERT_THAT(text, SmallString("text"));
}

TEST(SmallString, long_path_string_move_constuctor_to_self)
{
    PathString text(
        "this is a very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very long text");

    text = std::move(text);

    ASSERT_THAT(
        text,
        PathString(
            "this is a very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very long "
            "text"));
}

QT_WARNING_POP

TEST(SmallString, short_small_string_copy_assignment)
{
    SmallString text("text");
    SmallString copy("more text");

    copy = text;

    ASSERT_THAT(copy, text);
}

TEST(SmallString, long_small_string_copy_assignment)
{
    SmallString text("this is a very very very very long text");
    SmallString copy("more text");

    copy = text;

    ASSERT_THAT(copy, text);
}

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

TEST(SmallString, long_small_string_copy_self_assignment)
{
    SmallString text("this is a very very very very long text");

    text = text;

    ASSERT_THAT(text, SmallString("this is a very very very very long text"));
}

#if __clang__
#pragma clang diagnostic pop
#endif

TEST(SmallString, short_small_string_move_assignment)
{
    SmallString text("text");
    SmallString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(copy, SmallString("text"));
}

TEST(SmallString, long_small_string_move_assignment)
{
    SmallString text("this is a very very very very long text");
    SmallString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(copy, SmallString("this is a very very very very long text"));
}

TEST(SmallString, short_path_string_move_assignment)
{
    PathString text("text");
    PathString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(copy, SmallString("text"));
}

TEST(SmallString, long_path_string_move_assignment)
{
    PathString text(
        "this is a very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very long text");
    PathString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(
        copy,
        PathString(
            "this is a very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very long "
            "text"));
}

TEST(SmallString, short_small_string_take)
{
    SmallString text("text");
    SmallString copy("more text");

    copy = text.take();

    ASSERT_THAT(text, IsEmpty());
    ASSERT_THAT(copy, SmallString("text"));
}

TEST(SmallString, long_small_string_take)
{
    SmallString text("this is a very very very very long text");
    SmallString copy("more text");

    copy = text.take();

    ASSERT_THAT(text, IsEmpty());
    ASSERT_THAT(copy, SmallString("this is a very very very very long text"));
}

TEST(SmallString, replace_by_position_shorter_with_longer_text)
{
    SmallString text("this is a very very very very long text");

    text.replace(8, 1, "some");

    ASSERT_THAT(text, SmallString("this is some very very very very long text"));
}

TEST(SmallString, replace_by_position_longer_with_short_text)
{
    SmallString text("this is some very very very very long text");

    text.replace(8, 4, "a");

    ASSERT_THAT(text, SmallString("this is a very very very very long text"));
}

TEST(SmallString, replace_by_position_equal_sized_texts)
{
    SmallString text("this is very very very very very long text");

    text.replace(33, 4, "much");

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TEST(SmallString, compare_text_with_different_line_endings)
{
    SmallString unixText("some \ntext");
    SmallString windowsText("some \n\rtext");

    auto convertedText = windowsText.toCarriageReturnsStripped();

    ASSERT_THAT(unixText, convertedText);
}

TEST(SmallString, const_subscript_operator)
{
    const SmallString text{"some text"};

    auto &&sign = text[5];

    ASSERT_THAT(sign, 't');
}

TEST(SmallString, non_const_subscript_operator)
{
    SmallString text{"some text"};

    auto &&sign = text[5];

    ASSERT_THAT(sign, 't');
}

TEST(SmallString, manipulate_const_subscript_operator)
{
    const SmallString text{"some text"};
    auto &&sign = text[5];

    sign = 'q';

    ASSERT_THAT(text, SmallString{"some text"});
}

TEST(SmallString, manipulate_non_const_subscript_operator)
{
    char rawText[] = "some text";
    SmallString text{rawText};
    auto &&sign = text[5];

    sign = 'q';

    ASSERT_THAT(text, SmallString{"some qext"});
}

TEST(SmallString, empty_initializer_list_content)
{
    SmallString text = {};

    ASSERT_THAT(text, SmallString());
}

TEST(SmallString, empty_initializer_list_size)
{
    SmallString text = {};

    ASSERT_THAT(text, SizeIs(0));
}

TEST(SmallString, initializer_list_content)
{
    auto text = SmallString::join({"some", " ", "text"});

    ASSERT_THAT(text, SmallString("some text"));
}

TEST(SmallString, initializer_list_size)
{
    auto text = SmallString::join({"some", " ", "text"});

    ASSERT_THAT(text, SizeIs(9));
}

TEST(SmallString, number_to_string)
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

TEST(SmallString, string_view_plus_operator)
{
    SmallStringView text = "text";

    auto result = text + " and more text";

    ASSERT_THAT(result, "text and more text");
}

TEST(SmallString, string_view_plus_operator_reverse_order)
{
    SmallStringView text = " and more text";

    auto result = "text" + text;

    ASSERT_THAT(result, "text and more text");
}

TEST(SmallString, string_plus_operator)
{
    SmallString text = "text";

    auto result = text + " and more text";

    ASSERT_THAT(result, "text and more text");
}

TEST(SmallString, string_plus_operator_reverse_order)
{
    SmallString text = " and more text";

    auto result = "text" + text;

    ASSERT_THAT(result, "text and more text");
}

TEST(SmallString, short_string_capacity)
{
    ASSERT_THAT(SmallString().shortStringCapacity(), 31);
    ASSERT_THAT(PathString().shortStringCapacity(), 190);
}

TEST(SmallString, to_view)
{
    SmallString text = "text";

    auto view = text.toStringView();

    ASSERT_THAT(view, "text");

}

TEST(SmallString, compare)
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
