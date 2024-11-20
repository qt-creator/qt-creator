// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Utils { class Environment; }

namespace Core {

enum class HandleIncludeGuards { No, Yes };

namespace FileUtils {

// Helpers for common directory browser options.
CORE_EXPORT void showInGraphicalShell(QWidget *parent, const Utils::FilePath &path);
CORE_EXPORT void showInFileSystemView(const Utils::FilePath &path);
CORE_EXPORT void openTerminal(const Utils::FilePath &path, const Utils::Environment &env);
CORE_EXPORT QString msgFindInDirectory();
CORE_EXPORT QString msgFileSystemAction();
// Platform-dependent action descriptions
CORE_EXPORT QString msgGraphicalShellAction();
CORE_EXPORT QString msgTerminalHereAction();
CORE_EXPORT QString msgTerminalWithAction();
// File operations aware of version control and file system case-insensitiveness
CORE_EXPORT void removeFiles(const Utils::FilePaths &filePaths, bool deleteFromFS);
CORE_EXPORT bool renameFile(const Utils::FilePath &from, const Utils::FilePath &to,
                            HandleIncludeGuards handleGuards = HandleIncludeGuards::No);

CORE_EXPORT void updateHeaderFileGuardIfApplicable(const Utils::FilePath &oldFilePath,
                                                   const Utils::FilePath &newFilePath,
                                                   HandleIncludeGuards handleGuards);
} // namespace FileUtils
} // namespace Core
