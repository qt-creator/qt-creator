// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "futuresynchronizer.h"
#include "qtcassert.h"
#include "runextensions.h"
#include "tasktree.h"

#include <QFutureWatcher>
#include <QtConcurrent>

namespace Utils {

QTCREATOR_UTILS_EXPORT QThreadPool *asyncThreadPool();

class QTCREATOR_UTILS_EXPORT AsyncTaskBase : public QObject
{
    Q_OBJECT

signals:
    void started();
    void done();
    void resultReadyAt(int index);
};

template <typename ResultType>
class AsyncTask : public AsyncTaskBase
{
public:
    AsyncTask() {
        connect(&m_watcher, &QFutureWatcherBase::finished, this, &AsyncTaskBase::done);
        connect(&m_watcher, &QFutureWatcherBase::resultReadyAt,
                this, &AsyncTaskBase::resultReadyAt);
    }
    ~AsyncTask()
    {
        if (isDone())
            return;

        m_watcher.cancel();
        if (!m_synchronizer)
            m_watcher.waitForFinished();
    }

    template <typename Function, typename ...Args>
    void setConcurrentCallData(Function &&function, Args &&...args)
    {
        return wrapConcurrent(std::forward<Function>(function), std::forward<Args>(args)...);
    }

    template <typename Function, typename ...Args>
    void setAsyncCallData(const Function &function, const Args &...args)
    {
        m_startHandler = [=] {
            return Utils::runAsync(m_threadPool, m_priority, function, args...);
        };
    }
    void setFutureSynchronizer(FutureSynchronizer *synchorizer) { m_synchronizer = synchorizer; }
    void setThreadPool(QThreadPool *pool) { m_threadPool = pool; }
    void setPriority(QThread::Priority priority) { m_priority = priority; }

    void start()
    {
        QTC_ASSERT(m_startHandler, qWarning("No start handler specified."); return);
        m_watcher.setFuture(m_startHandler());
        emit started();
        if (m_synchronizer)
            m_synchronizer->addFuture(m_watcher.future());
    }

    bool isDone() const { return m_watcher.isFinished(); }
    bool isCanceled() const { return m_watcher.isCanceled(); }

    QFuture<ResultType> future() const { return m_watcher.future(); }
    ResultType result() const { return m_watcher.result(); }
    ResultType resultAt(int index) const { return m_watcher.resultAt(index); }
    QList<ResultType> results() const { return future().results(); }
    bool isResultAvailable() const { return future().resultCount(); }

private:
    template <typename Function, typename ...Args>
    void wrapConcurrent(Function &&function, Args &&...args)
    {
        m_startHandler = [=] {
            return callConcurrent(function, args...);
        };
    }

    template <typename Function, typename ...Args>
    void wrapConcurrent(std::reference_wrapper<const Function> &&wrapper, Args &&...args)
    {
        m_startHandler = [=] {
            return callConcurrent(std::forward<const Function>(wrapper.get()), args...);
        };
    }

    template <typename Function, typename ...Args>
    auto callConcurrent(Function &&function, Args &&...args)
    {
        // Notice: we can't just call:
        //
        // return QtConcurrent::run(function, args...);
        //
        // since there is no way of passing m_priority there.
        // There is an overload with thread pool, however, there is no overload with priority.
        //
        // Below implementation copied from QtConcurrent::run():
        QThreadPool *threadPool = m_threadPool ? m_threadPool : asyncThreadPool();
        QtConcurrent::DecayedTuple<Function, Args...>
            tuple{std::forward<Function>(function), std::forward<Args>(args)...};
        return QtConcurrent::TaskResolver<std::decay_t<Function>, std::decay_t<Args>...>
            ::run(std::move(tuple), QtConcurrent::TaskStartParameters{threadPool, m_priority});
    }

    using StartHandler = std::function<QFuture<ResultType>()>;
    StartHandler m_startHandler;
    FutureSynchronizer *m_synchronizer = nullptr;
    QThreadPool *m_threadPool = nullptr;
    QThread::Priority m_priority = QThread::InheritPriority;
    QFutureWatcher<ResultType> m_watcher;
};

template <typename ResultType>
class AsyncTaskAdapter : public Tasking::TaskAdapter<AsyncTask<ResultType>>
{
public:
    AsyncTaskAdapter() {
        this->connect(this->task(), &AsyncTaskBase::done, this, [this] {
            emit this->done(!this->task()->isCanceled());
        });
    }
    void start() final { this->task()->start(); }
};

} // namespace Utils

QTC_DECLARE_CUSTOM_TEMPLATE_TASK(Async, AsyncTaskAdapter);
