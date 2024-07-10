// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tasktreerunner.h"

#include "tasktree.h"

QT_BEGIN_NAMESPACE

namespace Tasking {

TaskTreeRunner::~TaskTreeRunner() = default;

void TaskTreeRunner::start(const Group &recipe,
                           const SetupHandler &setupHandler,
                           const DoneHandler &doneHandler)
{
    m_taskTree.reset(new TaskTree(recipe));
    connect(m_taskTree.get(), &TaskTree::done, this, [this, doneHandler](DoneWith result) {
        m_taskTree.release()->deleteLater();
        if (doneHandler)
            doneHandler(result);
        emit done(result);
    });
    if (setupHandler)
        setupHandler(m_taskTree.get());
    emit aboutToStart(m_taskTree.get());
    m_taskTree->start();
}

void TaskTreeRunner::cancel()
{
    if (m_taskTree)
        m_taskTree->cancel();
}

void TaskTreeRunner::reset()
{
    m_taskTree.reset();
}

} // namespace Tasking

QT_END_NAMESPACE
