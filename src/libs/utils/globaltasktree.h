// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QtTaskTree/QParallelTaskTreeRunner>

namespace Utils {

class QTCREATOR_UTILS_EXPORT GlobalTaskTree final
{
public:
    template <typename SetupHandler = QAbstractTaskTreeRunner::TreeSetupHandler,
              typename DoneHandler = QAbstractTaskTreeRunner::TreeDoneHandler>
    static void start(const QtTaskTree::Group &recipe,
                      SetupHandler &&setupHandler = {},
                      DoneHandler &&doneHandler = {},
                      QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)
    {
        if (auto runner = taskTreeRunner())
            runner->start(recipe, setupHandler, doneHandler, callDone);
    }

private:
    GlobalTaskTree() = delete;
    static QParallelTaskTreeRunner *taskTreeRunner();
};

} // namespace Utils
