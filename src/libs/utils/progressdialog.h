// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QProgressDialog>

namespace Utils {

QTCREATOR_UTILS_EXPORT QProgressDialog *createProgressDialog(int maxValue, const QString &windowTitle,
                                                             const QString &labelText);

} // namespace Utils
