// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggeritem.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>

#include <tasking/tasktree.h>

#include <utils/filepath.h>
#include <utils/treemodel.h>

#include <QList>

namespace Debugger {

namespace DebuggerItemManager {

DEBUGGER_EXPORT void restoreDebuggers();

DEBUGGER_EXPORT const QList<DebuggerItem> debuggers();

DEBUGGER_EXPORT QVariant registerDebugger(const DebuggerItem &item);
DEBUGGER_EXPORT void deregisterDebugger(const QVariant &id);

DEBUGGER_EXPORT void autoDetectDebuggersForDevice(const Utils::FilePaths &searchPaths,
                                         const QString &detectionSource,
                                         QString *logMessage);
DEBUGGER_EXPORT void removeDetectedDebuggers(const QString &detectionSource, QString *logMessage);
DEBUGGER_EXPORT void listDetectedDebuggers(const QString &detectionSource, QString *logMessage);

DEBUGGER_EXPORT const DebuggerItem *findByCommand(const Utils::FilePath &command);
DEBUGGER_EXPORT const DebuggerItem *findById(const QVariant &id);
DEBUGGER_EXPORT const DebuggerItem *findByEngineType(DebuggerEngineType engineType);

} // DebuggerItemManager

namespace Internal {
class DebuggerTreeItem : public Utils::TreeItem
{
public:
    DebuggerTreeItem(const DebuggerItem &item, bool changed)
        : m_item(item), m_orig(item), m_added(changed), m_changed(changed)
    {}

    QVariant data(int column, int role) const override;

    DebuggerItem m_item; // Displayed, possibly unapplied data.
    DebuggerItem m_orig; // Stored original data.
    bool m_added;
    bool m_changed;
    bool m_removed = false;
};

Tasking::ExecutableItem autoDetectDebuggerRecipe(
    ProjectExplorer::Kit *kit,
    const Utils::FilePaths &searchPaths,
    const ProjectExplorer::DetectionSource &detectionSource,
    const ProjectExplorer::LogCallback &logCallback);

Tasking::ExecutableItem removeAutoDetected(
    const QString &detectionSource, const ProjectExplorer::LogCallback &logCallback);

Utils::Result<Tasking::ExecutableItem> createAspectFromJson(
    const QString &detectionSource,
    const Utils::FilePath &rootPath,
    ProjectExplorer::Kit *kit,
    const QJsonValue &json,
    const ProjectExplorer::LogCallback &logCallback);

} // Internal
} // Debugger
