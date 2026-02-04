// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBARRIERTASK_H
#define QBARRIERTASK_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/QSharedData>

QT_BEGIN_NAMESPACE

class QBarrierPrivate;

class Q_TASKTREE_EXPORT QBarrier : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QBarrier)

public:
    QBarrier(QObject *parent = nullptr);
    ~QBarrier() override;

    void setLimit(int value);
    int limit() const;

    void start();
    void advance();
    void stopWithResult(QtTaskTree::DoneResult result);

    bool isRunning() const;
    int current() const;
    std::optional<QtTaskTree::DoneResult> result() const;

Q_SIGNALS:
    void done(QtTaskTree::DoneResult success, QPrivateSignal);
};

using QBarrierTask = QCustomTask<QBarrier>;

template <int Limit = 1>
class QStartedBarrier : public QBarrier
{
public:
    static_assert(Limit > 0, "StartedBarrier's limit should be 1 or more.");
    QStartedBarrier(QObject *parent = nullptr)
        : QBarrier(parent)
    {
        setLimit(Limit);
        start();
    }
};

template <int Limit = 1>
using QStoredMultiBarrier = QtTaskTree::Storage<QStartedBarrier<Limit>>;
using QStoredBarrier = QStoredMultiBarrier<1>;

namespace QtTaskTree {

template <int Limit>
ExecutableItem barrierAwaiterTask(const QStoredMultiBarrier<Limit> &storedBarrier)
{
    return QBarrierTask([storedBarrier](QBarrier &barrier) {
        QBarrier *activeBarrier = storedBarrier.activeStorage();
        if (!activeBarrier) {
            qWarning("The barrier referenced from WaitForBarrier element "
                     "is not reachable in the running tree. "
                     "It is possible that no barrier was added to the tree, "
                     "or the barrier is not reachable from where it is referenced. "
                     "The WaitForBarrier task finishes with an error. ");
            return SetupResult::StopWithError;
        }
        const std::optional<DoneResult> result = activeBarrier->result();
        if (result.has_value()) {
            return *result == DoneResult::Success ? SetupResult::StopWithSuccess
                                                  : SetupResult::StopWithError;
        }
        QObject::connect(activeBarrier, &QBarrier::done, &barrier, &QBarrier::stopWithResult);
        return SetupResult::Continue;
    });
}

template <typename Signal>
ExecutableItem signalAwaiterTask(const typename QtPrivate::FunctionPointer<Signal>::Object *sender,
                                 Signal signal)
{
    return QBarrierTask([sender, signal](QBarrier &barrier) {
        QObject::connect(sender, signal, &barrier, &QBarrier::advance, Qt::SingleShotConnection);
    });
}

using BarrierKickerGetter = std::function<ExecutableItem(const QStoredBarrier &)>;

class WhenPrivate;

class When final
{
public:
    Q_TASKTREE_EXPORT explicit When(const BarrierKickerGetter &kicker,
                                    WorkflowPolicy policy = WorkflowPolicy::StopOnError);

    template <typename Task, typename Adapter, typename Deleter, typename Signal>
    explicit When(const QCustomTask<Task, Adapter, Deleter> &customTask, Signal signal,
                  WorkflowPolicy policy = WorkflowPolicy::StopOnError)
        : When(kickerForSignal(customTask, signal), policy)
    {}
    Q_TASKTREE_EXPORT ~When();
    Q_TASKTREE_EXPORT When(const When &other);
    Q_TASKTREE_EXPORT When(When &&other) noexcept;
    Q_TASKTREE_EXPORT When &operator=(const When &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(When)

    void swap(When &other) noexcept { d.swap(other.d); }

private:
    Q_TASKTREE_EXPORT friend Group operator>>(const When &whenItem, const Do &doItem);

    template <typename Task, typename Adapter, typename Deleter, typename Signal>
    static BarrierKickerGetter kickerForSignal(const QCustomTask<Task, Adapter, Deleter> &task,
                                               Signal signal)
    {
        return [taskHandler = task.taskHandler(), signal](const QStoredBarrier &barrier) {
            auto handler = std::move(taskHandler);
            const auto wrappedSetupHandler
                = [originalSetupHandler = std::move(handler.m_taskAdapterSetupHandler),
                   barrier, signal](void *taskAdapter) {
                const SetupResult setupResult = std::invoke(originalSetupHandler, taskAdapter);
                using TaskAdapter = typename QCustomTask<Task, Adapter, Deleter>::TaskAdapter;
                TaskAdapter *adapter = static_cast<TaskAdapter *>(taskAdapter);
                QObject::connect(adapter->task.get(), signal,
                                 barrier.activeStorage(), &QBarrier::advance);
                return setupResult;
            };
            handler.m_taskAdapterSetupHandler = std::move(wrappedSetupHandler);
            return ExecutableItem(std::move(handler));
        };
    }

    QExplicitlySharedDataPointer<WhenPrivate> d;
};

} // namespace QtTaskTree

QT_END_NAMESPACE

#endif // QBARRIERTASK_H
