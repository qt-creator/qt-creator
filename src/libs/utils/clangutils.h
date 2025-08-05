// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <utils/result.h>

QT_BEGIN_NAMESPACE
class QVersionNumber;
QT_END_NAMESPACE

namespace Utils {

class FilePath;

QTCREATOR_UTILS_EXPORT QVersionNumber clangdVersion(const FilePath &clangd);
QTCREATOR_UTILS_EXPORT Utils::Result<> checkClangdVersion(const FilePath &clangd);
QTCREATOR_UTILS_EXPORT QVersionNumber minimumClangdVersion();

} // namespace Utils
