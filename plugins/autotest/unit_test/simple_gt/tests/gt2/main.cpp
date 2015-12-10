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
