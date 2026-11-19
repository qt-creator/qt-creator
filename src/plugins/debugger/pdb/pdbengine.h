// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

namespace Debugger::Internal {

DebuggerEngine *createPdbEngine(const DebuggerRunParameters &rp);

} // Debugger::Internal
