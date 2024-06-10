// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <QString>

#include <utils/smallstring.h>
#include <utils/smallstringio.h>
#include <utils/smallstringvector.h>

#include <random>

namespace {

using Utils::SmallStringLiteral;
using Utils::SmallStringView;

static_assert(32 == sizeof(Utils::BasicSmallString<31>));
static_assert(80 == sizeof(Utils::BasicSmallString<64>));
static_assert(192 == sizeof(Utils::BasicSmallString<176>));

static_assert(16 == alignof(Utils::BasicSmallString<31>));
static_assert(16 == alignof(Utils::BasicSmallString<64>));
static_assert(16 == alignof(Utils::BasicSmallString<192>));

template<typename StringType>
class SmallString : public testing::Test
{
public:
    using String = StringType;
};

using StringTypes = ::testing::Types<Utils::BasicSmallString<31>, Utils::BasicSmallString<64>, Utils::PathString>;
TYPED_TEST_SUITE(SmallString, StringTypes);

TYPED_TEST(SmallString, basic_string_equal)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    ASSERT_THAT(text, Eq(SmallString("text")));
}

TYPED_TEST(SmallString, basic_small_string_unequal)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    ASSERT_THAT(text, Ne(SmallString("other text")));
}

TYPED_TEST(SmallString, null_small_string_is_equal_to_empty_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString defaultString;

    ASSERT_THAT(defaultString, Eq(SmallString("")));
}

TYPED_TEST(SmallString, short_small_string_literal_is_short_small_string)
{
    // constexpr
    SmallStringLiteral shortText("short string");

    ASSERT_TRUE(shortText.isShortString());
}

TYPED_TEST(SmallString, short_small_string_is_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString shortText("short string");

    ASSERT_TRUE(shortText.isShortString());
}

