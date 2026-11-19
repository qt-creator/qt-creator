// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

namespace Debugger::Internal {

DebuggerEngine *createUvscEngine(const DebuggerRunParameters &rp);

} // Debugger::Internal
