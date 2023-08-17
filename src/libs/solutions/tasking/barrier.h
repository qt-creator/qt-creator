// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tasking_global.h"

#include "tasktree.h"

namespace Tasking {

class TASKING_EXPORT Barrier final : public QObject
{
    Q_OBJECT

public:
    void setLimit(int value);
    int limit() const { return m_limit; }

    void start();
    void advance(); // If limit reached, stops with true
    void stopWithResult(bool success); // Ignores limit

    bool isRunning() const { return m_current >= 0; }
    int current() const { return m_current; }
    std::optional<bool> result() const { return m_result; }

signals:
    void done(bool success);

private:
    std::optional<bool> m_result = {};
    int m_limit = 1;
    int m_current = -1;
};

class TASKING_EXPORT BarrierTaskAdapter : public TaskAdapter<Barrier>
{
public:
    BarrierTaskAdapter() { connect(task(), &Barrier::done, this, &TaskInterface::done); }
    void start() final { task()->start(); }
};

using BarrierTask = CustomTask<BarrierTaskAdapter>;

template <int Limit = 1>
class SharedBarrier
{
public:
    static_assert(Limit > 0, "SharedBarrier's limit should be 1 or more.");
    SharedBarrier() : m_barrier(new Barrier) {
        m_barrier->setLimit(Limit);
        m_barrier->start();
    }
    Barrier *barrier() const { return m_barrier.get(); }

private:
    std::shared_ptr<Barrier> m_barrier;
};

template <int Limit = 1>
using MultiBarrier = TreeStorage<SharedBarrier<Limit>>;

// Can't write: "MultiBarrier barrier;". Only "MultiBarrier<> barrier;" would work.
// Can't have one alias with default type in C++17, getting the following error:
// alias template deduction only available with C++20.
using SingleBarrier = MultiBarrier<1>;

template <int Limit>
GroupItem waitForBarrierTask(const MultiBarrier<Limit> &sharedBarrier)
{
    return BarrierTask([sharedBarrier](Barrier &barrier) {
        SharedBarrier<Limit> *activeBarrier = sharedBarrier.activeStorage();
        if (!activeBarrier) {
            qWarning("The barrier referenced from WaitForBarrier element "
                     "is not reachable in the running tree. "
                     "It is possible that no barrier was added to the tree, "
                     "or the storage is not reachable from where it is referenced. "
                     "The WaitForBarrier task finishes with an error. ");
            return SetupResult::StopWithError;
        }
        Barrier *activeSharedBarrier = activeBarrier->barrier();
        const std::optional<bool> result = activeSharedBarrier->result();
        if (result.has_value())
            return result.value() ? SetupResult::StopWithDone : SetupResult::StopWithError;
        QObject::connect(activeSharedBarrier, &Barrier::done, &barrier, &Barrier::stopWithResult);
        return SetupResult::Continue;
    });
}

} // namespace Tasking
