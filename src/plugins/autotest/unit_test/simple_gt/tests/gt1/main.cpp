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
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace testing;

int factorial(int a)
{
    if (a == 0)
        return 1;
    if (a == 1)
        return a;
    return a * factorial(a - 1);
}

int factorial_it(int a)
{
    int result = 1;
    for (int i = 2; i <= a; ++i)
        result *= i;
    return result;
}

TEST(FactorialTest, HandlesZeroInput)
{
    EXPECT_EQ(1, factorial(0));
}

TEST(FactorialTest, HandlesPositiveInput)
{
    ASSERT_EQ(1, factorial(1));
    ASSERT_THAT(factorial(2), Eq(2));
    EXPECT_EQ(6, factorial(3));
    EXPECT_EQ(40320, factorial(8));
}

TEST(FactorialTest_Iterative, HandlesZeroInput)
{
    ASSERT_EQ(1, factorial_it(0));
}

TEST(FactorialTest_Iterative, DISABLED_HandlesPositiveInput)
{
    ASSERT_EQ(1, factorial_it(1));
    ASSERT_EQ(2, factorial_it(2));
    ASSERT_EQ(6, factorial_it(3));
    ASSERT_EQ(40320, factorial_it(8));
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
