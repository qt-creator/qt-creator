// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QFuture>

#include <optional>

namespace Android::Internal {

class AndroidAvdManager
{
public:
    QString startAvd(const QString &name) const;
    bool startAvdAsync(const QString &avdName) const;
    QString findAvd(const QString &avdName) const;
    QString waitForAvd(const QString &avdName, const std::optional<QFuture<void>> &future = {}) const;
    bool isAvdBooted(const QString &device) const;
    static bool avdManagerCommand(const QStringList &args, QString *output);

private:
    bool waitForBooted(const QString &serialNumber, const std::optional<QFuture<void>> &future = {}) const;
};

} // Android::Internal