TYPED_TEST(SmallString, create_from_c_string_iterators)
{
    using SmallString = typename TestFixture::String;
    char sourceText[] = "this is very very very very very much text";

    SmallString text(sourceText, &sourceText[sizeof(sourceText) - 1]);

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TYPED_TEST(SmallString, create_from_q_byte_array_iterators)
{
    using SmallString = typename TestFixture::String;
    QByteArray sourceText = "this is very very very very very much text";

    SmallString text(sourceText.begin(), sourceText.end());

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TYPED_TEST(SmallString, create_from_small_string_iterators)
{
    using SmallString = typename TestFixture::String;
    SmallString sourceText = "this is very very very very very much text";

    SmallString text(sourceText.begin(), sourceText.end());

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TYPED_TEST(SmallString, create_from_string_view)
{
    using SmallString = typename TestFixture::String;
    SmallStringView sourceText = "this is very very very very very much text";

    SmallString text(sourceText);

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TYPED_TEST(SmallString, short_small_string_is_reference)
{
    Utils::SmallString longText("very very very very very long text");

    ASSERT_TRUE(longText.isReadOnlyReference());
}

TYPED_TEST(SmallString, small_string_contructor_is_not_reference)
{
    using SmallString = typename TestFixture::String;
    const char *shortCSmallString = "short string";
    auto shortText = SmallString(shortCSmallString);

    ASSERT_TRUE(shortText.isShortString());
}

TYPED_TEST(SmallString, short_small_string_is_not_reference)
{
    using SmallString = typename TestFixture::String;
    const char *shortCSmallString = "short string";
    auto shortText = SmallString::fromUtf8(shortCSmallString);

    ASSERT_FALSE(shortText.isReadOnlyReference());
}

TYPED_TEST(SmallString, long_small_string_construtor_is_allocated)
{
    const char *longCSmallString = "very very very very very long text";
    auto longText = Utils::SmallString(longCSmallString);

    ASSERT_TRUE(longText.hasAllocatedMemory());
}

TYPED_TEST(SmallString, maximum_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString maximumShortText("very very very very short text", 30);

    ASSERT_THAT(maximumShortText, StrEq("very very very very short text"));
}

TYPED_TEST(SmallString, long_const_expression_small_string_is_reference)
{
    using SmallString = typename TestFixture::String;
    SmallString longText(
        "very very very very very very very very very very very long string very very very very "
        "very very very very very very very long string very very very very very very very very "
        "very very very long string very very very very very very very very very very very long "
        "string very very very very very very very very very very very long string very very very "
        "very very very very very very very very long string very very very very very very very "
        "very very very very long string");

    ASSERT_TRUE(longText.isReadOnlyReference());
}

TYPED_TEST(SmallString, copy_short_const_expression_small_string_is_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString shortText("short string");

    auto shortTextCopy = shortText;

    ASSERT_TRUE(shortTextCopy.isShortString());
}

TYPED_TEST(SmallString, copy_long_const_expression_small_string_is_long_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString longText(
        "very very very very very very very very very very very long string very very very very "
        "very very very very very very very long string very very very very very very very very "
        "very very very long string very very very very very very very very very very very long "
        "string very very very very very very very very very very very long string very very very "
        "very very very very very very very very long string very very very very very very very "
        "very very very very long string");

    auto longTextCopy = longText;

    ASSERT_FALSE(longTextCopy.isShortString());
}

TYPED_TEST(SmallString, short_path_string_is_short_string)
{
    const char *rawText = "very very very very very very very very very very very long path which fits in the short memory";

    Utils::PathString text(rawText);

    ASSERT_TRUE(text.isShortString());
}

TYPED_TEST(SmallString, small_string_from_character_array_is_reference)
{
    using SmallString = typename TestFixture::String;
    const char longCString[]
        = "very very very very very very very very very very very long string very very very very "
          "very very very very very very very long string very very very very very very very very "
          "very very very long string very very very very very very very very very very very long "
          "string very very very very very very very very very very very long string very very "
          "very very very very very very very very very long string very very very very very very "
          "very very very very very long string";

    SmallString longString(longCString);

    ASSERT_TRUE(longString.isReadOnlyReference());
}

TYPED_TEST(SmallString, small_string_from_character_pointer_is_not_reference)
{
    using SmallString = typename TestFixture::String;
    const char *longCString = "very very very very very very very very very very very long string";

    SmallString longString = SmallString::fromUtf8(longCString);

    ASSERT_FALSE(longString.isReadOnlyReference());
}

TYPED_TEST(SmallString, copy_string_from_reference)
{
    using SmallString = typename TestFixture::String;
    SmallString longText(
        "very very very very very very very very very very very long string very very very very "
        "very very very very very very very long string very very very very very very very very "
        "very very very long string very very very very very very very very very very very long "
        "string very very very very very very very very very very very long string very very "
        "very very very very very very very very very long string very very very very very very "
        "very very very very very long string");
    SmallString longTextCopy;

    longTextCopy = longText;

    ASSERT_TRUE(longTextCopy.isReadOnlyReference());
}

TYPED_TEST(SmallString, small_string_literal_short_small_string_data_access)
{
    SmallStringLiteral literalText(
        "very very very very very very very very very very very long string");

    ASSERT_THAT(literalText, StrEq("very very very very very very very very very very very long string"));
}

TYPED_TEST(SmallString, small_string_literal_long_small_string_data_access)
{
    SmallStringLiteral literalText("short string");

    ASSERT_THAT(literalText, StrEq("short string"));
}

TYPED_TEST(SmallString, reference_data_access)
{
    using SmallString = typename TestFixture::String;

    SmallString literalText("short string");

    ASSERT_THAT(literalText, StrEq("short string"));
}

TYPED_TEST(SmallString, short_data_access)
{
    using SmallString = typename TestFixture::String;
    const char *shortCString = "short string";
    auto shortText = SmallString::fromUtf8(shortCString);

    ASSERT_THAT(shortText, StrEq("short string"));
}

TYPED_TEST(SmallString, long_data_access)
{
    using SmallString = typename TestFixture::String;
    const char *longCString = "very very very very very very very very very very very long string";
    auto longText = SmallString::fromUtf8(longCString);

    ASSERT_THAT(longText, StrEq(longCString));
}

TYPED_TEST(SmallString, small_string_begin_is_equal_end_for_empty_small_string)
{
    using SmallString = typename TestFixture::String;

    SmallString text;

    ASSERT_THAT(text.begin(), Eq(text.end()));
}

TYPED_TEST(SmallString, small_string_begin_is_not_equal_end_for_non_empty_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("x");

    ASSERT_THAT(text.begin(), Ne(text.end()));
}

TYPED_TEST(SmallString, small_string_begin_plus_one_is_equal_end_for_small_string_width_size_one)
{
    using SmallString = typename TestFixture::String;
    SmallString text("x");

    auto beginPlusOne = text.begin() + std::size_t(1);

    ASSERT_THAT(beginPlusOne, Eq(text.end()));
}

TYPED_TEST(SmallString, small_string_r_begin_is_equal_r_end_for_empty_small_string)
{
    using SmallString = typename TestFixture::String;

    SmallString text;

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TYPED_TEST(SmallString, small_string_r_begin_is_not_equal_r_end_for_non_empty_small_string)
{
    using SmallString = typename TestFixture::String;

    SmallString text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TYPED_TEST(SmallString, small_string_r_begin_plus_one_is_equal_r_end_for_small_string_width_size_one)
{
    using SmallString = typename TestFixture::String;
    SmallString text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TYPED_TEST(SmallString, small_string_const_r_begin_is_equal_r_end_for_empty_small_string)
{
    using SmallString = typename TestFixture::String;
    const SmallString text;

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TYPED_TEST(SmallString, small_string_const_r_begin_is_not_equal_r_end_for_non_empty_small_string)
{
    using SmallString = typename TestFixture::String;
    const SmallString text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TYPED_TEST(SmallString,
           small_string_small_string_const_r_begin_plus_one_is_equal_r_end_for_small_string_width_size_one)
{
    using SmallString = typename TestFixture::String;
    const SmallString text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TYPED_TEST(SmallString, small_string_distance_between_begin_and_end_is_zero_for_empty_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 0);
}

TYPED_TEST(SmallString, small_string_distance_between_begin_and_end_is_one_for_one_sign)
{
    using SmallString = typename TestFixture::String;
    SmallString text("x");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 1);
}

TYPED_TEST(SmallString, small_string_distance_between_r_begin_and_r_end_is_zero_for_empty_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TYPED_TEST(SmallString, small_string_distance_between_r_begin_and_r_end_is_one_for_one_sign)
{
    using SmallString = typename TestFixture::String;
    SmallString text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TYPED_TEST(SmallString, small_string_begin_points_to_x)
{
    using SmallString = typename TestFixture::String;
    SmallString text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, small_string_r_begin_points_to_x)
{
    using SmallString = typename TestFixture::String;
    SmallString text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, const_small_string_begin_points_to_x)
{
    using SmallString = typename TestFixture::String;
    const SmallString text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, const_small_string_r_begin_points_to_x)
{
    using SmallString = typename TestFixture::String;
    const SmallString text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, small_string_view_begin_is_equal_end_for_empty_small_string)
{
    SmallStringView text{""};

    ASSERT_THAT(text.begin(), Eq(text.end()));
}

TYPED_TEST(SmallString, small_string_view_begin_is_not_equal_end_for_non_empty_small_string)
{
    SmallStringView text("x");

    ASSERT_THAT(text.begin(), Ne(text.end()));
}

TYPED_TEST(SmallString, small_string_view_begin_plus_one_is_equal_end_for_small_string_width_size_one)
{
    SmallStringView text("x");

    auto beginPlusOne = text.begin() + std::size_t(1);

    ASSERT_THAT(beginPlusOne, Eq(text.end()));
}

TYPED_TEST(SmallString, small_string_view_r_begin_is_equal_r_end_for_empty_small_string)
{
    SmallStringView text{""};

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TYPED_TEST(SmallString, small_string_view_r_begin_is_not_equal_r_end_for_non_empty_small_string)
{
    SmallStringView text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TYPED_TEST(SmallString,
           small_string_view_r_begin_plus_one_is_equal_r_end_for_small_string_width_size_one)
{
    SmallStringView text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TYPED_TEST(SmallString, small_string_view_const_r_begin_is_equal_r_end_for_empty_small_string)
{
    const SmallStringView text{""};

    ASSERT_THAT(text.rbegin(), Eq(text.rend()));
}

TYPED_TEST(SmallString, small_string_view_const_r_begin_is_not_equal_r_end_for_non_empty_small_string)
{
    const SmallStringView text("x");

    ASSERT_THAT(text.rbegin(), Ne(text.rend()));
}

TYPED_TEST(SmallString,
           small_string_view_const_r_begin_plus_one_is_equal_r_end_for_small_string_width_size_one)
{
    const SmallStringView text("x");

    auto beginPlusOne = text.rbegin() + 1l;

    ASSERT_THAT(beginPlusOne, Eq(text.rend()));
}

TYPED_TEST(SmallString, small_string_view_distance_between_begin_and_end_is_zero_for_empty_text)
{
    SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TYPED_TEST(SmallString, small_string_view_distance_between_begin_and_end_is_one_for_one_sign)
{
    SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TYPED_TEST(SmallString, small_string_view_distance_between_r_begin_and_r_end_is_zero_for_empty_text)
{
    SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TYPED_TEST(SmallString, small_string_view_distance_between_r_begin_and_r_end_is_one_for_one_sign)
{
    SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TYPED_TEST(SmallString, const_small_string_view_distance_between_begin_and_end_is_zero_for_empty_text)
{
    const SmallStringView text("");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 0);
}

TYPED_TEST(SmallString, const_small_string_view_distance_between_begin_and_end_is_one_for_one_sign)
{
    const SmallStringView text("x");

    auto distance = std::distance(text.begin(), text.end());

    ASSERT_THAT(distance, 1);
}

TYPED_TEST(SmallString,
           const_small_string_view_distance_between_r_begin_and_r_end_is_zero_for_empty_text)
{
    const SmallStringView text("");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 0);
}

TYPED_TEST(SmallString, const_small_string_view_distance_between_r_begin_and_r_end_is_one_for_one_sign)
{
    const SmallStringView text("x");

    auto distance = std::distance(text.rbegin(), text.rend());

    ASSERT_THAT(distance, 1);
}

TYPED_TEST(SmallString, small_string_view_begin_points_to_x)
{
    SmallStringView text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, small_string_view_r_begin_points_to_x)
{
    SmallStringView text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, const_small_string_view_begin_points_to_x)
{
    const SmallStringView text("x");

    auto sign = *text.begin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, const_small_string_view_r_begin_points_to_x)
{
    const SmallStringView text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, small_string_literal_view_r_begin_points_to_x)
{
    SmallStringLiteral text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, const_small_string_literal_view_r_begin_points_to_x)
{
    const SmallStringLiteral text("x");

    auto sign = *text.rbegin();

    ASSERT_THAT(sign, 'x');
}

TYPED_TEST(SmallString, constructor_standard_string)
{
    using SmallString = typename TestFixture::String;
    std::string stdStringText = "short string";

    auto text = SmallString(stdStringText);

    ASSERT_THAT(text, SmallString("short string"));
}

TYPED_TEST(SmallString, to_q_string)
{
    using SmallString = typename TestFixture::String;
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

TEST_P(FromQString, small_string_from_qstring)
{
    const QString qStringText = exampleString(size);

    auto text = Utils::SmallString(qStringText);

    ASSERT_THAT(text, qStringText.toStdString());
}

TEST_P(FromQString, path_string_from_qstring)
{
    const QString qStringText = exampleString(size);

    auto text = Utils::PathString(qStringText);

    ASSERT_THAT(text, qStringText.toStdString());
}

TYPED_TEST(SmallString, from_q_byte_array)
{
    using SmallString = typename TestFixture::String;
    QByteArray qByteArray = QByteArrayLiteral("short string");

    auto text = SmallString::fromQByteArray(qByteArray);

    ASSERT_THAT(text, SmallString("short string"));
}

TYPED_TEST(SmallString, mid_one_parameter)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    auto midString = text.mid(5);

    ASSERT_THAT(midString, Eq(SmallString("text")));
}

TYPED_TEST(SmallString, mid_two_parameter)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text and more");

    auto midString = text.mid(5, 4);

    ASSERT_THAT(midString, Eq(SmallString("text")));
}

TYPED_TEST(SmallString, small_string_view_mid_one_parameter)
{
    SmallStringView text("some text");

    auto midString = text.mid(5);

    ASSERT_THAT(midString, Eq(SmallStringView("text")));
}

TYPED_TEST(SmallString, small_string_view_mid_two_parameter)
{
    SmallStringView text("some text and more");

    auto midString = text.mid(5, 4);

    ASSERT_THAT(midString, Eq(SmallStringView("text")));
}

TYPED_TEST(SmallString, size_of_empty_stringl)
{
    using SmallString = typename TestFixture::String;
    SmallString emptyString;

    auto size = emptyString.size();

    ASSERT_THAT(size, 0);
}

TYPED_TEST(SmallString, size_short_small_string_literal)
{
    SmallStringLiteral shortText("text");

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TYPED_TEST(SmallString, size_long_small_string_literal)
{
    auto longText = SmallStringLiteral("very very very very very very very very very very very long string");

    auto size = longText.size();

    ASSERT_THAT(size, 66);
}

TYPED_TEST(SmallString, size_reference)
{
    using SmallString = typename TestFixture::String;
    SmallString shortText("text");

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TYPED_TEST(SmallString, size_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString shortText("text", 4);

    auto size = shortText.size();

    ASSERT_THAT(size, 4);
}

TYPED_TEST(SmallString, size_short_path_string)
{
    using SmallString = typename TestFixture::String;
    SmallString shortPath("very very very very very very very very very very very long path which fits in the short memory");

    auto size = shortPath.size();

    ASSERT_THAT(size, 95);
}

TYPED_TEST(SmallString, size_long_small_string)
{
    using SmallString = typename TestFixture::String;
    auto longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto size = longText.size();

    ASSERT_THAT(size, 66);
}

TYPED_TEST(SmallString, capacity_reference)
{
    Utils::SmallString shortText("very very very very very very very long string");

    auto capacity = shortText.capacity();

    ASSERT_THAT(capacity, 0);
}

TYPED_TEST(SmallString, capacity_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString shortText("text", 4);

    auto capacity = shortText.capacity();

    ASSERT_THAT(capacity, SmallString::Capacity);
}

TYPED_TEST(SmallString, capacity_long_small_string)
{
    using SmallString = typename TestFixture::String;
    auto longText = SmallString::fromUtf8("very very very very very very very very very very very long string");

    auto capacity = longText.capacity();

    ASSERT_THAT(capacity, Ge(66));
}

TYPED_TEST(SmallString, fits_not_in_capacity_because_null_small_string_is_a_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text;

    ASSERT_FALSE(text.fitsNotInCapacity(30));
}

TYPED_TEST(SmallString, fits_not_in_capacity_because_it_is_reference)
{
    Utils::SmallString text("very very very very very very very long string");

    ASSERT_TRUE(text.fitsNotInCapacity(1));
}

TYPED_TEST(SmallString, fits_in_short_small_string_capacity)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text", 4);

    ASSERT_FALSE(text.fitsNotInCapacity(SmallString::Capacity));
}

TYPED_TEST(SmallString, fits_in_not_short_small_string_capacity)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text", 4);

    ASSERT_TRUE(text.fitsNotInCapacity(SmallString::Capacity + 1));
}

TYPED_TEST(SmallString, fits_in_long_small_string_capacity)
{
    using SmallString = typename TestFixture::String;
    SmallString text = SmallString::fromUtf8("very very very very very very long string");

    ASSERT_FALSE(text.fitsNotInCapacity(33)) << text.capacity();
}

TYPED_TEST(SmallString, fits_not_in_small_string_capacity)
{
    using SmallString = typename TestFixture::String;
    SmallString text = SmallString::fromUtf8("very very very very long string");

    ASSERT_TRUE(text.fitsNotInCapacity(SmallString::Capacity + 1)) << text.capacity();
}

TYPED_TEST(SmallString, append_null_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text += SmallString();

    ASSERT_THAT(text, SmallString("text"));
}

TYPED_TEST(SmallString, append_null_q_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text += QString();

    ASSERT_THAT(text, SmallString("text"));
}

TYPED_TEST(SmallString, append_empty_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text += SmallString("");

    ASSERT_THAT(text, SmallString("text"));
}

TYPED_TEST(SmallString, append_empty_q_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text += QString("");

    ASSERT_THAT(text, SmallString("text"));
}

TYPED_TEST(SmallString, append_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some ");

    text += SmallString("text");

    ASSERT_THAT(text, SmallString("some text"));
}

TYPED_TEST(SmallString, append_short_q_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some ");

    text += QString("text");

    ASSERT_THAT(text, SmallString("some text"));
}

TYPED_TEST(SmallString, append_long_small_string_to_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some ");

    text += SmallString("very very very very very long string");

    ASSERT_THAT(text, SmallString("some very very very very very long string"));
}

TYPED_TEST(SmallString, append_long_q_string_to_short_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some ");

    text += QString("very very very very very long string");

    ASSERT_THAT(text, SmallString("some very very very very very long string"));
}

TYPED_TEST(SmallString, append_long_small_string)
{
    using SmallString = typename TestFixture::String;
    SmallString longText("some very very very very very very very very very very very long string");

    longText += SmallString(" text");

    ASSERT_THAT(longText, SmallString("some very very very very very very very very very very very long string text"));
}

TYPED_TEST(SmallString, append_long_q_string)
{
    using SmallString = typename TestFixture::String;
    SmallString longText("some very very very very very very very very very very very long string");

    longText += QString(" text");

    ASSERT_THAT(
        longText,
        SmallString(
            "some very very very very very very very very very very very long string text"));
}

TYPED_TEST(SmallString, append_initializer_list)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    text += {" and", " some", " other", " text"};

    ASSERT_THAT(text, Eq("some text and some other text"));
}

TYPED_TEST(SmallString, append_empty_initializer_list)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    text += {};

    ASSERT_THAT(text, Eq("some text"));
}

TYPED_TEST(SmallString, append_int)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    text += 123;

    ASSERT_THAT(text, Eq("some text123"));
}

TYPED_TEST(SmallString, append_float)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    text += 123.456;

    ASSERT_THAT(text, Eq("some text123.456"));
}

TYPED_TEST(SmallString, append_character)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    text += 'x';

    ASSERT_THAT(text, Eq("some textx"));
}

TYPED_TEST(SmallString, to_byte_array)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    ASSERT_THAT(text.toQByteArray(), QByteArrayLiteral("some text"));
}

TYPED_TEST(SmallString, contains)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    ASSERT_TRUE(text.contains(SmallString("text")));
    ASSERT_TRUE(text.contains("text"));
    ASSERT_TRUE(text.contains('x'));
}

TYPED_TEST(SmallString, not_contains)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some text");

    ASSERT_FALSE(text.contains(SmallString("textTwo")));
    ASSERT_FALSE(text.contains("foo"));
    ASSERT_FALSE(text.contains('q'));
}

TYPED_TEST(SmallString, equal_small_string_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString() == SmallString(""));
    ASSERT_FALSE(SmallString() == SmallString("text"));
    ASSERT_TRUE(SmallString("text") == SmallString("text"));
    ASSERT_FALSE(SmallString("text") == SmallString("text2"));
}

TYPED_TEST(SmallString, equal_small_string_operator_with_difference_class_sizes)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString() == Utils::PathString(""));
    ASSERT_FALSE(SmallString() == Utils::PathString("text"));
    ASSERT_TRUE(SmallString("text") == Utils::PathString("text"));
    ASSERT_FALSE(SmallString("text") == Utils::PathString("text2"));
}

TYPED_TEST(SmallString, equal_c_string_array_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString() == "");
    ASSERT_FALSE(SmallString() == "text");
    ASSERT_TRUE(SmallString("text") == "text");
    ASSERT_FALSE(SmallString("text") == "text2");
}

TYPED_TEST(SmallString, equal_c_string_pointer_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString("text") == std::string("text").data());
    ASSERT_FALSE(SmallString("text") == std::string("text2").data());
}

TYPED_TEST(SmallString, equal_small_string_view_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString("text") == SmallStringView("text"));
    ASSERT_FALSE(SmallString("text") == SmallStringView("text2"));
}

