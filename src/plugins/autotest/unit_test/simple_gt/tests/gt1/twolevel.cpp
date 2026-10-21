// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <gtest/gtest.h>

#define LEVEL_TWO(suite, name) TEST(suite, name)
#define LEVEL_ONE() LEVEL_TWO(CustomMacro, TwoLevels)

LEVEL_ONE()
{
    EXPECT_EQ(2, 2);
}
