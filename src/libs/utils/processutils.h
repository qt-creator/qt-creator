// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace Utils {

class Process;

QTCREATOR_UTILS_EXPORT bool readDataFromProcess(Process &process, QByteArray *stdOut,
                                                QByteArray *stdErr, int timeoutS = 30);

} // Utils
