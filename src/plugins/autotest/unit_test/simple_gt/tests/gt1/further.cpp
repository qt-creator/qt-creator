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

#define MYTEST TEST

MYTEST(CustomMacro, Alias)
{
    EXPECT_EQ(2, sum(1, 1));
}

#define MY_TEST(suite, name) TEST(suite, name)

MY_TEST(CustomMacro, Wrapped)
{
    EXPECT_EQ(3, sum(1, 2));
}

#define MAKE_SCENARIO(TestClass, description, FixtureClass) \
    class TestClass : public FixtureClass \
    { \
    private: \
        void TestBody() override; \
        static ::testing::TestInfo *const test_info_; \
    }; \
    ::testing::TestInfo *const TestClass::test_info_ \
        = ::testing::internal::MakeAndRegisterTestInfo( \
            #FixtureClass, description, nullptr, nullptr, \
            ::testing::internal::CodeLocation(__FILE__, __LINE__), \
            ::testing::internal::GetTypeId<FixtureClass>(), \
            FixtureClass::SetUpTestSuite, FixtureClass::TearDownTestSuite, \
            new ::testing::internal::TestFactoryImpl<TestClass>); \
    void TestClass::TestBody()

class Scenarios : public ::testing::Test
{};

/* custom */ MAKE_SCENARIO(ScenarioOne, "described: log-in \"read/write\" 1/2", Scenarios)
{
    EXPECT_EQ(4, sum(2, 2));
}
