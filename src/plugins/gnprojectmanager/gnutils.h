// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once
#include <utils/filepath.h>


namespace GNProjectManager::Internal {

// Resolve a GN source-root-relative path (starting with "//") to an absolute path.
// For example, "//hello.cc" with rootPath "/home/user/project" becomes "/home/user/project/hello.cc"

QString resolveGNPath(QString gnPath,
                      const Utils::FilePath &rootPath,
                      bool stripTailingSlash = false);

} // namespace GNProjectManager::Internal
