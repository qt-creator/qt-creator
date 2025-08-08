// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TASKING_TASKTREERUNNER_H
#define TASKING_TASKTREERUNNER_H

#include "tasking_global.h"
#include "tasktree.h"

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

namespace Tasking {

// TODO: Is AbstractTaskTreeController a better name?
class TASKING_EXPORT AbstractTaskTreeRunner : public QObject
{
    Q_OBJECT

public:
    using TreeSetupHandler = std::function<void(TaskTree &)>;
    using TreeDoneHandler = std::function<void(const TaskTree &, DoneWith)>;

    virtual bool isRunning() const = 0;
    virtual void cancel() = 0;
    virtual void reset() = 0;

Q_SIGNALS:
    void aboutToStart(TaskTree *taskTree);
    void done(DoneWith result, TaskTree *taskTree);

protected:
    struct TreeData
    {
        Group recipe;
        TreeSetupHandler setupHandler;
        TreeDoneHandler doneHandler;
        CallDoneFlags callDone;
    };

    template <typename Handler>
    static TreeSetupHandler wrapTreeSetupHandler(Handler &&handler) {
        if constexpr (std::is_same_v<std::decay_t<Handler>, TreeSetupHandler>)
            return {}; // When user passed {} for the setup handler.
        // V, T stands for: [V]oid, [T]askTree
        static constexpr bool isVT = isInvocable<void, Handler, TaskTree &>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isVT || isV,
            "Tree setup handler needs to take (TaskTree &) or (void) as an argument and has to "
            "return void. The passed handler doesn't fulfill these requirements.");
        return [handler = std::forward<Handler>(handler)](TaskTree &taskTree) {
            if constexpr (isVT)
                std::invoke(handler, taskTree);
            else if constexpr (isV)
                std::invoke(handler);
        };
    }

    template <typename Handler>
    static TreeDoneHandler wrapTreeDoneHandler(Handler &&handler) {
        if constexpr (std::is_same_v<std::decay_t<Handler>, TreeDoneHandler>)
            return {}; // User passed {} for the done handler.
        // V, T, D stands for: [V]oid, [T]askTree, [D]oneWith
        static constexpr bool isVTD = isInvocable<void, Handler, const TaskTree &, DoneWith>();
        static constexpr bool isVT = isInvocable<void, Handler, const TaskTree &>();
        static constexpr bool isVD = isInvocable<void, Handler, DoneWith>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isVTD || isVT || isVD || isV,
            "Task done handler needs to take (const TaskTree &, DoneWith), (const Task &), "
            "(DoneWith) or (void) as arguments and has to return void. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler = std::forward<Handler>(handler)](const TaskTree &taskTree, DoneWith result) {
            if constexpr (isVTD)
                std::invoke(handler, taskTree, result);
            else if constexpr (isVT)
                std::invoke(handler, taskTree);
            else if constexpr (isVD)
                std::invoke(handler, result);
            else if constexpr (isV)
                std::invoke(handler);
        };
    }
};

class TASKING_EXPORT SingleTaskTreeRunner : public AbstractTaskTreeRunner
{
    Q_OBJECT

public:
    ~SingleTaskTreeRunner();

    bool isRunning() const override { return bool(m_taskTree); }

    // When task tree is running it resets the old task tree.
    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void start(const Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               CallDoneFlags callDone = CallDone::Always)
    {
        startImpl(recipe,
                  wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                  wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

    // When task tree is running it emits done(DoneWith::Cancel) synchronously.
    void cancel() override;

    // No done() signal is emitted.
    void reset() override;

private:
    void startImpl(const Group &recipe,
                   const TreeSetupHandler &setupHandler = {},
                   const TreeDoneHandler &doneHandler = {},
                   CallDoneFlags callDone = CallDone::Always);

    std::unique_ptr<TaskTree> m_taskTree;
};

class TASKING_EXPORT SequentialTaskTreeRunner : public AbstractTaskTreeRunner
{
    Q_OBJECT

public:
    SequentialTaskTreeRunner();
    ~SequentialTaskTreeRunner();

    bool isRunning() const override;

    // When task tree is running it resets the old task tree.
    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void enqueue(const Group &recipe,
                 SetupHandler &&setupHandler = {},
                 DoneHandler &&doneHandler = {},
                 CallDoneFlags callDone = CallDone::Always)
    {
        enqueueImpl({recipe,
                     wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                     wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                     callDone});
    }

    // The running task trees is canceled. Emits done(DoneWith::Cancel) signals synchronously.
    // The eunqeued tasks are removed.
    void cancel() override;
    void cancelCurrent();

    // All running task trees are deleted. No done() signal is emitted.
    // The eunqeued tasks are removed.
    void reset() override;
    void resetCurrent();

private:
    void enqueueImpl(const TreeData &data);
    void startNext();

    QList<TreeData> m_treeDataQueue;
    SingleTaskTreeRunner m_taskTreeRunner;
};

class TASKING_EXPORT ParallelTaskTreeRunner : public AbstractTaskTreeRunner
{
    Q_OBJECT

public:
    ~ParallelTaskTreeRunner();

    bool isRunning() const override { return !m_taskTrees.empty(); }

    // When task tree is running it resets the old task tree.
    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void start(const Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               CallDoneFlags callDone = CallDone::Always)
    {
        startImpl(recipe,
                  wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                  wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

    // All running task trees are canceled. Emits done(DoneWith::Cancel) signals synchronously.
    // The order of cancellations is not specified.
    void cancel() override;

    // All running task trees are deleted. No done() signal is emitted.
    void reset() override;

private:
    void startImpl(const Group &recipe,
                   const TreeSetupHandler &setupHandler = {},
                   const TreeDoneHandler &doneHandler = {},
                   CallDoneFlags callDone = CallDone::Always);

    std::unordered_map<TaskTree *, std::unique_ptr<TaskTree>> m_taskTrees;
};

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_TASKTREERUNNER_H
