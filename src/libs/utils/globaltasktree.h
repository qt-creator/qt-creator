// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <solutions/tasking/tasktreerunner.h>

namespace Utils {

class QTCREATOR_UTILS_EXPORT GlobalTaskTree final
{
public:
    template <typename SetupHandler = Tasking::AbstractTaskTreeRunner::TreeSetupHandler,
              typename DoneHandler = Tasking::AbstractTaskTreeRunner::TreeDoneHandler>
    static void start(const Tasking::Group &recipe,
                      SetupHandler &&setupHandler = {},
                      DoneHandler &&doneHandler = {},
                      Tasking::CallDoneFlags callDone = Tasking::CallDone::Always)
    {
        if (auto runner = taskTreeRunner())
            runner->start(recipe, setupHandler, doneHandler, callDone);
    }

private:
    GlobalTaskTree() = delete;
    static Tasking::ParallelTaskTreeRunner *taskTreeRunner();
};

} // namespace Utils
