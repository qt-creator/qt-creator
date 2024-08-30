// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <utils/filepath.h>

namespace CMakeProjectManager {

CMAKE_EXPORT void buildTarget(const Utils::FilePath &projectPath, const QString &targetName);

} // namespace CMakeProjectManager
