// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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

namespace Dummy {
namespace {
namespace Internal {

TEST(NamespaceTest, Valid)
{
    EXPECT_EQ(1, 1);
}

} // namespace Internal
} // anon namespace
} // namespace Dummy
