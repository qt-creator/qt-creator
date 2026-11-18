// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cdbimpl.h"

#include <debugger/debuggerengine.h>

namespace Core { class IOptionsPage; }

namespace Debugger::Internal {

QString cdbExtensionLibraryName(bool is64Bit, bool isArm = false);
CdbImplStartData cdbImplStartData(const DebuggerRunParameters &rp);
DebuggerEngine *createCdbEngine(const DebuggerRunParameters &rp);
void addCdbOptionPages(QList<Core::IOptionsPage*> *opts);

} // Debugger::Internal
