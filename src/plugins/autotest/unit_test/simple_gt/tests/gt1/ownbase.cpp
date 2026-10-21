// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <gtest/gtest.h>

class OwnBase : public ::testing::Test
{};

#define OWN_BASE_TEST(suite, name) \
    GTEST_TEST_(suite, name, OwnBase, ::testing::internal::GetTypeId<OwnBase>())

OWN_BASE_TEST(OwnBaseMacro, Plain)
{
    EXPECT_EQ(1, 1);
}
