// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Utils { class Environment; }

namespace Core {

enum class HandleIncludeGuards { No, Yes };

struct CORE_EXPORT FileUtils
{
    // Helpers for common directory browser options.
    static void showInGraphicalShell(QWidget *parent, const Utils::FilePath &path);
    static void showInFileSystemView(const Utils::FilePath &path);
    static void openTerminal(const Utils::FilePath &path, const Utils::Environment &env);
    static QString msgFindInDirectory();
    static QString msgFileSystemAction();
    // Platform-dependent action descriptions
    static QString msgGraphicalShellAction();
    static QString msgTerminalHereAction();
    static QString msgTerminalWithAction();
    // File operations aware of version control and file system case-insensitiveness
    static void removeFiles(const Utils::FilePaths &filePaths, bool deleteFromFS);
    static bool renameFile(const Utils::FilePath &from, const Utils::FilePath &to,
                           HandleIncludeGuards handleGuards = HandleIncludeGuards::No);

    static void updateHeaderFileGuardIfApplicable(const Utils::FilePath &oldFilePath,
                                                  const Utils::FilePath &newFilePath,
                                                  HandleIncludeGuards handleGuards);

private:
    // This method is used to refactor the include guards in the renamed headers
    static bool updateHeaderFileGuardAfterRename(const QString &headerPath,
                                                 const QString &oldHeaderBaseName);
};

} // namespace Core
