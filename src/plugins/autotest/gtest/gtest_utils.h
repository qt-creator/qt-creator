// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Autotest {
namespace Internal {
namespace GTestUtils {

bool isGTestMacro(const QString &macro);
bool isGTestParameterized(const QString &macro);
bool isGTestTyped(const QString &macro);
bool isValidGTestFilter(const QString &filterExpression);

} // namespace GTestUtils
} // namespace Internal
} // namespace Autotest
