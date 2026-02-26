// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <memory>

namespace ProjectExplorer {

class Kit;
class KitAspect;

} // namespace ProjectExplorer

namespace GNProjectManager::Internal {

class GNTool;

class GNKitAspect
{
public:
    static void setGNTool(ProjectExplorer::Kit *kit, Utils::Id id);
    static Utils::Id gnToolId(const ProjectExplorer::Kit *kit);
    static std::shared_ptr<GNTool> gnTool(const ProjectExplorer::Kit *kit);
    static bool isValid(const ProjectExplorer::Kit *kit);
};

void setupGNKitAspect();

} // namespace GNProjectManager::Internal
