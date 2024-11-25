// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <solutions/tasking/tasktree.h>

namespace Android::Internal::AndroidAvdManager {

Tasking::ExecutableItem startAvdRecipe(
    const QString &avdName, const Tasking::Storage<QString> &serialNumberStorage);

} // namespace Android::Internal::AndroidAvdManager
