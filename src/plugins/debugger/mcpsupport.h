// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/result.h>

#include <QJsonObject>

#include <functional>

namespace Debugger::Internal {

void registerMcpTools();

void callMcpToolForTests(const QString &name,
                         const QJsonObject &args,
                         const std::function<void(Utils::Result<QJsonObject>)> &done);

} // namespace Debugger::Internal
