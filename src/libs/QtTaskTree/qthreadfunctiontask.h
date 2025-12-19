// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTHREADFUNCTIONTASK_H
#define QTHREADFUNCTIONTASK_H

#include <QtTaskTree/qtasktree.h>

#include <QtConcurrent/QtConcurrent>

QT_REQUIRE_CONFIG(concurrent);

QT_BEGIN_NAMESPACE

class QThreadFunctionBasePrivate;

class Q_TASKTREE_EXPORT QThreadFunctionBase : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QThreadFunctionBase)

public:
    QThreadFunctionBase(QObject *parent = nullptr);
    ~QThreadFunctionBase() override;

    void setThreadPool(QThreadPool *pool);
    QThreadPool *threadPool() const;

    void setAutoDelayedSync(bool on);
    bool isAutoDelayedSync() const;

    void setSyncSkipped(bool on);
    bool isSyncSkipped() const;

    static void syncAll();

Q_SIGNALS:
    void started();
    void done(QtTaskTree::DoneResult result);
    void resultReadyAt(int index);
    void resultsReadyAt(int beginIndex, int endIndex);
    void progressRangeChanged(int minimum, int maximum);
    void progressValueChanged(int value);
    void progressTextChanged(const QString &text);

protected:
    void storeFuture(const QFuture<void> &future);
};

template <typename ResultType>
class QThreadFunction : public QThreadFunctionBase
{
    Q_DISABLE_COPY_MOVE(QThreadFunction)

public:
    explicit QThreadFunction(QObject *parent = nullptr)
        : QThreadFunctionBase(parent)
    {
        connect(&m_watcher, &QFutureWatcherBase::finished, this, [this] {
            Q_EMIT done(QtTaskTree::toDoneResult(!m_watcher.isCanceled()));
        });
        connect(&m_watcher, &QFutureWatcherBase::resultReadyAt,
                this, &QThreadFunctionBase::resultReadyAt);
        connect(&m_watcher, &QFutureWatcherBase::resultsReadyAt,
                this, &QThreadFunctionBase::resultsReadyAt);
        connect(&m_watcher, &QFutureWatcherBase::progressValueChanged,
                this, &QThreadFunctionBase::progressValueChanged);
        connect(&m_watcher, &QFutureWatcherBase::progressRangeChanged,
                this, &QThreadFunctionBase::progressRangeChanged);
        connect(&m_watcher, &QFutureWatcherBase::progressTextChanged,
                this, &QThreadFunctionBase::progressTextChanged);
    }

    ~QThreadFunction() override
    {
        disconnect(&m_watcher);
    }

    template <typename Function, typename ...Args>
    void setThreadFunctionData(Function &&function, Args &&...args)
    {
        wrapConcurrent(std::forward<Function>(function), std::forward<Args>(args)...);
    }

    bool isDone() const { return m_watcher.isFinished(); }
    bool isResultAvailable() const { return future().resultCount(); }

    QFuture<ResultType> future() const { return m_watcher.future(); }
    ResultType result() const { return m_watcher.result(); }
    ResultType takeResult() const { return future().takeResult(); }
    ResultType resultAt(int index) const { return m_watcher.resultAt(index); }
    QList<ResultType> results() const { return future().results(); }

    void start()
    {
        if (future().isRunning()) {
            qWarning("QThreadFunction: Can't start, the future is still running.");
            return;
        }

        if (!m_startHandler) {
            qWarning("QThreadFunction: No start handler specified.");
            Q_EMIT done(QtTaskTree::DoneResult::Error);
            return;
        }

        m_watcher.setFuture(m_startHandler());
        storeFuture(QFuture<void>(future()));
        Q_EMIT started();
    }

#ifdef Q_QDOC
    void setThreadPool(QThreadPool *pool);
    QThreadPool *threadPool() const;

    void setAutoDelayedSync(bool on);
    bool isAutoDelayedSync() const;

Q_SIGNALS:
    void started();
    void done(QtTaskTree::DoneResult result);
    void resultReadyAt(int index);
    void resultsReadyAt(int beginIndex, int endIndex);
    void progressRangeChanged(int min, int max);
    void progressValueChanged(int value);
    void progressTextChanged(const QString &text);
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
};

template <typename ResultType>
using QThreadFunctionTask = QCustomTask<QThreadFunction<ResultType>>;

QT_END_NAMESPACE

#endif // QTHREADFUNCTIONTASK_H
