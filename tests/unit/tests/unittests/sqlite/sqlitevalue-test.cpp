// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sqlitevalue.h>
#include <utils/span.h>

namespace {

TEST(SqliteValue, construct_default)
{
    Sqlite::Value value{};

    ASSERT_TRUE(value.isNull());
}

TEST(SqliteValue, construct_null_value)
{
    Sqlite::Value value{Sqlite::NullValue{}};

    ASSERT_TRUE(value.isNull());
}

TEST(SqliteValue, construct_long_long)
{
    Sqlite::Value value{1LL};

    ASSERT_THAT(value.toInteger(), Eq(1LL));
}

TEST(SqliteValue, construct_integer)
{
    Sqlite::Value value{1};

    ASSERT_THAT(value.toInteger(), Eq(1LL));
}

TEST(SqliteValue, construct_floating_point)
{
    Sqlite::Value value{1.1};

    ASSERT_THAT(value.toFloat(), Eq(1.1));
}

TEST(SqliteValue, construct_string_from_c_string)
{
    Sqlite::Value value{"foo"};

    ASSERT_THAT(value.toStringView(), Eq("foo"));
}

TEST(SqliteValue, construct_string_from_utils_string)
{
    Sqlite::Value value{Utils::SmallString{"foo"}};

    ASSERT_THAT(value.toStringView(), Eq("foo"));
}

TEST(SqliteValue, construct_string_from_q_string)
{
    Sqlite::Value value{QString{"foo"}};

    ASSERT_THAT(value.toStringView(), Eq("foo"));
}

TEST(SqliteValue, construct_blob_from_span)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};

    Sqlite::Value value{Sqlite::BlobView{bytes}};

    ASSERT_THAT(value.toBlobView(), Eq(bytes));
}

TEST(SqliteValue, construct_blob_from_blob)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};

    Sqlite::Value value{Sqlite::Blob{bytes}};

    ASSERT_THAT(value.toBlobView(), Eq(bytes));
}

TEST(SqliteValue, construct_null_from_null_q_variant)
{
    QVariant variant{};

    Sqlite::Value value{variant};

    ASSERT_TRUE(value.isNull());
}

TEST(SqliteValue, construct_string_from_int_q_variant)
{
    QVariant variant{1};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toInteger(), Eq(1));
}

TEST(SqliteValue, construct_string_from_long_long_q_variant)
{
    QVariant variant{1LL};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toInteger(), Eq(1));
}

TEST(SqliteValue, construct_string_from_uint_q_variant)
{
    QVariant variant{1u};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toInteger(), Eq(1));
}

TEST(SqliteValue, construct_string_from_float_q_variant)
{
    QVariant variant{1.};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toFloat(), Eq(1));
}

TEST(SqliteValue, construct_string_from_string_q_variant)
{
    QVariant variant{QString{"foo"}};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toStringView(), Eq("foo"));
}

TEST(SqliteValue, construct_blob_from_byte_array_q_variant)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};
    QVariant variant{QByteArray{"abcd"}};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toBlobView(), Eq(bytes));
}

TEST(SqliteValue, convert_to_null_q_variant)
{
    Sqlite::Value value{};

    auto variant = QVariant{value};

    ASSERT_TRUE(variant.isNull());
}

TEST(SqliteValue, convert_to_string_q_variant)
{
    Sqlite::Value value{"foo"};

    auto variant = QVariant{value};

    ASSERT_THAT(variant, Eq("foo"));
}

TEST(SqliteValue, convert_to_integer_q_variant)
{
    Sqlite::Value value{1};

    auto variant = QVariant{value};

    ASSERT_THAT(variant, Eq(1));
}

TEST(SqliteValue, convert_to_float_q_variant)
{
    Sqlite::Value value{1.1};

    auto variant = QVariant{value};

    ASSERT_THAT(variant, Eq(1.1));
}

TEST(SqliteValue, convert_to_byte_array_q_variant)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};
    Sqlite::Value value{bytes};

    auto variant = QVariant{value};

    ASSERT_THAT(variant, Eq(QByteArray{"abcd"}));
}

