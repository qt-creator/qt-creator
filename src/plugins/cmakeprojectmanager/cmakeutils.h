// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cmakelang/cmakedocument.h>

#include <QStringList>

namespace Utils { class FilePath; }

namespace CMakeProjectManager::Internal {

QString addCMakePrefix(const QString &str);
QStringList addCMakePrefix(const QStringList &list);

// Reads and parses a CMake file. The document is invalid when the contents do
// not parse; a file that cannot be read yields an empty document.
CMakeLang::DocumentPtr parseCMakeFile(const Utils::FilePath &filePath);

} // CMakeProjectManager::Internal
