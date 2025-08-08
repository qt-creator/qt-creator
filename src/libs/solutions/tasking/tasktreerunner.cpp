// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tasktreerunner.h"

#include "tasktree.h"

QT_BEGIN_NAMESPACE

namespace Tasking {

SingleTaskTreeRunner::~SingleTaskTreeRunner() = default;

void SingleTaskTreeRunner::startImpl(const Group &recipe,
                               const TreeSetupHandler &setupHandler,
                               const TreeDoneHandler &doneHandler,
                               CallDoneFlags callDone)
{
    m_taskTree.reset(new TaskTree(recipe));
    connect(m_taskTree.get(), &TaskTree::done, this, [this, doneHandler, callDone](DoneWith result) {
        TaskTree *taskTree = m_taskTree.release();
        taskTree->deleteLater();
        if (doneHandler && shouldCallDone(callDone, result))
            doneHandler(*taskTree, result);
        emit done(result, taskTree);
    });
    if (setupHandler)
        setupHandler(*m_taskTree);
    emit aboutToStart(m_taskTree.get());
    m_taskTree->start();
}

void SingleTaskTreeRunner::cancel()
{
    if (m_taskTree)
        m_taskTree->cancel();
}

void SingleTaskTreeRunner::reset()
{
    m_taskTree.reset();
}

SequentialTaskTreeRunner::SequentialTaskTreeRunner()
{
    connect(&m_taskTreeRunner, &SingleTaskTreeRunner::aboutToStart,
            this, &SequentialTaskTreeRunner::aboutToStart);
    connect(&m_taskTreeRunner, &SingleTaskTreeRunner::done,
            this, [this](DoneWith result, TaskTree *taskTree) {
        emit done(result, taskTree);
        startNext();
    });
}

SequentialTaskTreeRunner::~SequentialTaskTreeRunner() = default;

bool SequentialTaskTreeRunner::isRunning() const
{
    return !m_treeDataQueue.isEmpty() || m_taskTreeRunner.isRunning();
}

void SequentialTaskTreeRunner::cancel()
{
    m_treeDataQueue.empty();
    m_taskTreeRunner.cancel();
}

void SequentialTaskTreeRunner::cancelCurrent()
{
    m_taskTreeRunner.cancel();
}

void SequentialTaskTreeRunner::reset()
{
    m_treeDataQueue.empty();
    m_taskTreeRunner.reset();
}

void SequentialTaskTreeRunner::resetCurrent()
{
    m_taskTreeRunner.reset();
    startNext();
}

void SequentialTaskTreeRunner::enqueueImpl(const TreeData &data)
{
    m_treeDataQueue.append(data);
    if (!m_taskTreeRunner.isRunning())
        startNext();
}

void SequentialTaskTreeRunner::startNext()
{
    if (m_treeDataQueue.isEmpty())
        return;

    const TreeData data = m_treeDataQueue.takeFirst();
    m_taskTreeRunner.start(data.recipe, data.setupHandler, data.doneHandler, data.callDone);
}

ParallelTaskTreeRunner::~ParallelTaskTreeRunner() = default;

void ParallelTaskTreeRunner::cancel()
{
    while (!m_taskTrees.empty())
        m_taskTrees.begin()->second->cancel();
}

void ParallelTaskTreeRunner::reset()
{
    m_taskTrees.clear();
}

void ParallelTaskTreeRunner::startImpl(const Group &recipe,
                                       const TreeSetupHandler &setupHandler,
                                       const TreeDoneHandler &doneHandler,
                                       CallDoneFlags callDone)
{
    TaskTree *taskTree = new TaskTree(recipe);
    connect(taskTree, &TaskTree::done, this, [this, taskTree, doneHandler, callDone](DoneWith result) {
        const auto it = m_taskTrees.find(taskTree);
        it->second.release()->deleteLater();
        m_taskTrees.erase(it);
        if (doneHandler && shouldCallDone(callDone, result))
            doneHandler(*taskTree, result);
        emit done(result, taskTree);
    });
    m_taskTrees.emplace(taskTree, taskTree);
    if (setupHandler)
        setupHandler(*taskTree);
    emit aboutToStart(taskTree);
    taskTree->start();
}

} // namespace Tasking

QT_END_NAMESPACE
