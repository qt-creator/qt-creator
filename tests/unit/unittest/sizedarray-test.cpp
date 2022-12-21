// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <utils/sizedarray.h>

namespace {

using Utils::SizedArray;

TEST(SizedArray, EmptySize)
{
    SizedArray<char, 8> array;

    ASSERT_THAT(array.size(), 0);
}

TEST(SizedArray, IsEmpty)
{
    SizedArray<char, 8> array;

    ASSERT_TRUE(array.empty());
}

TEST(SizedArray, IsNotEmpty)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_FALSE(array.empty());
}

TEST(SizedArray, SizeOneAfterPushBack)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(array.size(), 1);
}

TEST(SizedArray, FirstValueAfterPushBack)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(array.front(), 'x');
}

TEST(SizedArray, LastValueAfterPushBack)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(array.back(), 'x');
}

TEST(SizedArray, EndIteratorIsEqualBeginForEmptyArray)
{
    SizedArray<char, 8> array;

    ASSERT_THAT(array.begin(), array.end());
}

TEST(SizedArray, ConstEndIteratorIsEqualBeginForEmptyArray)
{
    const SizedArray<char, 8> array = {};

    ASSERT_THAT(array.begin(), array.end());
}

TEST(SizedArray, EndIteratorIsOneAfterBeginForOneSizedArray)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(std::next(array.begin(), 1), array.end());
}

TEST(SizedArray, CEndIteratorIsOneAfterBeginForOneSizedArray)
{
    SizedArray<char, 8> array = {};

    array.push_back('x');

    ASSERT_THAT(std::next(array.cbegin(), 1), array.cend());
}

TEST(SizedArray, REndIteratorIsEqualRBeginForEmptyArray)
{
    SizedArray<char, 8> array;

    ASSERT_THAT(array.rbegin(), array.rend());
}

TEST(SizedArray, ConstREndIteratorIsEqualRBeginForEmptyArray)
{
    const SizedArray<char, 8> array = {};

    ASSERT_THAT(array.rbegin(), array.rend());
}

TEST(SizedArray, REndIteratorIsOneAfterRBeginForOneSizedArray)
{
    SizedArray<char, 8> array;

    array.push_back('x');

    ASSERT_THAT(std::next(array.rbegin(), 1), array.rend());
}

TEST(SizedArray, ConstREndIteratorIsOneAfterRBeginForOneSizedArray)
{
    SizedArray<char, 8> array = {};

    array.push_back('x');

    ASSERT_THAT(std::next(array.crbegin(), 1), array.crend());
}

TEST(SizedArray, InitializerListSize)
{
    SizedArray<char, 8> array{'a', 'b'};

    auto size = array.size();

    ASSERT_THAT(size, 2);
}

} // anonymous namespace
