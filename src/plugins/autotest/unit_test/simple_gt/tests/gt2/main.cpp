// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "queuetest.h"

using namespace testing;

TEST_F(QueueTest, IsEmptyInitialized)
{
    EXPECT_EQ(0, m_queue0.size());
}

TEST_F(QueueTest, DequeueWorks)
{
    ASSERT_FALSE(m_queue1.isEmpty());
    int n = m_queue1.dequeue();
    ASSERT_TRUE(m_queue1.isEmpty());
    EXPECT_EQ(1, n);

    ASSERT_FALSE(m_queue2.isEmpty());
    float f = m_queue2.dequeue();
    ASSERT_TRUE(m_queue2.isEmpty());
    EXPECT_EQ(1.0f, f);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
