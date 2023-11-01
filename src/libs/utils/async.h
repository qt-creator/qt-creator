// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "futuresynchronizer.h"
#include "qtcassert.h"

#include <solutions/tasking/tasktree.h>

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

/*!
    Adds a handler for when a result is ready.
    This creates a new QFutureWatcher. Do not use if you intend to react on multiple conditions
    or create a QFutureWatcher already for other reasons.
*/
template <typename R, typename T>
const QFuture<T> &onResultReady(const QFuture<T> &future, R *receiver, void(R::*member)(const T &))
{
    auto watcher = new QFutureWatcher<T>();
    QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
    QObject::connect(watcher, &QFutureWatcherBase::resultReadyAt, receiver, [=](int index) {
        (receiver->*member)(watcher->future().resultAt(index));
    });
    watcher->setFuture(future);
    return future;
}

/*!
    Adds a handler for when a result is ready. The guard object determines the lifetime of
    the connection.
    This creates a new QFutureWatcher. Do not use if you intend to react on multiple conditions
    or create a QFutureWatcher already for other reasons.
*/
template <typename T, typename Function>
const QFuture<T> &onResultReady(const QFuture<T> &future, QObject *guard, Function f)
{
    auto watcher = new QFutureWatcher<T>();
    QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
    QObject::connect(watcher, &QFutureWatcherBase::resultReadyAt, guard, [f, watcher](int index) {
        f(watcher->future().resultAt(index));
    });
    watcher->setFuture(future);
    return future;
}

/*!
    Adds a handler for when the future is finished.
    This creates a new QFutureWatcher. Do not use if you intend to react on multiple conditions
    or create a QFutureWatcher already for other reasons.
*/
template<typename R, typename T>
const QFuture<T> &onFinished(const QFuture<T> &future,
                             R *receiver, void (R::*member)(const QFuture<T> &))
{
    auto watcher = new QFutureWatcher<T>();
    QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
    QObject::connect(watcher, &QFutureWatcherBase::finished, receiver,
                     [=] { (receiver->*member)(watcher->future()); });
    watcher->setFuture(future);
    return future;
}

/*!
    Adds a handler for when the future is finished. The guard object determines the lifetime of
    the connection.
    This creates a new QFutureWatcher. Do not use if you intend to react on multiple conditions
    or create a QFutureWatcher already for other reasons.
*/
template<typename T, typename Function>
const QFuture<T> &onFinished(const QFuture<T> &future, QObject *guard, Function f)
{
    auto watcher = new QFutureWatcher<T>();
    QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
    QObject::connect(watcher, &QFutureWatcherBase::finished, guard, [f, watcher] {
        f(watcher->future());
    });
    watcher->setFuture(future);
    return future;
}

class QTCREATOR_UTILS_EXPORT AsyncBase : public QObject
{
    Q_OBJECT

signals:
    void started();
    void done();
    void resultReadyAt(int index);
};

template <typename ResultType>
class Async : public AsyncBase
{
public:
    Async() {
        connect(&m_watcher, &QFutureWatcherBase::finished, this, &AsyncBase::done);
        connect(&m_watcher, &QFutureWatcherBase::resultReadyAt, this, &AsyncBase::resultReadyAt);
    }
    ~Async()
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
class AsyncTaskAdapter : public Tasking::TaskAdapter<Async<ResultType>>
{
public:
    AsyncTaskAdapter() {
        this->connect(this->task(), &AsyncBase::done, this, [this] {
            emit this->done(!this->task()->isCanceled());
        });
    }
    void start() final { this->task()->start(); }
};

template <typename T>
using AsyncTask = Tasking::CustomTask<AsyncTaskAdapter<T>>;

} // namespace Utils
