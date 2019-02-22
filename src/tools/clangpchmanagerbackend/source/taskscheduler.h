/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "taskschedulerinterface.h"
#include "symbolindexertask.h"
#include "queueinterface.h"
#include "progresscounter.h"

#include <executeinloop.h>
#include <processormanagerinterface.h>
#include <symbolindexertaskqueueinterface.h>
#include <symbolscollectorinterface.h>

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

#include <algorithm>
#include <functional>
#include <future>
#include <thread>
#include <vector>

namespace Sqlite {
class TransactionInterface;
};

namespace ClangBackEnd {

class FilePathCachingInterface;
class ProcessorManagerInterface;
class QueueInterface;
class SymbolStorageInterface;

enum class CallDoInMainThreadAfterFinished : char { Yes, No };

template <typename ProcessorManager,
          typename Task>
class TaskScheduler : public TaskSchedulerInterface<Task>
{
public:
    using ProcessorInterface = typename ProcessorManager::Processor;
    using Future = std::future<ProcessorInterface&>;

    TaskScheduler(ProcessorManager &processorManager,
                  QueueInterface &queue,
                  ProgressCounter &progressCounter,
                  uint hardwareConcurrency,
                  CallDoInMainThreadAfterFinished callDoInMainThreadAfterFinished,
                  std::launch launchPolicy = std::launch::async)
        : m_processorManager(processorManager)
        , m_queue(queue)
        , m_progressCounter(progressCounter)
        , m_hardwareConcurrency(hardwareConcurrency)
        , m_launchPolicy(launchPolicy)
        , m_callDoInMainThreadAfterFinished(callDoInMainThreadAfterFinished)
    {}

    ~TaskScheduler()
    {
        syncTasks();
    }

    void addTasks(std::vector<Task> &&tasks)
    {
        for (auto &task : tasks) {
            auto callWrapper = [&, task = std::move(task)](auto processor) -> ProcessorInterface & {
                task(processor.get());
                executeInLoop([&] { m_queue.processEntries(); });

                return processor;
            };
            m_futures.emplace_back(std::async(m_launchPolicy,
                                              std::move(callWrapper),
                                              std::ref(m_processorManager.unusedProcessor())));
        }
    }

    const std::vector<Future> &futures() const
    {
        return m_futures;
    }

    SlotUsage slotUsage()
    {
        removeFinishedFutures();

        if (m_isDisabled)
            return {};

        return {uint(std::max(int(m_hardwareConcurrency) - int(m_futures.size()), 0)),
                uint(m_futures.size())};
    }

    void syncTasks()
    {
        for (auto &future : m_futures)
            future.wait();
    }

    void disable()
    {
        m_isDisabled = true;
    }

private:
    void removeFinishedFutures()
    {
        auto notReady = [] (Future &future) {
            return future.wait_for(std::chrono::duration<int>::zero()) != std::future_status::ready;
        };

        auto split = std::partition(m_futures.begin(), m_futures.end(), notReady);

        std::for_each(split, m_futures.end(), [&] (Future &future) {
            ProcessorInterface &processor = future.get();
            if (m_callDoInMainThreadAfterFinished == CallDoInMainThreadAfterFinished::Yes)
                processor.doInMainThreadAfterFinished();
            processor.setIsUsed(false);
            processor.clear();
        });

        m_progressCounter.addProgress(std::distance(split, m_futures.end()));

        m_futures.erase(split, m_futures.end());
    }

private:
    std::vector<Future> m_futures;
    ProcessorManager &m_processorManager;
    QueueInterface &m_queue;
    ProgressCounter &m_progressCounter;
    uint m_hardwareConcurrency;
    std::launch m_launchPolicy;
    bool m_isDisabled = false;
    CallDoInMainThreadAfterFinished m_callDoInMainThreadAfterFinished;
};

} // namespace ClangBackEnd
