// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "futuresynchronizer.h"
#include "qtcassert.h"
#include "tasktree.h"

#include <QFutureWatcher>
#include <QtConcurrent>

namespace Utils {

QTCREATOR_UTILS_EXPORT QThreadPool *asyncThreadPool(QThread::Priority priority);

template <typename Function, typename ...Args>
auto asyncRun(QThreadPool *threadPool, QThread::Priority priority,
              Function &&function, Args &&...args)
{
    QThreadPool *pool = threadPool ? threadPool : asyncThreadPool(priority);
    return QtConcurrent::run(pool, std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename Function, typename ...Args>
auto asyncRun(QThread::Priority priority, Function &&function, Args &&...args)
{
    return asyncRun<Function, Args...>(nullptr, priority,
                    std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename Function, typename ...Args>
auto asyncRun(QThreadPool *threadPool, Function &&function, Args &&...args)
{
    return asyncRun<Function, Args...>(threadPool, QThread::InheritPriority,
                    std::forward<Function>(function), std::forward<Args>(args)...);
}

template <typename Function, typename ...Args>
auto asyncRun(Function &&function, Args &&...args)
{
    return asyncRun<Function, Args...>(nullptr, QThread::InheritPriority,
                                       std::forward<Function>(function), std::forward<Args>(args)...);
}

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
            return asyncRun(m_threadPool, m_priority, function, args...);
        };
    }

    template <typename Function, typename ...Args>
    void wrapConcurrent(std::reference_wrapper<const Function> &&wrapper, Args &&...args)
    {
        m_startHandler = [=] {
            return asyncRun(m_threadPool, m_priority, std::forward<const Function>(wrapper.get()),
                            args...);
        };
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
