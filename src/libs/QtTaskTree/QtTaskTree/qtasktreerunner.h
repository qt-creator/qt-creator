// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QTASKTREE_QTASKTREERUNNER_H
#define QTASKTREE_QTASKTREERUNNER_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/QObject>

#include <unordered_map>

QT_BEGIN_NAMESPACE

namespace QtTaskTree {

class QSingleTaskTreeRunnerPrivate;
class QSequentialTaskTreeRunnerPrivate;
class QParallelTaskTreeRunnerPrivate;

using TreeSetupHandler = std::function<void(QTaskTree &)>;
using TreeDoneHandler = std::function<void(const QTaskTree &, DoneWith)>;

template <typename Handler>
TreeSetupHandler wrapTreeSetupHandler(Handler &&handler) {
    if constexpr (std::is_same_v<q20::remove_cvref_t<Handler>, TreeSetupHandler>) {
        if (!handler)
            return {}; // User passed {} for the setup handler.
    }
    return [handler = std::forward<Handler>(handler)](QTaskTree &taskTree) {
        // V, T stands for: [V]oid, [T]askTree
        constexpr bool isVT = isInvocable<void, Handler, QTaskTree &>();
        constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isVT || isV,
                      "Tree setup handler needs to take (TaskTree &) or (void) as an argument and has to "
                      "return void. The passed handler doesn't fulfill these requirements.");
        if constexpr (isVT)
            std::invoke(handler, taskTree);
        else if constexpr (isV)
            std::invoke(handler);
    };
}

