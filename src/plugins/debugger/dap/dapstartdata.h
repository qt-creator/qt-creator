// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>
#include <utils/processinterface.h>

#include <QString>
#include <QStringList>

namespace Debugger::Internal {

class DEBUGGER_EXPORT BridgeStartData
{
public:
    QStringList startupArguments;
    QString bridgeModule;
    QString serverCall;
};

DEBUGGER_EXPORT BridgeStartData dapHostRecipe(bool loadInitFile);

// What a backend speaking a DAP-shaped protocol is started with.
class DEBUGGER_EXPORT DapStartData
{
public:
    Utils::ProcessRunData debuggerRunData;
    InferiorStartData inferiorStartData;
    Utils::FilePath dumperScriptsDir;
    BridgeStartData bridgeStartData;
    Utils::FilePaths extraDumperFiles;
    QStringList extraDumperCommands;
    Utils::FilePath sysroot;
    QList<QPair<QString, QString>> sourcePathMap;
    Utils::FilePaths sourceDirectories;
    bool nativeMixedDebugging = false;
    // Dumper context the interface's RefreshRequest does not carry.
    int qtVersion = 0;
    QString qtNamespace;
};

} // namespace Debugger::Internal
