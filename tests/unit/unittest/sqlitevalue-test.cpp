/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <sqlitevalue.h>

namespace {

TEST(SqliteValue, ConstructDefault)
{
    Sqlite::Value value{};

    ASSERT_TRUE(value.isNull());
}

TEST(SqliteValue, ConstructNullValue)
{
    Sqlite::Value value{Sqlite::NullValue{}};

    ASSERT_TRUE(value.isNull());
}

TEST(SqliteValue, ConstructLongLong)
{
    Sqlite::Value value{1LL};

    ASSERT_THAT(value.toInteger(), Eq(1LL));
}

TEST(SqliteValue, ConstructInteger)
{
    Sqlite::Value value{1};

    ASSERT_THAT(value.toInteger(), Eq(1LL));
}

TEST(SqliteValue, ConstructFloatingPoint)
{
    Sqlite::Value value{1.1};

    ASSERT_THAT(value.toFloat(), Eq(1.1));
}

TEST(SqliteValue, ConstructStringFromCString)
{
    Sqlite::Value value{"foo"};

    ASSERT_THAT(value.toStringView(), Eq("foo"));
}

TEST(SqliteValue, ConstructStringFromUtilsString)
{
    Sqlite::Value value{Utils::SmallString{"foo"}};

    ASSERT_THAT(value.toStringView(), Eq("foo"));
}

TEST(SqliteValue, ConstructStringFromQString)
{
    Sqlite::Value value{QString{"foo"}};

    ASSERT_THAT(value.toStringView(), Eq("foo"));
}

TEST(SqliteValue, ConstructStringFromBlob)
{
    //    Utils::span<const Sqlite::byte> bytes{reinterpret_cast<const Sqlite::byte *>("abcd"), 4};

    //    Sqlite::Value value{bytes};

    //ASSERT_THAT(value.toBlob(), Eq(bytes));
}

TEST(SqliteValue, ConstructNullFromNullQVariant)
{
    QVariant variant{};

    Sqlite::Value value{variant};

    ASSERT_TRUE(value.isNull());
}

TEST(SqliteValue, ConstructStringFromIntQVariant)
{
    QVariant variant{1};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toInteger(), Eq(1));
}

TEST(SqliteValue, ConstructStringFromLongLongQVariant)
{
    QVariant variant{1LL};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toInteger(), Eq(1));
}

TEST(SqliteValue, ConstructStringFromUintQVariant)
{
    QVariant variant{1u};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toInteger(), Eq(1));
}

TEST(SqliteValue, ConstructStringFromFloatQVariant)
{
    QVariant variant{1.};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toFloat(), Eq(1));
}

TEST(SqliteValue, ConstructStringFromStringQVariant)
{
    QVariant variant{QString{"foo"}};

    Sqlite::Value value{variant};

    ASSERT_THAT(value.toStringView(), Eq("foo"));
}

TEST(SqliteValue, ConvertToNullQVariant)
{
    Sqlite::Value value{};

    auto variant = QVariant{value};

    ASSERT_TRUE(variant.isNull());
}

TEST(SqliteValue, ConvertToStringQVariant)
{
    Sqlite::Value value{"foo"};

    auto variant = QVariant{value};

    ASSERT_THAT(variant, Eq("foo"));
}

TEST(SqliteValue, ConvertToIntegerQVariant)
{
    Sqlite::Value value{1};

    auto variant = QVariant{value};

    ASSERT_THAT(variant, Eq(1));
}

TEST(SqliteValue, ConvertToFloatQVariant)
{
    Sqlite::Value value{1.1};

    auto variant = QVariant{value};

    ASSERT_THAT(variant, Eq(1.1));
}

