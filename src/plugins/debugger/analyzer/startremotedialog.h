// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debugger_global.h>

#include <utils/processinterface.h>

namespace Debugger {

DEBUGGER_EXPORT std::optional<Utils::ProcessRunData> runStartRemoteDialog();

} // Debugger