TYPED_TEST(SmallString, equal_small_string_views_operator)
{
    ASSERT_TRUE(SmallStringView("text") == SmallStringView("text"));
    ASSERT_FALSE(SmallStringView("text") == SmallStringView("text2"));
}

TYPED_TEST(SmallString, unequal_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_FALSE(SmallString("text") != SmallString("text"));
    ASSERT_TRUE(SmallString("text") != SmallString("text2"));
}

TYPED_TEST(SmallString, unequal_c_string_array_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_FALSE(SmallString("text") != "text");
    ASSERT_TRUE(SmallString("text") != "text2");
}

TYPED_TEST(SmallString, unequal_c_string_pointer_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_FALSE(SmallString("text") != std::string("text").data());
    ASSERT_TRUE(SmallString("text") != std::string("text2").data());
}

TYPED_TEST(SmallString, unequal_small_string_view_array_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_FALSE(SmallString("text") != SmallStringView("text"));
    ASSERT_TRUE(SmallString("text") != SmallStringView("text2"));
}

TYPED_TEST(SmallString, unequal_small_string_views_array_operator)
{
    ASSERT_FALSE(SmallStringView("text") != SmallStringView("text"));
    ASSERT_TRUE(SmallStringView("text") != SmallStringView("text2"));
}

TYPED_TEST(SmallString, smaller_operator)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString() < SmallString("text"));
    ASSERT_TRUE(SmallString("some") < SmallString("text"));
    ASSERT_TRUE(SmallString("text") < SmallString("texta"));
    ASSERT_FALSE(SmallString("texta") < SmallString("text"));
    ASSERT_FALSE(SmallString("text") < SmallString("some"));
    ASSERT_FALSE(SmallString("text") < SmallString("text"));
}

