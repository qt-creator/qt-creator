// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "qtsupport_global.h"

namespace Utils {
class Environment;
class FilePath;
class FilePaths;
}

namespace QtSupport {
namespace Internal {
Utils::FilePaths findQtsInEnvironment(
    const Utils::Environment &env, const Utils::FilePath &deviceRoot);
Utils::FilePaths findQtsInPaths(const Utils::FilePaths &paths);
bool isQtChooser(const Utils::FilePath &filePath);
Utils::FilePath qtChooserToQmakePath(const Utils::FilePath &qtChooser);
} // namespace Internal

QTSUPPORT_EXPORT QString filterForQmakeFileDialog();

} // namespace QtSupport
