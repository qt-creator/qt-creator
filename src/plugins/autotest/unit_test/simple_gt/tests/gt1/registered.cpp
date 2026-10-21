// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <gtest/gtest.h>

class Registered : public ::testing::Test
{
    void TestBody() override { EXPECT_EQ(3, 3); }
};

#define REGISTER_SCENARIO(description, TestClass) \
    static ::testing::TestInfo *const TestClass##Info \
        = ::testing::internal::MakeAndRegisterTestInfo( \
            "RegisteredMacro", description, nullptr, nullptr, \
            ::testing::internal::CodeLocation(__FILE__, __LINE__), \
            ::testing::internal::GetTypeId<::testing::Test>(), \
            ::testing::Test::SetUpTestSuite, ::testing::Test::TearDownTestSuite, \
            new ::testing::internal::TestFactoryImpl<TestClass>);

REGISTER_SCENARIO("registered only", Registered)