TYPED_TEST(SmallString, smaller_operator_with_string_view_right)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString() < SmallStringView("text"));
    ASSERT_TRUE(SmallString("some") < SmallStringView("text"));
    ASSERT_TRUE(SmallString("text") < SmallStringView("texta"));
    ASSERT_FALSE(SmallString("texta") < SmallStringView("text"));
    ASSERT_FALSE(SmallString("text") < SmallStringView("some"));
    ASSERT_FALSE(SmallString("text") < SmallStringView("text"));
}

TYPED_TEST(SmallString, smaller_operator_with_string_view_left)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallStringView("") < SmallString("text"));
    ASSERT_TRUE(SmallStringView("some") < SmallString("text"));
    ASSERT_TRUE(SmallStringView("text") < SmallString("texta"));
    ASSERT_FALSE(SmallStringView("texta") < SmallString("text"));
    ASSERT_FALSE(SmallStringView("text") < SmallString("some"));
    ASSERT_FALSE(SmallStringView("text") < SmallString("text"));
}

TYPED_TEST(SmallString, smaller_operator_for_difference_class_sizes)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString() < Utils::PathString("text"));
    ASSERT_TRUE(SmallString("some") < Utils::PathString("text"));
    ASSERT_TRUE(SmallString("text") < Utils::PathString("texta"));
    ASSERT_FALSE(SmallString("texta") < Utils::PathString("text"));
    ASSERT_FALSE(SmallString("text") < Utils::PathString("some"));
    ASSERT_FALSE(SmallString("text") < Utils::PathString("text"));
}

