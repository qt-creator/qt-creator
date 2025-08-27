// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globaltasktree.h"

#include "qtcassert.h"
#include "shutdownguard.h"
#include "threadutils.h"

using namespace Tasking;

namespace Utils {

static ParallelTaskTreeRunner *getRunner()
{
    static GuardedObject<ParallelTaskTreeRunner> theRunner;
    return theRunner.get();
}

ParallelTaskTreeRunner *GlobalTaskTree::taskTreeRunner()
{
    QTC_ASSERT(Utils::isMainThread(), return nullptr);
    return getRunner();
}

} // namespace Utils
