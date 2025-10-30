// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globaltasktree.h"

#include "qtcassert.h"
#include "shutdownguard.h"
#include "threadutils.h"

namespace Utils {

static QParallelTaskTreeRunner *getRunner()
{
    static GuardedObject<QParallelTaskTreeRunner> theRunner;
    return theRunner.get();
}

QParallelTaskTreeRunner *GlobalTaskTree::taskTreeRunner()
{
    QTC_ASSERT(Utils::isMainThread(), return nullptr);
    return getRunner();
}

} // namespace Utils