TYPED_TEST(SmallString, is_empty)
{
    using SmallString = typename TestFixture::String;
    ASSERT_FALSE(SmallString("text").isEmpty());
    ASSERT_TRUE(SmallString("").isEmpty());
    ASSERT_TRUE(SmallString().isEmpty());
}

TYPED_TEST(SmallString, string_view_is_empty)
{
    ASSERT_FALSE(SmallStringView("text").isEmpty());
    ASSERT_TRUE(SmallStringView("").isEmpty());
}

TYPED_TEST(SmallString, string_view_empty)
{
    ASSERT_FALSE(SmallStringView("text").empty());
    ASSERT_TRUE(SmallStringView("").empty());
}

TYPED_TEST(SmallString, has_content)
{
    using SmallString = typename TestFixture::String;
    ASSERT_TRUE(SmallString("text").hasContent());
    ASSERT_FALSE(SmallString("").hasContent());
    ASSERT_FALSE(SmallString().hasContent());
}

TYPED_TEST(SmallString, clear)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text.clear();

    ASSERT_TRUE(text.isEmpty());
}

TYPED_TEST(SmallString, no_occurrences_for_empty_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text;

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 0);
}

TYPED_TEST(SmallString, no_occurrences_in_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("here is some text, here is some text, here is some text");

    auto occurrences = text.countOccurrence("texts");

    ASSERT_THAT(occurrences, 0);
}

TYPED_TEST(SmallString, some_occurrences)
{
    using SmallString = typename TestFixture::String;
    SmallString text("here is some text, here is some text, here is some text");

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 3);
}

TYPED_TEST(SmallString, some_more_occurrences)
{
    using SmallString = typename TestFixture::String;
    SmallString text("texttexttext");

    auto occurrences = text.countOccurrence("text");

    ASSERT_THAT(occurrences, 3);
}

TYPED_TEST(SmallString, replace_with_character)
{
    using SmallString = typename TestFixture::String;
    SmallString text("here is some text, here is some text, here is some text");

    text.replace('s', 'x');

    ASSERT_THAT(text, SmallString("here ix xome text, here ix xome text, here ix xome text"));
}

TYPED_TEST(SmallString, replace_with_equal_sized_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("here is some text");

    text.replace("some", "much");

    ASSERT_THAT(text, SmallString("here is much text"));
}

