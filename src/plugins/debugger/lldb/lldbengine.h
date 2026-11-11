// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>
#include <debugger/debuggerengineinterface.h>

namespace Debugger::Internal {

InferiorStartData lldbInferiorStartData(const DebuggerRunParameters &rp);
DebuggerEngine *createLldbEngine(const DebuggerRunParameters &rp);

} // Debugger::Internal
