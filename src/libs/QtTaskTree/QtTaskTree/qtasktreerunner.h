// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTASKTREERUNNER_H
#define QTASKTREERUNNER_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/QObject>

#include <unordered_map>

QT_BEGIN_NAMESPACE

class QAbstractTaskTreeRunnerPrivate;
class QSingleTaskTreeRunnerPrivate;
class QSequentialTaskTreeRunnerPrivate;
class QParallelTaskTreeRunnerPrivate;

class Q_TASKTREE_EXPORT QAbstractTaskTreeRunner : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstractTaskTreeRunner)

public:
    using TreeSetupHandler = std::function<void(QTaskTree &)>;
    using TreeDoneHandler = std::function<void(const QTaskTree &, QtTaskTree::DoneWith)>;

    QAbstractTaskTreeRunner(QObject *parent = nullptr);
    ~QAbstractTaskTreeRunner() override;

    virtual bool isRunning() const = 0;
    virtual void cancel() = 0;
    virtual void reset() = 0;

Q_SIGNALS:
    void aboutToStart(QTaskTree *taskTree);
    void done(QtTaskTree::DoneWith result, QTaskTree *taskTree);

protected:
    QAbstractTaskTreeRunner(QAbstractTaskTreeRunnerPrivate &dd, QObject *parent);

    template <typename Handler>
    static TreeSetupHandler wrapTreeSetupHandler(Handler &&handler) {
        using QtTaskTree::isInvocable;
        if constexpr (std::is_same_v<std::decay_t<Handler>, TreeSetupHandler>) {
            if (!handler)
                return {}; // User passed {} for the setup handler.
        }
        // V, T stands for: [V]oid, [T]askTree
        static constexpr bool isVT = isInvocable<void, Handler, QTaskTree &>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isVT || isV,
            "Tree setup handler needs to take (TaskTree &) or (void) as an argument and has to "
            "return void. The passed handler doesn't fulfill these requirements.");
        return [handler = std::forward<Handler>(handler)](QTaskTree &taskTree) {
            if constexpr (isVT)
                std::invoke(handler, taskTree);
            else if constexpr (isV)
                std::invoke(handler);
        };
    }

    template <typename Handler>
    static TreeDoneHandler wrapTreeDoneHandler(Handler &&handler) {
        using QtTaskTree::isInvocable;
        if constexpr (std::is_same_v<std::decay_t<Handler>, TreeDoneHandler>) {
            if (!handler)
                return {}; // User passed {} for the done handler.
        }
        // V, T, D stands for: [V]oid, [T]askTree, [D]oneWith
        static constexpr bool isVTD = isInvocable<void, Handler, const QTaskTree &, QtTaskTree::DoneWith>();
        static constexpr bool isVT = isInvocable<void, Handler, const QTaskTree &>();
        static constexpr bool isVD = isInvocable<void, Handler, QtTaskTree::DoneWith>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isVTD || isVT || isVD || isV,
            "Task done handler needs to take (const TaskTree &, DoneWith), (const Task &), "
            "(DoneWith) or (void) as arguments and has to return void. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler = std::forward<Handler>(handler)](const QTaskTree &taskTree, QtTaskTree::DoneWith result) {
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

class Q_TASKTREE_EXPORT QSingleTaskTreeRunner : public QAbstractTaskTreeRunner
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSingleTaskTreeRunner)

public:
    QSingleTaskTreeRunner(QObject *parent = nullptr);
    ~QSingleTaskTreeRunner() override;

    bool isRunning() const override;
    void cancel() override;
    void reset() override;

    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void start(const QtTaskTree::Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)
    {
        startImpl(recipe,
                  wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                  wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

private:
    void startImpl(const QtTaskTree::Group &recipe,
                   const TreeSetupHandler &setupHandler,
                   const TreeDoneHandler &doneHandler,
                   QtTaskTree::CallDoneFlags callDone);
};

class Q_TASKTREE_EXPORT QSequentialTaskTreeRunner : public QAbstractTaskTreeRunner
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSequentialTaskTreeRunner)

public:
    QSequentialTaskTreeRunner(QObject *parent = nullptr);
    ~QSequentialTaskTreeRunner();

    bool isRunning() const override;
    void cancel() override;
    void reset() override;

    void cancelCurrent();
    void resetCurrent();

    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void enqueue(const QtTaskTree::Group &recipe,
                 SetupHandler &&setupHandler = {},
                 DoneHandler &&doneHandler = {},
                 QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)
    {
        enqueueImpl(recipe,
                    wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                    wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                    callDone);
    }

private:
    void enqueueImpl(const QtTaskTree::Group &recipe,
                     const TreeSetupHandler &setupHandler,
                     const TreeDoneHandler &doneHandler,
                     QtTaskTree::CallDoneFlags callDone);
};

class Q_TASKTREE_EXPORT QParallelTaskTreeRunner : public QAbstractTaskTreeRunner
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QParallelTaskTreeRunner)

public:
    QParallelTaskTreeRunner(QObject *parent = nullptr);
    ~QParallelTaskTreeRunner();

    bool isRunning() const override;
    void cancel() override;
    void reset() override;

    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void start(const QtTaskTree::Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)
    {
        startImpl(recipe,
                  wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                  wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

private:
    void startImpl(const QtTaskTree::Group &recipe,
                   const TreeSetupHandler &setupHandler,
                   const TreeDoneHandler &doneHandler,
                   QtTaskTree::CallDoneFlags callDone);
};

template <typename Key>
class QMappedTaskTreeRunner : public QAbstractTaskTreeRunner
{
public:
    QMappedTaskTreeRunner(QObject *parent = nullptr)
        : QAbstractTaskTreeRunner(parent)
    {}

    ~QMappedTaskTreeRunner() = default;

    bool isRunning() const override { return !m_taskTrees.empty(); }

    void cancel() override
    {
        while (!m_taskTrees.empty())
            m_taskTrees.begin()->second->cancel();
    }

    void reset() override { m_taskTrees.clear(); }

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
    void start(const Key &key, const QtTaskTree::Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)
    {
        startImpl(key, recipe,
                  wrapTreeSetupHandler(std::forward<SetupHandler>(setupHandler)),
                  wrapTreeDoneHandler(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

private:
    void startImpl(const Key &key, const QtTaskTree::Group &recipe,
                   const TreeSetupHandler &setupHandler,
                   const TreeDoneHandler &doneHandler,
                   QtTaskTree::CallDoneFlags callDone)
    {
        QTaskTree *taskTree = new QTaskTree(recipe);
        connect(taskTree, &QTaskTree::done,
                this, [this, key, doneHandler, callDone](QtTaskTree::DoneWith result) {
            const auto it = m_taskTrees.find(key);
            QTaskTree *runningTaskTree = it->second.release();
            runningTaskTree->deleteLater();
            m_taskTrees.erase(it);
            if (doneHandler && shouldCallDone(callDone, result))
                doneHandler(*runningTaskTree, result);
            Q_EMIT done(result, runningTaskTree);
        });
        m_taskTrees[key].reset(taskTree);
        if (setupHandler)
            setupHandler(*taskTree);
        Q_EMIT aboutToStart(taskTree);
        taskTree->start();
    }
    std::unordered_map<Key, std::unique_ptr<QTaskTree>> m_taskTrees;
};

QT_END_NAMESPACE

#endif // QTASKTREERUNNER_H
