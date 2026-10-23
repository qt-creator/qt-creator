// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Utils { class FilePath; }

namespace Zephyr::Internal {

void setupZephyrRun();

QString sysbuildDomain(const Utils::FilePath &buildDir);
Utils::FilePath appBuildDirectory(const Utils::FilePath &buildDir);
enum class ZephyrTarget : quint8 { Unknown, Emulator, Hardware };
ZephyrTarget zephyrTarget(const Utils::FilePath &appBuildDir);

} // namespace Zephyr::Internal
