// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uvscengine.h"
#include "uvscimpl.h"

#include <debugger/genericdebuggerengine.h>

namespace Debugger::Internal {

DebuggerEngine *createUvscEngine(const DebuggerRunParameters &rp)
{
    UvscImplStartData startData;
    startData.uVisionExecutable = rp.debugger().command.executable();
    startData.remoteChannel = rp.remoteChannel();
    startData.projectFile = rp.uVisionProjectFilePath();
    startData.optionsFile = rp.uVisionOptionsFilePath();
    startData.simulator = rp.uVisionSimulator();
    startData.inferiorExecutable = rp.inferior().command.executable();
    return new GenericDebuggerEngine("UVSC (UvscImpl)", new UvscImpl(startData));
}

} // Debugger::Internal
