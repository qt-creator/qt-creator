// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Utils { class FilePath; }

namespace Zephyr::Internal {

void setupZephyrRun();

QString sysbuildDomain(const Utils::FilePath &buildDir);

} // namespace Zephyr::Internal
