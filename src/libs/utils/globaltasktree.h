// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QtTaskTree/QParallelTaskTreeRunner>

namespace Utils {

class QTCREATOR_UTILS_EXPORT GlobalTaskTree final
{
public:
    template <typename SetupHandler = QtTaskTree::TreeSetupHandler,
              typename DoneHandler = QtTaskTree::TreeDoneHandler>
    static void start(const QtTaskTree::Group &recipe,
                      SetupHandler &&setupHandler = {},
                      DoneHandler &&doneHandler = {},
                      QtTaskTree::CallDone callDone = QtTaskTree::CallDoneFlag::Always)
    {
        if (auto runner = taskTreeRunner())
            runner->start(recipe, std::forward<SetupHandler>(setupHandler),
                          std::forward<DoneHandler>(doneHandler), callDone);
    }

private:
    GlobalTaskTree() = delete;
    static QtTaskTree::QParallelTaskTreeRunner *taskTreeRunner();
};

} // namespace Utils
