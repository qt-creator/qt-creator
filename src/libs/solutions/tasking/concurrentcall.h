// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TASKING_CONCURRENTCALL_H
#define TASKING_CONCURRENTCALL_H

#include "tasktree.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>

QT_BEGIN_NAMESPACE

namespace Tasking {

// This class introduces the dependency to Qt::Concurrent, otherwise Tasking namespace
// is independent on Qt::Concurrent.
// Possibly, it could be placed inside Qt::Concurrent library, as a wrapper around
// QtConcurrent::run() call.

template <typename ResultType>
class ConcurrentCall
{
    Q_DISABLE_COPY_MOVE(ConcurrentCall)

public:
    ConcurrentCall() = default;
    template <typename Function, typename ...Args>
    void setConcurrentCallData(Function &&function, Args &&...args)
    {
        wrapConcurrent(std::forward<Function>(function), std::forward<Args>(args)...);
    }
    void setThreadPool(QThreadPool *pool) { m_threadPool = pool; }
    ResultType result() const
    {
        return m_future.resultCount() ? m_future.result() : ResultType();
    }
    QList<ResultType> results() const
    {
        return m_future.results();
    }
    QFuture<ResultType> future() const { return m_future; }

private:
    template <typename Function, typename ...Args>
    void wrapConcurrent(Function &&function, Args &&...args)
    {
        m_startHandler = [this, function = std::forward<Function>(function), args...] {
            QThreadPool *threadPool = m_threadPool ? m_threadPool : QThreadPool::globalInstance();
            return QtConcurrent::run(threadPool, function, args...);
        };
    }

    template <typename Function, typename ...Args>
    void wrapConcurrent(std::reference_wrapper<const Function> &&wrapper, Args &&...args)
    {
        m_startHandler = [this, wrapper = std::forward<std::reference_wrapper<const Function>>(wrapper), args...] {
            QThreadPool *threadPool = m_threadPool ? m_threadPool : QThreadPool::globalInstance();
            return QtConcurrent::run(threadPool, std::forward<const Function>(wrapper.get()),
                                     args...);
        };
    }

    template <typename T>
    friend class ConcurrentCallTaskAdapter;

    std::function<QFuture<ResultType>()> m_startHandler;
    QThreadPool *m_threadPool = nullptr;
    QFuture<ResultType> m_future;
};

template <typename ResultType>
class ConcurrentCallTaskAdapter final
{
public:
    ~ConcurrentCallTaskAdapter() {
        if (m_watcher) {
            m_watcher->cancel();
            m_watcher->waitForFinished();
        }
    }

    void operator()(ConcurrentCall<ResultType> *task, TaskInterface *iface) {
        if (!task->m_startHandler) {
            iface->reportDone(DoneResult::Error); // TODO: Add runtime assert
            return;
        }
        m_watcher.reset(new QFutureWatcher<ResultType>);
        QObject::connect(m_watcher.get(), &QFutureWatcherBase::finished, iface, [this, iface] {
            iface->reportDone(toDoneResult(!m_watcher->isCanceled()));
            m_watcher.release()->deleteLater();
        });
        task->m_future = task->m_startHandler();
        m_watcher->setFuture(task->m_future);
    }

private:
    std::unique_ptr<QFutureWatcher<ResultType>> m_watcher;
};

template <typename ResultType>
using ConcurrentCallTask = CustomTask<ConcurrentCall<ResultType>, ConcurrentCallTaskAdapter<ResultType>>;

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_CONCURRENTCALL_H
