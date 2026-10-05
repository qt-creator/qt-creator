// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Debugger {
class DebuggerRunParameters;

namespace Internal {
class DebuggerEngine;

DebuggerEngine *createJdwpEngine(const DebuggerRunParameters &rp);

} // namespace Internal
} // namespace Debugger
