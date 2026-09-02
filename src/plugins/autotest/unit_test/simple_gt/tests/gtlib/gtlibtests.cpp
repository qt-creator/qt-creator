// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "gtlibtests.h"

#include <gtest/gtest.h>

int libValue()
{
    return 42;
}

TEST(InLibTest, KnowsItsValue)
{
    EXPECT_EQ(42, libValue());
}

TEST(InLibTest, ValueIsPositive)
{
    EXPECT_GT(libValue(), 0);
}
