// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QList>

namespace CMakeProjectManager::Internal {

class CMakeJavaTarget
{
public:
    QString name;
    Utils::FilePath jar;
    Utils::FilePath sourceDirectory;
    Utils::FilePaths sources;
    Utils::FilePaths classPath;
};

QList<CMakeJavaTarget> readJavaTargets(const Utils::FilePath &sourceDirectory,
                                       const Utils::FilePath &buildDirectory);

} // namespace CMakeProjectManager::Internal