TEST(SqliteValue, integer_equals)
{
    bool isEqual = Sqlite::Value{1} == 1LL;

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, integer_equals_inverse)
{
    bool isEqual = 1LL == Sqlite::Value{1};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, float_equals)
{
    bool isEqual = Sqlite::Value{1.0} == 1.;

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, float_equals_inverse)
{
    bool isEqual = 1. == Sqlite::Value{1.0};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, string_equals)
{
    bool isEqual = Sqlite::Value{"foo"} == "foo";

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, string_equals_inverse)
{
    bool isEqual = "foo" == Sqlite::Value{"foo"};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, blob_equals)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};
    bool isEqual = Sqlite::Value{bytes} == bytes;

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, blob_inverse_equals)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};
    bool isEqual = bytes == Sqlite::Value{bytes};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, integer_and_float_are_not_equals)
{
    bool isEqual = Sqlite::Value{1} == 1.;

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, null_values_never_equal)
{
    bool isEqual = Sqlite::Value{} == Sqlite::Value{};

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, integer_values_are_equals)
{
    bool isEqual = Sqlite::Value{1} == Sqlite::Value{1};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, integer_and_float_values_are_not_equals)
{
    bool isEqual = Sqlite::Value{1} == Sqlite::Value{1.};

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, string_and_q_string_are_equals)
{
    bool isEqual = Sqlite::Value{"foo"} == QString{"foo"};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, integer_and_float_values_are_unequal)
{
    bool isUnequal = Sqlite::Value{1} != Sqlite::Value{1.0};

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, integer_and_float_are_unequal)
{
    bool isUnequal = Sqlite::Value{1} != 1.0;

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, integer_and_float_are_unequal_inverse)
{
    bool isUnequal = 1.0 != Sqlite::Value{1};

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, integers_are_unequal)
{
    bool isUnequal = Sqlite::Value{1} != 2;

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, integers_are_unequal_inverse)
{
    bool isUnequal = 2 != Sqlite::Value{1};

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, null_type)
{
    auto type = Sqlite::Value{}.type();

    ASSERT_THAT(type, Sqlite::ValueType::Null);
}

TEST(SqliteValue, integer_type)
{
    auto type = Sqlite::Value{1}.type();

    ASSERT_THAT(type, Sqlite::ValueType::Integer);
}

TEST(SqliteValue, float_type)
{
    auto type = Sqlite::Value{1.}.type();

    ASSERT_THAT(type, Sqlite::ValueType::Float);
}

TEST(SqliteValue, string_type)
{
    auto type = Sqlite::Value{"foo"}.type();

    ASSERT_THAT(type, Sqlite::ValueType::String);
}

TEST(SqliteValue, blob_type)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};
    auto type = Sqlite::Value{bytes}.type();

    ASSERT_THAT(type, Sqlite::ValueType::Blob);
}

TEST(SqliteValue, null_value_and_value_view_are_not_equal)
{
    bool isEqual = Sqlite::ValueView::create(Sqlite::NullValue{}) == Sqlite::Value{};

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, null_value_view_and_value_are_not_equal)
{
    bool isEqual = Sqlite::Value{} == Sqlite::ValueView::create(Sqlite::NullValue{});

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, string_value_and_value_view_equals)
{
    bool isEqual = Sqlite::ValueView::create("foo") == Sqlite::Value{"foo"};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, string_value_and_value_view_equals_inverse)
{
    bool isEqual = Sqlite::Value{"foo"} == Sqlite::ValueView::create("foo");

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, integer_value_and_value_view_equals)
{
    bool isEqual = Sqlite::ValueView::create(1) == Sqlite::Value{1};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, integer_value_and_value_view_equals_inverse)
{
    bool isEqual = Sqlite::Value{2} == Sqlite::ValueView::create(2);

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, float_value_and_value_view_equals)
{
    bool isEqual = Sqlite::ValueView::create(1.1) == Sqlite::Value{1.1};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, float_value_and_value_view_equals_inverse)
{
    bool isEqual = Sqlite::Value{1.1} == Sqlite::ValueView::create(1.1);

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, string_value_and_interger_value_view_are_not_equal)
{
    bool isEqual = Sqlite::Value{"foo"} == Sqlite::ValueView::create(1);

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, blob_value_and_value_view_equals)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};

    bool isEqual = Sqlite::ValueView::create(Sqlite::BlobView{bytes}) == Sqlite::Value{bytes};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, convert_null_value_view_into_value)
{
    auto view = Sqlite::ValueView::create(Sqlite::NullValue{});

    Sqlite::Value value{view};

    ASSERT_TRUE(value.isNull());
}

TEST(SqliteValue, convert_string_value_view_into_value)
{
    auto view = Sqlite::ValueView::create("foo");

    Sqlite::Value value{view};

    ASSERT_THAT(value, Eq("foo"));
}

TEST(SqliteValue, convert_integer_value_view_into_value)
{
    auto view = Sqlite::ValueView::create(1);

    Sqlite::Value value{view};

    ASSERT_THAT(value, Eq(1));
}

TEST(SqliteValue, convert_float_value_view_into_value)
{
    auto view = Sqlite::ValueView::create(1.4);

    Sqlite::Value value{view};

    ASSERT_THAT(value, Eq(1.4));
}

TEST(SqliteValue, convert_blob_value_view_into_value)
{
    Utils::span<const std::byte> bytes{reinterpret_cast<const std::byte *>("abcd"), 4};
    auto view = Sqlite::ValueView::create(Sqlite::BlobView{bytes});

    Sqlite::Value value{view};

    ASSERT_THAT(value, Eq(Sqlite::BlobView{bytes}));
}

} // namespace
