// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Debugger::Internal {

#ifdef WITH_TESTS
QObject *createDebuggerTest();
#endif

} // Debugger::Internal
