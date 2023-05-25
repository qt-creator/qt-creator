// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <condition_variable>
#include <mutex>

class Notification
{
public:
    void wait(int count = 1)
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_waitCount += count;
        if (m_waitCount > 0)
            m_condition.wait(lock, [&] { return m_waitCount <= 0; });
    }

    void notify()
    {
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            --m_waitCount;
        }

        m_condition.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    int m_waitCount = 0;
};
