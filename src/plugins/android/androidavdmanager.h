// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QFuture>

#include <optional>

#include <solutions/tasking/tasktree.h>

namespace Android::Internal::AndroidAvdManager {

QString startAvd(const QString &name, const std::optional<QFuture<void>> &future = {});
bool startAvdAsync(const QString &avdName);
QString findAvd(const QString &avdName);
QString waitForAvd(const QString &avdName, const std::optional<QFuture<void>> &future = {});
bool isAvdBooted(const QString &device);

Tasking::ExecutableItem serialNumberRecipe(
    const QString &avdName, const Tasking::Storage<QString> &serialNumberStorage);
Tasking::ExecutableItem startAvdAsyncRecipe(const QString &avdName);
Tasking::ExecutableItem waitForAvdRecipe(
    const QString &avdName, const Tasking::Storage<QString> &serialNumberStorage);
Tasking::ExecutableItem startAvdRecipe(
    const QString &avdName, const Tasking::Storage<QString> &serialNumberStorage);

} // namespace Android::Internal::AndroidAvdManager
