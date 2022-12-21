// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Debugger::Internal {

bool interruptProcess(qint64 pID, int engineType, QString *errorMessage,
                      const bool engineExecutableIs64Bit = false);

} // Debugger::Internal