TEST(SqliteValue, IntegerEquals)
{
    bool isEqual = Sqlite::Value{1} == 1LL;

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, IntegerEqualsInverse)
{
    bool isEqual = 1LL == Sqlite::Value{1};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, FloatEquals)
{
    bool isEqual = Sqlite::Value{1.0} == 1.;

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, FloatEqualsInverse)
{
    bool isEqual = 1. == Sqlite::Value{1.0};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, StringEquals)
{
    bool isEqual = Sqlite::Value{"foo"} == "foo";

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, StringEqualsInverse)
{
    bool isEqual = "foo" == Sqlite::Value{"foo"};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, IntegerAndFloatAreNotEquals)
{
    bool isEqual = Sqlite::Value{1} == 1.;

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, NullValuesNeverEqual)
{
    bool isEqual = Sqlite::Value{} == Sqlite::Value{};

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, IntegerValuesAreEquals)
{
    bool isEqual = Sqlite::Value{1} == Sqlite::Value{1};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, IntegerAndFloatValuesAreNotEquals)
{
    bool isEqual = Sqlite::Value{1} == Sqlite::Value{1.};

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, StringAndQStringAreEquals)
{
    bool isEqual = Sqlite::Value{"foo"} == QString{"foo"};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, IntegerAndFloatValuesAreUnequal)
{
    bool isUnequal = Sqlite::Value{1} != Sqlite::Value{1.0};

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, IntegerAndFloatAreUnequal)
{
    bool isUnequal = Sqlite::Value{1} != 1.0;

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, IntegerAndFloatAreUnequalInverse)
{
    bool isUnequal = 1.0 != Sqlite::Value{1};

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, IntegersAreUnequal)
{
    bool isUnequal = Sqlite::Value{1} != 2;

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, IntegersAreUnequalInverse)
{
    bool isUnequal = 2 != Sqlite::Value{1};

    ASSERT_TRUE(isUnequal);
}

TEST(SqliteValue, NullType)
{
    auto type = Sqlite::Value{}.type();

    ASSERT_THAT(type, Sqlite::ValueType::Null);
}

TEST(SqliteValue, IntegerType)
{
    auto type = Sqlite::Value{1}.type();

    ASSERT_THAT(type, Sqlite::ValueType::Integer);
}

TEST(SqliteValue, FloatType)
{
    auto type = Sqlite::Value{1.}.type();

    ASSERT_THAT(type, Sqlite::ValueType::Float);
}

TEST(SqliteValue, StringType)
{
    auto type = Sqlite::Value{"foo"}.type();

    ASSERT_THAT(type, Sqlite::ValueType::String);
}

TEST(SqliteValue, NullValueAndValueViewAreNotEqual)
{
    bool isEqual = Sqlite::ValueView::create(Sqlite::NullValue{}) == Sqlite::Value{};

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, NullValueViewAndValueAreNotEqual)
{
    bool isEqual = Sqlite::Value{} == Sqlite::ValueView::create(Sqlite::NullValue{});

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, StringValueAndValueViewEquals)
{
    bool isEqual = Sqlite::ValueView::create("foo") == Sqlite::Value{"foo"};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, StringValueAndValueViewEqualsInverse)
{
    bool isEqual = Sqlite::Value{"foo"} == Sqlite::ValueView::create("foo");

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, IntegerValueAndValueViewEquals)
{
    bool isEqual = Sqlite::ValueView::create(1) == Sqlite::Value{1};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, IntegerValueAndValueViewEqualsInverse)
{
    bool isEqual = Sqlite::Value{2} == Sqlite::ValueView::create(2);

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, FloatValueAndValueViewEquals)
{
    bool isEqual = Sqlite::ValueView::create(1.1) == Sqlite::Value{1.1};

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, FloatValueAndValueViewEqualsInverse)
{
    bool isEqual = Sqlite::Value{1.1} == Sqlite::ValueView::create(1.1);

    ASSERT_TRUE(isEqual);
}

TEST(SqliteValue, StringValueAndIntergerValueViewAreNotEqual)
{
    bool isEqual = Sqlite::Value{"foo"} == Sqlite::ValueView::create(1);

    ASSERT_FALSE(isEqual);
}

TEST(SqliteValue, ConvertNullValueViewIntoValue)
{
    auto view = Sqlite::ValueView::create(Sqlite::NullValue{});

    Sqlite::Value value{view};

    ASSERT_TRUE(value.isNull());
}

TEST(SqliteValue, ConvertStringValueViewIntoValue)
{
    auto view = Sqlite::ValueView::create("foo");

    Sqlite::Value value{view};

    ASSERT_THAT(value, Eq("foo"));
}

TEST(SqliteValue, ConvertIntegerValueViewIntoValue)
{
    auto view = Sqlite::ValueView::create(1);

    Sqlite::Value value{view};

    ASSERT_THAT(value, Eq(1));
}

TEST(SqliteValue, ConvertFloatValueViewIntoValue)
{
    auto view = Sqlite::ValueView::create(1.4);

    Sqlite::Value value{view};

    ASSERT_THAT(value, Eq(1.4));
}

} // namespace
