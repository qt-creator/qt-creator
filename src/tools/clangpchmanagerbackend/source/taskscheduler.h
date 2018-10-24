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

template <typename ProcessorManager,
          typename Task>
class TaskScheduler : public TaskSchedulerInterface<Task>
{
public:
    using ProcessorInterface = typename ProcessorManager::Processor;
    using Future = std::future<ProcessorInterface&>;

    TaskScheduler(ProcessorManager &processorManager,
                  QueueInterface &queue,
                  uint hardwareConcurrency,
                  std::launch launchPolicy = std::launch::async)
        : m_processorManager(processorManager),
          m_queue(queue),
          m_hardwareConcurrency(hardwareConcurrency),
          m_launchPolicy(launchPolicy)
    {}

    void addTasks(std::vector<Task> &&tasks)
    {
        for (auto &task : tasks) {
            auto callWrapper = [&, task=std::move(task)] (auto processor)
                    -> ProcessorInterface& {
                task(processor.get());
                executeInLoop([&] {
                    m_queue.processEntries();
                });

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

    uint freeSlots()
    {
        removeFinishedFutures();

        if (m_isDisabled)
            return 0;

        return uint(std::max(int(m_hardwareConcurrency) - int(m_futures.size()), 0));
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

        std::for_each(split, m_futures.end(), [] (Future &future) {
            ProcessorInterface &processor = future.get();
            processor.doInMainThreadAfterFinished();
            processor.setIsUsed(false);
            processor.clear();
        });

        m_futures.erase(split, m_futures.end());
    }

    #if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    template <typename CallableType>
    class CallableEvent : public QEvent {
    public:
       using Callable = std::decay_t<CallableType>;
       CallableEvent(Callable &&callable)
           : QEvent(QEvent::None),
             callable(std::move(callable))
       {}
       CallableEvent(const Callable &callable)
           : QEvent(QEvent::None),
             callable(callable)
       {}

       ~CallableEvent()
       {
           callable();
       }
    public:
       Callable callable;
    };

    template <typename Callable>
    void executeInLoop(Callable &&callable, QObject *object = QCoreApplication::instance()) {
       if (QThread *thread = qobject_cast<QThread*>(object))
           object = QAbstractEventDispatcher::instance(thread);

       QCoreApplication::postEvent(object,
                                   new CallableEvent<Callable>(std::forward<Callable>(callable)),
                                   Qt::HighEventPriority);
    }
    #else
    template <typename Callable>
    void executeInLoop(Callable &&callable, QObject *object = QCoreApplication::instance()) {
       if (QThread *thread = qobject_cast<QThread*>(object))
           object = QAbstractEventDispatcher::instance(thread);

       QMetaObject::invokeMethod(object, std::forward<Callable>(callable));
    }
    #endif

private:
    std::vector<Future> m_futures;
    ProcessorManager &m_processorManager;
    QueueInterface &m_queue;
    uint m_hardwareConcurrency;
    std::launch m_launchPolicy;
    bool m_isDisabled = false;
};

} // namespace ClangBackEnd
