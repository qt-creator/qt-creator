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
    using TreeSetupHandler = std::function<void(TaskTree &)>;
    using TreeDoneHandler = std::function<void(const TaskTree &, DoneWith)>;

    ~TaskTreeRunner();

    bool isRunning() const { return bool(m_taskTree); }

    // When task tree is running it resets the old task tree.
    template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler>
    void start(const Group &recipe,
               SetupHandler &&setupHandler = {},
               DoneHandler &&doneHandler = {},
               CallDoneFlags callDone = CallDone::Always)
    {
        startImpl(recipe,
                  wrapSetup(std::forward<SetupHandler>(setupHandler)),
                  wrapDone(std::forward<DoneHandler>(doneHandler)),
                  callDone);
    }

    // When task tree is running it emits done(DoneWith::Cancel) synchronously.
    void cancel();

    // No done() signal is emitted.
    void reset();

Q_SIGNALS:
    void aboutToStart(TaskTree *taskTree);
    void done(DoneWith result);

private:
    template <typename Handler>
    static TreeSetupHandler wrapSetup(Handler &&handler) {
        if constexpr (std::is_same_v<Handler, TreeSetupHandler>)
            return {}; // When user passed {} for the setup handler.
        // V, T stands for: [V]oid, [T]askTree
        static constexpr bool isVT = isInvocable<void, Handler, TaskTree &>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isVT || isV,
            "Tree setup handler needs to take (TaskTree &) or (void) as an argument and has to "
            "return void. The passed handler doesn't fulfill these requirements.");
        return [handler = std::move(handler)](TaskTree &taskTree) {
            if constexpr (isVT)
                std::invoke(handler, taskTree);
            else if constexpr (isV)
                std::invoke(handler);
        };
    }

    template <typename Handler>
    static TreeDoneHandler wrapDone(Handler &&handler) {
        if constexpr (std::is_same_v<Handler, TreeDoneHandler>)
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
        return [handler = std::move(handler)](const TaskTree &taskTree, DoneWith result) {
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

    void startImpl(const Group &recipe,
                   const TreeSetupHandler &setupHandler = {},
                   const TreeDoneHandler &doneHandler = {},
                   CallDoneFlags callDone = CallDone::Always);

    std::unique_ptr<TaskTree> m_taskTree;
};

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_TASKTREERUNNER_H