template <typename Handler>
TreeDoneHandler wrapTreeDoneHandler(Handler &&handler) {
    if constexpr (std::is_same_v<q20::remove_cvref_t<Handler>, TreeDoneHandler>) {
        if (!handler)
            return {}; // User passed {} for the done handler.
    }
    return [handler = std::forward<Handler>(handler)](const QTaskTree &taskTree, DoneWith result) {
        // V, T, D stands for: [V]oid, [T]askTree, [D]oneWith
        constexpr bool isVTD = isInvocable<void, Handler, const QTaskTree &, DoneWith>();
        constexpr bool isVT = isInvocable<void, Handler, const QTaskTree &>();
        constexpr bool isVD = isInvocable<void, Handler, DoneWith>();
        constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isVTD || isVT || isVD || isV,
                      "Task done handler needs to take (const TaskTree &, DoneWith), (const Task &), "
                      "(DoneWith) or (void) as arguments and has to return void. "
                      "The passed handler doesn't fulfill these requirements.");
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

class QSingleTaskTreeRunner final
{
    Q_DECLARE_PRIVATE(QSingleTaskTreeRunner)
    Q_DISABLE_COPY_MOVE(QSingleTaskTreeRunner)

public:
    Q_TASKTREE_EXPORT QSingleTaskTreeRunner();
    Q_TASKTREE_EXPORT ~QSingleTaskTreeRunner();

    Q_TASKTREE_EXPORT bool isRunning() const;
    Q_TASKTREE_EXPORT void cancel() ;
    Q_TASKTREE_EXPORT void reset() ;

    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void start(const Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               CallDone callDone = CallDoneFlag::Always)
    {
        startImpl(recipe,
                  wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                  wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

private:
    Q_TASKTREE_EXPORT void startImpl(const Group &recipe,
                                     const TreeSetupHandler &setupHandler,
                                     const TreeDoneHandler &doneHandler,
                                     CallDone callDone);

    std::unique_ptr<QSingleTaskTreeRunnerPrivate> d_ptr;
};

class QSequentialTaskTreeRunner final
{
    Q_DECLARE_PRIVATE(QSequentialTaskTreeRunner)
    Q_DISABLE_COPY_MOVE(QSequentialTaskTreeRunner)

public:
    Q_TASKTREE_EXPORT QSequentialTaskTreeRunner();
    Q_TASKTREE_EXPORT ~QSequentialTaskTreeRunner();

    Q_TASKTREE_EXPORT bool isRunning() const;
    Q_TASKTREE_EXPORT void cancel();
    Q_TASKTREE_EXPORT void reset();

    Q_TASKTREE_EXPORT void cancelCurrent();
    Q_TASKTREE_EXPORT void resetCurrent();

    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void enqueue(const Group &recipe,
                 SetupHandler &&setupHandler = {},
                 DoneHandler &&doneHandler = {},
                 CallDone callDone = CallDoneFlag::Always)
    {
        enqueueImpl(recipe,
                    wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                    wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                    callDone);
    }

private:
    Q_TASKTREE_EXPORT void enqueueImpl(const Group &recipe,
                                       const TreeSetupHandler &setupHandler,
                                       const TreeDoneHandler &doneHandler,
                                       CallDone callDone);

    std::unique_ptr<QSequentialTaskTreeRunnerPrivate> d_ptr;
};

class QParallelTaskTreeRunner final
{
    Q_DECLARE_PRIVATE(QParallelTaskTreeRunner)
    Q_DISABLE_COPY_MOVE(QParallelTaskTreeRunner)

public:
    Q_TASKTREE_EXPORT QParallelTaskTreeRunner();
    Q_TASKTREE_EXPORT ~QParallelTaskTreeRunner();

    Q_TASKTREE_EXPORT bool isRunning() const;
    Q_TASKTREE_EXPORT void cancel();
    Q_TASKTREE_EXPORT void reset();

    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void start(const Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               CallDone callDone = CallDoneFlag::Always)
    {
        startImpl(recipe,
                  wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                  wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

private:
    Q_TASKTREE_EXPORT void startImpl(const Group &recipe,
                                     const TreeSetupHandler &setupHandler,
                                     const TreeDoneHandler &doneHandler,
                                     CallDone callDone);

    std::unique_ptr<QParallelTaskTreeRunnerPrivate> d_ptr;
};

template <typename Key>
class QMappedTaskTreeRunner final
{
    Q_DISABLE_COPY_MOVE(QMappedTaskTreeRunner)

public:
    QMappedTaskTreeRunner() = default;

    ~QMappedTaskTreeRunner() = default;

    bool isRunning() const { return !m_taskTrees.empty(); }

    void cancel()
    {
        while (!m_taskTrees.empty())
            m_taskTrees.begin()->second->cancel();
    }

    void reset() { m_taskTrees.clear(); }

    bool isKeyRunning(const Key &key) const { return m_taskTrees.find(key) != m_taskTrees.end(); }

    void cancelKey(const Key &key)
    {
        if (const auto it = m_taskTrees.find(key); it != m_taskTrees.end())
            it->second->cancel();
    }

    void resetKey(const Key &key)
    {
        if (const auto it = m_taskTrees.find(key); it != m_taskTrees.end())
            m_taskTrees.erase(it);
    }

    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void start(const Key &key, const Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               CallDone callDone = CallDoneFlag::Always)
    {
        startImpl(key, recipe,
                  wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                  wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

private:
    void startImpl(const Key &key, const Group &recipe,
                   const TreeSetupHandler &setupHandler,
                   const TreeDoneHandler &doneHandler,
                   CallDone callDone)
    {
        QTaskTree *taskTree = new QTaskTree(recipe);
        QObject::connect(taskTree, &QTaskTree::done, taskTree,
                         [this, key, doneHandler, callDone](DoneWith result) {
            const auto it = m_taskTrees.find(key);
            QTaskTree *runningTaskTree = it->second.release();
            runningTaskTree->deleteLater();
            m_taskTrees.erase(it);
            if (doneHandler && shouldCallDone(callDone, result))
                doneHandler(*runningTaskTree, result);
        });
        m_taskTrees[key].reset(taskTree);
        if (setupHandler)
            setupHandler(*taskTree);
        taskTree->start();
    }

    std::unordered_map<Key, std::unique_ptr<QTaskTree>> m_taskTrees;
};

} // namespace QtTaskTree

QT_END_NAMESPACE

#endif // QTASKTREE_QTASKTREERUNNER_H