TYPED_TEST(SmallString, replace_with_equal_sized_text_on_empty_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text;

    text.replace("some", "much");

    ASSERT_THAT(text, SmallString());
}

TYPED_TEST(SmallString, replace_with_shorter_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("here is some text");

    text.replace("some", "any");

    ASSERT_THAT(text, SmallString("here is any text"));
}

TYPED_TEST(SmallString, replace_with_shorter_text_on_empty_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text;

    text.replace("some", "any");

    ASSERT_THAT(text, SmallString());
}

TYPED_TEST(SmallString, replace_with_longer_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("here is some text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text"));
}

TYPED_TEST(SmallString, replace_with_longer_text_on_empty_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text;

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString());
}

TYPED_TEST(SmallString, replace_short_small_string_with_longer_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text = SmallString::fromUtf8("here is some text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text"));
}

TYPED_TEST(SmallString, replace_long_small_string_with_longer_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text = SmallString::fromUtf8("some very very very very very very very very very very very long string");

    text.replace("long", "much much much much much much much much much much much much much much much much much much more");

    ASSERT_THAT(text, "some very very very very very very very very very very very much much much much much much much much much much much much much much much much much much more string");
}

TYPED_TEST(SmallString, multiple_replace_small_string_with_longer_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text = SmallString("here is some text with some longer text");

    text.replace("some", "much more");

    ASSERT_THAT(text, SmallString("here is much more text with much more longer text"));
}

TYPED_TEST(SmallString, multiple_replace_small_string_with_shorter_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text = SmallString("here is some text with some longer text");

    text.replace("some", "a");

    ASSERT_THAT(text, SmallString("here is a text with a longer text"));
}

TYPED_TEST(SmallString, dont_replace_replaced_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("here is some foo text");

    text.replace("foo", "foofoo");

    ASSERT_THAT(text, SmallString("here is some foofoo text"));
}

TYPED_TEST(SmallString, dont_reserve_if_nothing_is_replaced_for_longer_replacement_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text(
        "here is some text with some longer text here is some text with some longer texthere is "
        "some text with some longer texthere is some text with some longer texthere is some text "
        "with some longer texthere is some text with some longer texthere is some text with some "
        "longer texthere is some text with some longer texthere is some text with some longer "
        "texthere is some text with some longer text");

    text.replace("bar", "foofoo");

    ASSERT_TRUE(text.isReadOnlyReference());
}

TYPED_TEST(SmallString, dont_reserve_if_nothing_is_replaced_for_shorter_replacement_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text(
        "here is some text with some longer text here is some text with some longer texthere is "
        "some text with some longer texthere is some text with some longer texthere is some text "
        "with some longer texthere is some text with some longer texthere is some text with some "
        "longer texthere is some text with some longer texthere is some text with some longer "
        "texthere is some text with some longer text");

    text.replace("foofoo", "bar");

    ASSERT_TRUE(text.isReadOnlyReference());
}

TYPED_TEST(SmallString, starts_with)
{
    using SmallString = typename TestFixture::String;

    SmallString text("$column");

    ASSERT_FALSE(text.startsWith("$columnxxx"));
    ASSERT_TRUE(text.startsWith("$column"));
    ASSERT_TRUE(text.startsWith("$col"));
    ASSERT_FALSE(text.startsWith("col"));
    ASSERT_TRUE(text.startsWith('$'));
    ASSERT_FALSE(text.startsWith('@'));
}

TYPED_TEST(SmallString, starts_with_string_view)
{
    SmallStringView text("$column");

    ASSERT_FALSE(text.startsWith("$columnxxx"));
    ASSERT_TRUE(text.startsWith("$column"));
    ASSERT_TRUE(text.startsWith("$col"));
    ASSERT_FALSE(text.startsWith("col"));
    ASSERT_TRUE(text.startsWith('$'));
    ASSERT_FALSE(text.startsWith('@'));
}

TYPED_TEST(SmallString, starts_with_qstringview)
{
    using SmallString = typename TestFixture::String;
    using namespace Qt::StringLiterals;

    SmallString text("$column");

    ASSERT_FALSE(text.startsWith(u"$columnxxx"_s));
    ASSERT_TRUE(text.startsWith(u"$column"_s));
    ASSERT_TRUE(text.startsWith(u"$col"_s));
    ASSERT_FALSE(text.startsWith(u"col"_s));
    ASSERT_TRUE(text.startsWith(u"$"_s));
    ASSERT_FALSE(text.startsWith(u"@"_s));
}

TYPED_TEST(SmallString, ends_with)
{
    using SmallString = typename TestFixture::String;

    SmallString text("/my/path");

    ASSERT_TRUE(text.endsWith("/my/path"));
    ASSERT_TRUE(text.endsWith("path"));
    ASSERT_FALSE(text.endsWith("paths"));
    ASSERT_TRUE(text.endsWith('h'));
    ASSERT_FALSE(text.endsWith('x'));
}

TYPED_TEST(SmallString, ends_with_string_view)
{
    SmallStringView text("/my/path");

    ASSERT_TRUE(text.endsWith("/my/path"));
    ASSERT_TRUE(text.endsWith("path"));
    ASSERT_FALSE(text.endsWith("paths"));
}

TYPED_TEST(SmallString, ends_with_small_string)
{
    using SmallString = typename TestFixture::String;

    SmallString text("/my/path");

    ASSERT_TRUE(text.endsWith(SmallString("path")));
    ASSERT_TRUE(text.endsWith('h'));
}

TYPED_TEST(SmallString, reserve_smaller_than_short_string_capacity)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text.reserve(2);

    ASSERT_THAT(text.capacity(), SmallString::Capacity);
}

TYPED_TEST(SmallString, reserve_smaller_than_short_string_capacity_is_short_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text.reserve(2);

    ASSERT_TRUE(text.isShortString());
}

TYPED_TEST(SmallString, reserve_bigger_than_short_string_capacity)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(SmallString::Capacity + 1);

    ASSERT_THAT(text.capacity(), Ge(SmallString::Capacity + 1));
}

