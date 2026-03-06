// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggeritem.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>

#include <utils/filepath.h>

#include <QtTaskTree/QTaskTree>

namespace Debugger {

namespace DebuggerItemManager {

DEBUGGER_EXPORT void restoreDebuggers();

DEBUGGER_EXPORT const QList<DebuggerItem> debuggers();

DEBUGGER_EXPORT QVariant registerDebugger(const DebuggerItem &item);
DEBUGGER_EXPORT void deregisterDebugger(const QVariant &id);

DEBUGGER_EXPORT DebuggerItem findByCommand(const Utils::FilePath &command);
DEBUGGER_EXPORT DebuggerItem findById(const QVariant &id);
DEBUGGER_EXPORT DebuggerItem findByEngineType(DebuggerEngineType engineType);

} // DebuggerItemManager

namespace Internal {

QtTaskTree::ExecutableItem autoDetectDebuggerRecipe(
    ProjectExplorer::Kit *kit,
    const Utils::FilePaths &searchPaths,
    const ProjectExplorer::DetectionSource &detectionSource,
    const ProjectExplorer::LogCallback &logCallback);

QtTaskTree::ExecutableItem removeAutoDetected(
    const QString &detectionSource, const ProjectExplorer::LogCallback &logCallback);

Utils::Result<QtTaskTree::ExecutableItem> createAspectFromJson(
    const ProjectExplorer::DetectionSource &detectionSource,
    const Utils::FilePath &rootPath,
    ProjectExplorer::Kit *kit,
    const QJsonValue &json,
    const ProjectExplorer::LogCallback &logCallback);

} // Internal
} // Debugger
