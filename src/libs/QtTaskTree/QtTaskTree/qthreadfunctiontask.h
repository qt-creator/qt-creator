// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QTASKTREE_QTHREADFUNCTIONTASK_H
#define QTASKTREE_QTHREADFUNCTIONTASK_H

#include <QtTaskTree/qtasktree.h>

#include <QtConcurrent/QtConcurrent>

QT_REQUIRE_CONFIG(concurrent);

QT_BEGIN_NAMESPACE

namespace QtTaskTree {

class QThreadFunctionBasePrivate;

class Q_TASKTREE_EXPORT QThreadFunctionBase
{
    Q_DECLARE_PRIVATE(QThreadFunctionBase)
    Q_DISABLE_COPY_MOVE(QThreadFunctionBase)

public:
    QThreadFunctionBase();
    virtual ~QThreadFunctionBase();

    void setThreadPool(QThreadPool *pool);
    QThreadPool *threadPool() const;

    void setAutoDelayedSync(bool on);
    bool isAutoDelayedSync() const;

    void setSyncSkipped(bool on);
    bool isSyncSkipped() const;

    static void syncAll();

protected:
    void storeFuture(const QFuture<void> &future);

    std::unique_ptr<QThreadFunctionBasePrivate> d_ptr;
};

template <typename ResultType>
class QThreadFunction final : public QThreadFunctionBase
{
    Q_DISABLE_COPY_MOVE(QThreadFunction)

public:
    QThreadFunction() : QThreadFunctionBase() {}

    ~QThreadFunction() override
    {
        m_watcher.disconnect();
    }

    template <typename Function, typename ...Args>
    void setThreadFunctionData(Function &&function, Args &&...args)
    {
        wrapConcurrent(std::forward<Function>(function), std::forward<Args>(args)...);
    }

    bool isDone() const { return m_watcher.isFinished(); }
    bool isResultAvailable() const { return future().resultCount(); }

    QFutureWatcher<ResultType> *futureWatcher() { return &m_watcher; }
    const QFutureWatcher<ResultType> *futureWatcher() const { return &m_watcher; }
    QFuture<ResultType> future() const { return m_watcher.future(); }
    ResultType result() const { return m_watcher.result(); }
    ResultType takeResult() const { return future().takeResult(); }
    ResultType resultAt(int index) const { return m_watcher.resultAt(index); }
    QList<ResultType> results() const { return future().results(); }

#ifdef Q_QDOC
    void setThreadPool(QThreadPool *pool);
    QThreadPool *threadPool() const;

    void setAutoDelayedSync(bool on);
    bool isAutoDelayedSync() const;
#endif

private:
    template <typename Function, typename ...Args>
    void wrapConcurrent(Function &&function, Args &&...args)
    {
        m_startHandler = [this, function = std::forward<Function>(function), args...] {
            QThreadPool *pool = threadPool() ? threadPool() : QThreadPool::globalInstance();
            return QtConcurrent::run(pool, function, args...);
        };
    }

    template <typename Function, typename ...Args>
    void wrapConcurrent(std::reference_wrapper<const Function> &&wrapper, Args &&...args)
    {
        m_startHandler = [this, wrapper = std::forward<std::reference_wrapper<const Function>>(wrapper), args...] {
            QThreadPool *pool = threadPool() ? threadPool() : QThreadPool::globalInstance();
            return QtConcurrent::run(pool, std::forward<const Function>(wrapper.get()),
                                     args...);
        };
    }

    std::function<QFuture<ResultType>()> m_startHandler;
    QFutureWatcher<ResultType> m_watcher;

    template <typename T>
    friend class QThreadFunctionTaskAdapter;
};

template <typename ResultType>
class QThreadFunctionTaskAdapter
{
public:
    void operator()(QThreadFunction<ResultType> *task, QTaskInterface *iface) const
    {
        if (task->future().isRunning()) {
            qWarning("QThreadFunction: Can't start, the future is still running.");
            return;
        }

        if (!task->m_startHandler) {
            qWarning("QThreadFunction: No start handler specified.");
            iface->reportDone(DoneResult::Error);
            return;
        }

        QObject::connect(&task->m_watcher, &QFutureWatcherBase::finished, iface, [task, iface] {
            iface->reportDone(toDoneResult(!task->m_watcher.isCanceled()));
        }, Qt::SingleShotConnection);

        task->m_watcher.setFuture(task->m_startHandler());
        task->storeFuture(QFuture<void>(task->future()));
    }
};


template <typename ResultType>
using QThreadFunctionTask = QCustomTask<QThreadFunction<ResultType>,
                                        QThreadFunctionTaskAdapter<ResultType>>;

} // namespace QtTaskTree

QT_END_NAMESPACE

#endif // QTASKTREE_QTHREADFUNCTIONTASK_H
