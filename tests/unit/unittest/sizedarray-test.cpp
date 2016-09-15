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

} // anonymous namespace
