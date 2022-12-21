// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <gtest/gtest.h>
#include <QQueue>

class QueueTest : public ::testing::Test
{
protected:
    virtual void SetUp() {
        m_queue1.enqueue(1);
        m_queue2.enqueue(1.0f);
    }

    QQueue<double> m_queue0;
    QQueue<int> m_queue1;
    QQueue<float> m_queue2;
};
