// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <utils/sizedarray.h>

namespace {

using Utils::SizedArray;

TEST(SizedArray, empty_size)
{
    SizedArray<char, 8> array;

    ASSERT_THAT(array.size(), 0);
}

TEST(SizedArray, is_empty)
{
    SizedArray<char, 8> array;

    ASSERT_TRUE(array.empty());
}

TEST(SizedArray, is_not_empty)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_FALSE(array.empty());
}

TEST(SizedArray, size_one_after_push_back)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(array.size(), 1);
}

TEST(SizedArray, first_value_after_push_back)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(array.front(), 'x');
}

TEST(SizedArray, last_value_after_push_back)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(array.back(), 'x');
}

TEST(SizedArray, end_iterator_is_equal_begin_for_empty_array)
{
    SizedArray<char, 8> array;

    ASSERT_THAT(array.begin(), array.end());
}

TEST(SizedArray, const_end_iterator_is_equal_begin_for_empty_array)
{
    const SizedArray<char, 8> array = {};

    ASSERT_THAT(array.begin(), array.end());
}

TEST(SizedArray, end_iterator_is_one_after_begin_for_one_sized_array)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(std::next(array.begin(), 1), array.end());
}

TEST(SizedArray, c_end_iterator_is_one_after_begin_for_one_sized_array)
{
    SizedArray<char, 8> array = {};

    array.push_back('x');

    ASSERT_THAT(std::next(array.cbegin(), 1), array.cend());
}

TEST(SizedArray, r_end_iterator_is_equal_r_begin_for_empty_array)
{
    SizedArray<char, 8> array;

    ASSERT_THAT(array.rbegin(), array.rend());
}

TEST(SizedArray, const_r_end_iterator_is_equal_r_begin_for_empty_array)
{
    const SizedArray<char, 8> array = {};

    ASSERT_THAT(array.rbegin(), array.rend());
}

TEST(SizedArray, r_end_iterator_is_one_after_r_begin_for_one_sized_array)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(std::next(array.rbegin(), 1), array.rend());
}

TEST(SizedArray, const_r_end_iterator_is_one_after_r_begin_for_one_sized_array)
{
    SizedArray<char, 8> array = {};

    array.push_back('x');

    ASSERT_THAT(std::next(array.crbegin(), 1), array.crend());
}

TEST(SizedArray, initializer_list_size)
{
    SizedArray<char, 8> array{'a', 'b'};

    auto size = array.size();

    ASSERT_THAT(size, 2);
}

} // anonymous namespace
