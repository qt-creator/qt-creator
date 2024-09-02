// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace ProjectExplorer { class BuildSystem; }

namespace CMakeProjectManager::Internal {

void buildTarget(const ProjectExplorer::BuildSystem *buildSystem, const QString &targetName);

} // namespace CMakeProjectManager::Internal
