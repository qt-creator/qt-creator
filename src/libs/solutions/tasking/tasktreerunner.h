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

class TASKING_EXPORT TaskTreeRunner : public QObject
{
    Q_OBJECT

public:
    using SetupHandler = std::function<void(TaskTree *)>;
    using DoneHandler = std::function<void(DoneWith)>;

    ~TaskTreeRunner();

    bool isRunning() const { return bool(m_taskTree); }

    // When task tree is running it resets the old task tree.
    void start(const Group &recipe,
               const SetupHandler &setupHandler = {},
               const DoneHandler &doneHandler = {});

    // When task tree is running it emits done(DoneWith::Cancel) synchronously.
    void cancel();

    // No done() signal is emitted.
    void reset();

Q_SIGNALS:
    void aboutToStart(TaskTree *taskTree);
    void done(DoneWith result);

private:
    std::unique_ptr<TaskTree> m_taskTree;
};

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_TASKTREERUNNER_H
