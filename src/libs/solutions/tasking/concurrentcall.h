// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tasking_global.h"

#include "tasktree.h"

#include <QtConcurrent>

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
        return wrapConcurrent(std::forward<Function>(function), std::forward<Args>(args)...);
    }
    void setThreadPool(QThreadPool *pool) { m_threadPool = pool; }
    ResultType result() const
    {
        return m_future.resultCount() ? m_future.result() : ResultType();
    }
    QFuture<ResultType> future() const { return m_future; }

private:
    template <typename Function, typename ...Args>
    void wrapConcurrent(Function &&function, Args &&...args)
    {
        m_startHandler = [=] {
            if (m_threadPool)
                return QtConcurrent::run(m_threadPool, function, args...);
            return QtConcurrent::run(function, args...);
        };
    }

    template <typename Function, typename ...Args>
    void wrapConcurrent(std::reference_wrapper<const Function> &&wrapper, Args &&...args)
    {
        m_startHandler = [=] {
            if (m_threadPool) {
                return QtConcurrent::run(m_threadPool,
                                         std::forward<const Function>(wrapper.get()), args...);
            }
            return QtConcurrent::run(std::forward<const Function>(wrapper.get()), args...);
        };
    }

    template <typename T>
    friend class ConcurrentCallTaskAdapter;

    std::function<QFuture<ResultType>()> m_startHandler;
    QThreadPool *m_threadPool = nullptr;
    QFuture<ResultType> m_future;
};

template <typename ResultType>
class ConcurrentCallTaskAdapter : public TaskAdapter<ConcurrentCall<ResultType>>
{
public:
    ~ConcurrentCallTaskAdapter() {
        if (m_watcher) {
            m_watcher->cancel();
            m_watcher->waitForFinished();
        }
    }

    void start() {
        if (!this->task()->m_startHandler) {
            emit this->done(false); // TODO: Add runtime assert
            return;
        }
        m_watcher.reset(new QFutureWatcher<ResultType>);
        this->connect(m_watcher.get(), &QFutureWatcherBase::finished, this, [this] {
            emit this->done(!m_watcher->isCanceled());
            m_watcher.release()->deleteLater();
        });
        this->task()->m_future = this->task()->m_startHandler();
        m_watcher->setFuture(this->task()->m_future);
    }

private:
    std::unique_ptr<QFutureWatcher<ResultType>> m_watcher;
};

template <typename T>
using ConcurrentCallTask = CustomTask<ConcurrentCallTaskAdapter<T>>;

} // namespace Tasking
