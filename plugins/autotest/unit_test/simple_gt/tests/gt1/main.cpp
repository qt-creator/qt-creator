/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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