TYPED_TEST(SmallString, reserve_smaller_than_reference)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text.reserve(10);

    ASSERT_THAT(text.capacity(), SmallString::Capacity);
}

TYPED_TEST(SmallString, reserve_bigger_than_reference)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(SmallString::Capacity + 1);

    ASSERT_THAT(text.capacity(), Ge(SmallString::Capacity + 1));
}

TYPED_TEST(SmallString, text_is_copied_after_reserve_from_short_to_long_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text.reserve(100);

    ASSERT_THAT(text, "text");
}

TYPED_TEST(SmallString, text_is_copied_after_reserve_reference_to_long_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text("some very very very very very very very very very very very long string");

    text.reserve(100);

    ASSERT_THAT(text, "some very very very very very very very very very very very long string");
}

TYPED_TEST(SmallString, reserve_smaller_than_short_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text = SmallString::fromUtf8("text");

    text.reserve(10);

    ASSERT_THAT(text.capacity(), SmallString::Capacity);
}

TYPED_TEST(SmallString, reserve_bigger_than_short_string)
{
    using SmallString = typename TestFixture::String;
    SmallString text = SmallString::fromUtf8("text");

    text.reserve(SmallString::Capacity + 1);

    ASSERT_THAT(text.capacity(), Ge(SmallString::Capacity + 1));
}

TYPED_TEST(SmallString, optimal_heap_cache_line_for_size)
{
    using SmallString = typename TestFixture::String;

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

TYPED_TEST(SmallString, optimal_capacity_for_size)
{
    Utils::SmallString text;

    ASSERT_THAT(text.optimalCapacity(0), 0);
    ASSERT_THAT(text.optimalCapacity(31), 31);
    ASSERT_THAT(text.optimalCapacity(32), 64);
    ASSERT_THAT(text.optimalCapacity(64), 64);
    ASSERT_THAT(text.optimalCapacity(65), 128);
    ASSERT_THAT(text.optimalCapacity(129), 192);
}

TYPED_TEST(SmallString, data_stream_data)
{
    using SmallString = typename TestFixture::String;
    SmallString inputText("foo");
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::ReadWrite);

    out << inputText;

    ASSERT_TRUE(byteArray.endsWith("foo"));
}

TYPED_TEST(SmallString, read_data_stream_size)
{
    using SmallString = typename TestFixture::String;
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

TYPED_TEST(SmallString, read_data_stream_data)
{
    using SmallString = typename TestFixture::String;
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

TYPED_TEST(SmallString, short_small_string_copy_constuctor)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    auto copy = text;

    ASSERT_THAT(copy, text);
}

TYPED_TEST(SmallString, long_small_string_copy_constuctor)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is a very very very very long text");

    auto copy = text;

    ASSERT_THAT(copy, text);
}

TYPED_TEST(SmallString, short_small_string_move_constuctor)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    auto copy = std::move(text);

    ASSERT_THAT(copy, SmallString("text"));
}

TYPED_TEST(SmallString, long_small_string_move_constuctor)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is a very very very very long text");

    auto copy = std::move(text);

    ASSERT_THAT(copy, SmallString("this is a very very very very long text"));
}

TYPED_TEST(SmallString, short_path_string_move_constuctor)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    auto copy = std::move(text);

    ASSERT_THAT(copy, SmallString("text"));
}

TYPED_TEST(SmallString, long_path_string_move_constuctor)
{
    using SmallString = typename TestFixture::String;
    SmallString text(
        "this is a very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very long text");

    auto copy = std::move(text);

    ASSERT_THAT(
        copy,
        SmallString(
            "this is a very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very long "
            "text"));
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wpragmas")
QT_WARNING_DISABLE_GCC("-Wself-move")
QT_WARNING_DISABLE_CLANG("-Wself-move")

TYPED_TEST(SmallString, short_small_string_move_constuctor_to_self)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text = std::move(text);

    ASSERT_THAT(text, SmallString("text"));
}

TYPED_TEST(SmallString, long_small_string_move_constuctor_to_self)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is a very very very very long text");

    text = std::move(text);

    ASSERT_THAT(text, SmallString("this is a very very very very long text"));
}

TYPED_TEST(SmallString, short_path_string_move_constuctor_to_self)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");

    text = std::move(text);

    ASSERT_THAT(text, SmallString("text"));
}

TYPED_TEST(SmallString, long_path_string_move_constuctor_to_self)
{
    using SmallString = typename TestFixture::String;
    SmallString text(
        "this is a very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very long text");

    text = std::move(text);

    ASSERT_THAT(
        text,
        SmallString(
            "this is a very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very long "
            "text"));
}

QT_WARNING_POP

TYPED_TEST(SmallString, short_small_string_copy_assignment)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");
    SmallString copy("more text");

    copy = text;

    ASSERT_THAT(copy, text);
}

TYPED_TEST(SmallString, long_small_string_copy_assignment)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is a very very very very long text");
    SmallString copy("more text");

    copy = text;

    ASSERT_THAT(copy, text);
}

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

TYPED_TEST(SmallString, long_small_string_copy_self_assignment)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is a very very very very long text");

    text = text;

    ASSERT_THAT(text, SmallString("this is a very very very very long text"));
}

#if __clang__
#pragma clang diagnostic pop
#endif

TYPED_TEST(SmallString, short_small_string_move_assignment)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");
    SmallString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(copy, SmallString("text"));
}

TYPED_TEST(SmallString, long_small_string_move_assignment)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is a very very very very long text");
    SmallString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(copy, SmallString("this is a very very very very long text"));
}

TYPED_TEST(SmallString, short_path_string_move_assignment)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");
    SmallString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(copy, SmallString("text"));
}

TYPED_TEST(SmallString, long_path_string_move_assignment)
{
    using SmallString = typename TestFixture::String;
    SmallString text(
        "this is a very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very very very very very "
        "very very very very very very very very very very very very very very long text");
    SmallString copy("more text");

    copy = std::move(text);

    ASSERT_THAT(
        copy,
        SmallString(
            "this is a very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very very "
            "very very very very very very very very very very very very very very very very long "
            "text"));
}

