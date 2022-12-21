// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <utils/filepath.h>

#include <QList>

namespace Debugger {

class DebuggerItem;

class DEBUGGER_EXPORT DebuggerItemManager
{
    Q_DISABLE_COPY_MOVE(DebuggerItemManager)

public:
    DebuggerItemManager();
    ~DebuggerItemManager();

    void extensionsInitialized();

    static const QList<DebuggerItem> debuggers();

    static QVariant registerDebugger(const DebuggerItem &item);
    static void deregisterDebugger(const QVariant &id);

    static void autoDetectDebuggersForDevice(const Utils::FilePaths &searchPaths,
                                             const QString &detectionSource,
                                             QString *logMessage);
    static void removeDetectedDebuggers(const QString &detectionSource, QString *logMessage);
    static void listDetectedDebuggers(const QString &detectionSource, QString *logMessage);

    static const DebuggerItem *findByCommand(const Utils::FilePath &command);
    static const DebuggerItem *findById(const QVariant &id);
    static const DebuggerItem *findByEngineType(DebuggerEngineType engineType);
};

} // namespace Debugger
