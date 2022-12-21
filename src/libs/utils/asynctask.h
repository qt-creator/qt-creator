// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "futuresynchronizer.h"
#include "qtcassert.h"
#include "runextensions.h"
#include "tasktree.h"

#include <QFutureWatcher>

namespace Utils {

class QTCREATOR_UTILS_EXPORT AsyncTaskBase : public QObject
{
    Q_OBJECT

signals:
    void started();
    void done();
};

template <typename ResultType>
class AsyncTask : public AsyncTaskBase
{
public:
    AsyncTask() { connect(&m_watcher, &QFutureWatcherBase::finished, this, &AsyncTaskBase::done); }
    ~AsyncTask()
    {
        if (isDone())
            return;

        m_watcher.cancel();
        if (!m_synchronizer)
            m_watcher.waitForFinished();
    }

    using StartHandler = std::function<QFuture<ResultType>()>;

    template <typename Function, typename ...Args>
    void setAsyncCallData(const Function &function, const Args &...args)
    {
        m_startHandler = [=] {
            return Internal::runAsync_internal(m_threadPool, m_stackSize, m_priority,
                                               function, args...);
        };
    }
    void setFutureSynchronizer(FutureSynchronizer *synchorizer) { m_synchronizer = synchorizer; }
    void setThreadPool(QThreadPool *pool) { m_threadPool = pool; }
    void setStackSizeInBytes(const StackSizeInBytes &size) { m_stackSize = size; }
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
    ResultType result() const { return m_watcher.result(); } // TODO: warn when isRunning?
    QList<ResultType> results() const { return m_watcher.future().results(); }

private:
    StartHandler m_startHandler;
    FutureSynchronizer *m_synchronizer = nullptr;
    QThreadPool *m_threadPool = nullptr;
    QThread::Priority m_priority = QThread::InheritPriority;
    StackSizeInBytes m_stackSize = {};
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
