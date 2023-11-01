// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <utils/filepath.h>

#include <QList>

namespace Debugger {

class DebuggerItem;

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
} // Debugger
