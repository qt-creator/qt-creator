// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>

namespace Utils {
class FilePath;

enum class IterationPolicy { Stop, Continue };

using FilePathPredicate = std::function<bool(const FilePath &)>;
} // namespace Utils
