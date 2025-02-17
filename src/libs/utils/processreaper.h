// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <chrono>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Utils::ProcessReaper {
QTCREATOR_UTILS_EXPORT void deleteAll();
QTCREATOR_UTILS_EXPORT void reap(QProcess *process,
                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
} // namespace Utils::ProcessReaper
