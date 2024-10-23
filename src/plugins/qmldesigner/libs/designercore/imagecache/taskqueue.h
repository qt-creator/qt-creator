// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nanotrace/nanotracehr.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace QmlDesigner {

template<typename Task, typename DispatchCallback, typename ClearCallback>
class TaskQueue
{
    using Tasks = std::deque<Task>;

public:
    TaskQueue(DispatchCallback dispatchCallback, ClearCallback clearCallback)
        : m_dispatchCallback(std::move(dispatchCallback))
        , m_clearCallback(std::move(clearCallback))
    {}

    ~TaskQueue() { destroy(); }

    template<typename TraceEvent, NanotraceHR::Tracing isEnabled, typename... Arguments>
    void addTask(NanotraceHR::Token<TraceEvent, isEnabled> traceToken, Arguments &&...arguments)
    {
        {
            std::unique_lock lock{m_mutex};

            ensureThreadIsRunning(lock, std::move(traceToken));

            m_tasks.emplace_back(std::forward<Arguments>(arguments)...);
        }
        m_condition.notify_all();
    }

    template<typename... Arguments>
    void addTask(Arguments &&...arguments)
    {
        addTask(NanotraceHR::DisabledToken{}, std::forward<Arguments>(arguments)...);
    }

    void clean()
    {
        Tasks oldTasks;
        {
            std::unique_lock lock{m_mutex};
            std::swap(m_tasks, oldTasks);
        }
        clearTasks(oldTasks);
    }

    void putThreadToSleep()
    {
        {
            std::unique_lock lock{m_mutex};
            m_sleeping = true;
        }
        m_condition.notify_all();
    }

private:
    void destroy()
    {
        stopThread();
        joinThread();
        clearTasks(m_tasks);
    }

    [[nodiscard]] std::tuple<std::unique_lock<std::mutex>, bool> waitForTasks()
    {
        using namespace std::literals::chrono_literals;
        std::unique_lock lock{m_mutex};

        if (m_finishing || m_sleeping)
            return {std::move(lock), true};

        if (m_tasks.empty()) {
            auto timedOutWithoutEntriesOrFinishing = !m_condition.wait_for(lock, 10min, [&] {
                return m_tasks.size() || m_finishing || m_sleeping;
            });

            if (timedOutWithoutEntriesOrFinishing)
                m_sleeping = true;
        }

        return {std::move(lock), m_finishing || m_sleeping};
    }

    [[nodiscard]] std::optional<Task> getTask(std::unique_lock<std::mutex> lock)
    {
        auto l = std::move(lock);

        if (m_tasks.empty())
            return {};

        Task task = std::move(m_tasks.front());
        m_tasks.pop_front();

        return {std::move(task)};
    }

    template<typename Lock, typename TraceToken>
    void ensureThreadIsRunning(Lock &lock, TraceToken traceToken)
    {
        using namespace NanotraceHR::Literals;

        if (m_finishing || !m_sleeping)
            return;

        if (m_sleeping) {
            lock.unlock();
            joinThread();
            lock.lock();

            m_sleeping = false;
        }

        if (m_backgroundThread.joinable())
            return;

        auto [threadCreateToken, flowToken] = traceToken.beginDurationWithFlow(
            "thread is created in the task queue");
        m_backgroundThread = std::thread{[this](auto traceToken) {
                                             auto duration = traceToken.beginDuration(
                                                 "thread is ready");

                                             while (true) {
                                                 auto [lock, abort] = waitForTasks();
                                                 duration.end();

                                                 if (abort)
                                                     return;

                                                 auto getTaskToken = duration.beginDuration(
                                                     "get task from queue");
                                                 if (auto task = getTask(std::move(lock)); task) {
                                                     getTaskToken.end();
                                                     m_dispatchCallback(*task);
                                                 }
                                             }
                                         },
                                         std::move(flowToken)};
    }

    void clearTasks(Tasks &tasks)
    {
        for (Task &task : tasks)
            m_clearCallback(task);
    }

    void stopThread()
    {
        {
            std::unique_lock lock{m_mutex};
            m_finishing = true;
        }
        m_condition.notify_all();
    }

    void joinThread()
    {
        if (m_backgroundThread.joinable())
            m_backgroundThread.join();
    }

private:
    Tasks m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_backgroundThread;
    DispatchCallback m_dispatchCallback;
    ClearCallback m_clearCallback;
    bool m_finishing{false};
    bool m_sleeping{true};
};
} // namespace QmlDesigner