TYPED_TEST(SmallString, short_small_string_take)
{
    using SmallString = typename TestFixture::String;
    SmallString text("text");
    SmallString copy("more text");

    copy = text.take();

    ASSERT_THAT(text, IsEmpty());
    ASSERT_THAT(copy, SmallString("text"));
}

TYPED_TEST(SmallString, long_small_string_take)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is a very very very very long text");
    SmallString copy("more text");

    copy = text.take();

    ASSERT_THAT(text, IsEmpty());
    ASSERT_THAT(copy, SmallString("this is a very very very very long text"));
}

TYPED_TEST(SmallString, replace_by_position_shorter_with_longer_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is a very very very very long text");

    text.replace(8, 1, "some");

    ASSERT_THAT(text, SmallString("this is some very very very very long text"));
}

TYPED_TEST(SmallString, replace_by_position_longer_with_short_text)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is some very very very very long text");

    text.replace(8, 4, "a");

    ASSERT_THAT(text, SmallString("this is a very very very very long text"));
}

TYPED_TEST(SmallString, replace_by_position_equal_sized_texts)
{
    using SmallString = typename TestFixture::String;
    SmallString text("this is very very very very very long text");

    text.replace(33, 4, "much");

    ASSERT_THAT(text, SmallString("this is very very very very very much text"));
}

TYPED_TEST(SmallString, compare_text_with_different_line_endings)
{
    using SmallString = typename TestFixture::String;
    SmallString unixText("some \ntext");
    SmallString windowsText("some \n\rtext");

    auto convertedText = windowsText.toCarriageReturnsStripped();

    ASSERT_THAT(unixText, convertedText);
}

TYPED_TEST(SmallString, const_subscript_operator)
{
    using SmallString = typename TestFixture::String;
    const SmallString text{"some text"};

    auto &&sign = text[5];

    ASSERT_THAT(sign, 't');
}

TYPED_TEST(SmallString, non_const_subscript_operator)
{
    using SmallString = typename TestFixture::String;
    SmallString text{"some text"};

    auto &&sign = text[5];

    ASSERT_THAT(sign, 't');
}

TYPED_TEST(SmallString, manipulate_const_subscript_operator)
{
    using SmallString = typename TestFixture::String;
    const SmallString text{"some text"};
    auto &&sign = text[5];

    sign = 'q';

    ASSERT_THAT(text, SmallString{"some text"});
}

TYPED_TEST(SmallString, manipulate_non_const_subscript_operator)
{
    using SmallString = typename TestFixture::String;
    char rawText[] = "some text";
    SmallString text{rawText};
    auto &&sign = text[5];

    sign = 'q';

    ASSERT_THAT(text, SmallString{"some qext"});
}

TYPED_TEST(SmallString, empty_initializer_list_content)
{
    using SmallString = typename TestFixture::String;
    SmallString text = {};

    ASSERT_THAT(text, SmallString());
}

TYPED_TEST(SmallString, empty_initializer_list_size)
{
    using SmallString = typename TestFixture::String;
    SmallString text = {};

    ASSERT_THAT(text, SizeIs(0));
}

TYPED_TEST(SmallString, initializer_list_content)
{
    using SmallString = typename TestFixture::String;
    auto text = SmallString::join({"some", " ", "text"});

    ASSERT_THAT(text, SmallString("some text"));
}

TYPED_TEST(SmallString, initializer_list_size)
{
    using SmallString = typename TestFixture::String;
    auto text = SmallString::join({"some", " ", "text"});

    ASSERT_THAT(text, SizeIs(9));
}

TYPED_TEST(SmallString, number_to_string)
{
    using SmallString = typename TestFixture::String;
    ASSERT_THAT(SmallString::number(-0), "0");
    ASSERT_THAT(SmallString::number(1), "1");
    ASSERT_THAT(SmallString::number(-1), "-1");
    ASSERT_THAT(SmallString::number(std::numeric_limits<int>::max()), "2147483647");
    ASSERT_THAT(SmallString::number(std::numeric_limits<int>::min()), "-2147483648");
    ASSERT_THAT(SmallString::number(std::numeric_limits<long long int>::max()), "9223372036854775807");
    ASSERT_THAT(SmallString::number(std::numeric_limits<long long int>::min()), "-9223372036854775808");
    ASSERT_THAT(SmallString::number(1.2), "1.2");
    ASSERT_THAT(SmallString::number(-1.2), "-1.2");
    ASSERT_THAT(SmallString::number(1.2f), "1.2");
    ASSERT_THAT(SmallString::number(-1.2f), "-1.2");
}

TYPED_TEST(SmallString, string_view_plus_operator)
{
    SmallStringView text = "text";

    auto result = text + " and more text";

    ASSERT_THAT(result, "text and more text");
}

TYPED_TEST(SmallString, string_view_plus_operator_reverse_order)
{
    SmallStringView text = " and more text";

    auto result = "text" + text;

    ASSERT_THAT(result, "text and more text");
}

TYPED_TEST(SmallString, string_plus_operator)
{
    using SmallString = typename TestFixture::String;
    SmallString text = "text";

    auto result = text + " and more text";

    ASSERT_THAT(result, "text and more text");
}

TYPED_TEST(SmallString, string_plus_operator_reverse_order)
{
    using SmallString = typename TestFixture::String;
    SmallString text = " and more text";

    auto result = "text" + text;

    ASSERT_THAT(result, "text and more text");
}

TYPED_TEST(SmallString, short_string_capacity)
{
    ASSERT_THAT(Utils::SmallString().shortStringCapacity(), 31);
    ASSERT_THAT(Utils::PathString().shortStringCapacity(), 176);
}

TYPED_TEST(SmallString, to_view)
{
    using SmallString = typename TestFixture::String;
    SmallString text = "text";

    auto view = text.toStringView();

    ASSERT_THAT(view, "text");
}

TYPED_TEST(SmallString, compare)
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

} // namespace
