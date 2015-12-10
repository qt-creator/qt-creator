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

using namespace testing;

int sum(int a, int b)
{
    return a + b;
}

TEST(Sum, HandlePositives)
{
    EXPECT_EQ(5, sum(2, 3));
    EXPECT_EQ(5, sum(3, 2));
}

TEST(Sum, HandleZero)
{
    EXPECT_EQ(1, sum(0, 0));
}

TEST(FactorialTest, DISABLED_Fake)
{
    EXPECT_EQ(-4, sum(-2, -2));
}
