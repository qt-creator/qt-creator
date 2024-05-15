// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QFuture>

#include <optional>

namespace Android::Internal::AndroidAvdManager {

QString startAvd(const QString &name);
bool startAvdAsync(const QString &avdName);
QString findAvd(const QString &avdName);
QString waitForAvd(const QString &avdName, const std::optional<QFuture<void>> &future = {});
bool isAvdBooted(const QString &device);

} // namespace Android::Internal::AndroidAvdManager
