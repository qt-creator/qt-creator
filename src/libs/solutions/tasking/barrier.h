// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TASKING_BARRIER_H
#define TASKING_BARRIER_H

#include "tasking_global.h"

#include "tasktree.h"

QT_BEGIN_NAMESPACE

namespace Tasking {

class TASKING_EXPORT Barrier : public QObject
{
    Q_OBJECT

public:
    void setLimit(int value);
    int limit() const { return m_limit; }

    void start();
    void advance(); // If limit reached, stops with true
    void stopWithResult(DoneResult result); // Ignores limit

    bool isRunning() const { return m_current >= 0; }
    int current() const { return m_current; }
    std::optional<DoneResult> result() const { return m_result; }

Q_SIGNALS:
    void done(DoneResult success, QPrivateSignal);

private:
    std::optional<DoneResult> m_result = {};
    int m_limit = 1;
    int m_current = -1;
};

using BarrierTask = CustomTask<Barrier>;

template <int Limit = 1>
class StartedBarrier final : public Barrier
{
public:
    static_assert(Limit > 0, "StartedBarrier's limit should be 1 or more.");
    StartedBarrier()
    {
        setLimit(Limit);
        start();
    }
};

template <int Limit = 1>
using StoredMultiBarrier = Storage<StartedBarrier<Limit>>;
using StoredBarrier = StoredMultiBarrier<1>;

template <int Limit>
ExecutableItem waitForBarrierTask(const StoredMultiBarrier<Limit> &storedBarrier)
{
    return BarrierTask([storedBarrier](Barrier &barrier) {
        Barrier *activeBarrier = storedBarrier.activeStorage();
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
        QObject::connect(activeBarrier, &Barrier::done, &barrier, &Barrier::stopWithResult);
        return SetupResult::Continue;
    });
}

template <typename Signal>
ExecutableItem signalAwaiter(const typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal)
{
    return BarrierTask([sender, signal](Barrier &barrier) {
        QObject::connect(sender, signal, &barrier, &Barrier::advance, Qt::SingleShotConnection);
    });
}

using BarrierKickerGetter = std::function<ExecutableItem(const StoredBarrier &)>;

class TASKING_EXPORT When final
{
public:
    explicit When(const BarrierKickerGetter &kicker,
                  WorkflowPolicy policy = WorkflowPolicy::StopOnError)
        : m_barrierKicker(kicker)
        , m_workflowPolicy(policy)
    {}

    template <typename Task, typename Adapter, typename Deleter, typename Signal>
    explicit When(const CustomTask<Task, Adapter, Deleter> &customTask, Signal signal,
                  WorkflowPolicy policy = WorkflowPolicy::StopOnError)
        : When(kickerForSignal(customTask, signal), policy)
    {}

private:
    template <typename Task, typename Adapter, typename Deleter, typename Signal>
    BarrierKickerGetter kickerForSignal(const CustomTask<Task, Adapter, Deleter> &customTask, Signal signal)
    {
        return [taskHandler = customTask.m_taskHandler, signal](const StoredBarrier &barrier) {
            auto handler = std::move(taskHandler);
            const auto wrappedSetupHandler = [originalSetupHandler = std::move(handler.m_taskAdapterSetupHandler),
                                              barrier, signal](void *taskAdapter) {
                const SetupResult setupResult = std::invoke(originalSetupHandler, taskAdapter);
                using TaskAdapter = typename CustomTask<Task, Adapter, Deleter>::TaskAdapter;
                TaskAdapter *adapter = static_cast<TaskAdapter *>(taskAdapter);
                QObject::connect(adapter->task.get(), signal, barrier.activeStorage(), &Barrier::advance);
                return setupResult;
            };
            handler.m_taskAdapterSetupHandler = std::move(wrappedSetupHandler);
            return ExecutableItem(std::move(handler));
        };
    }

    TASKING_EXPORT friend Group operator>>(const When &whenItem, const Do &doItem);

    BarrierKickerGetter m_barrierKicker;
    WorkflowPolicy m_workflowPolicy = WorkflowPolicy::StopOnError;
};

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_BARRIER_H
