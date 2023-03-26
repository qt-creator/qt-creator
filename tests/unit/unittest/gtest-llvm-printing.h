// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <gtest/gtest-printers.h>

#include <iosfwd>

namespace clang {
namespace tooling {
struct CompileCommand;

std::ostream &operator<<(std::ostream &out, const CompileCommand &command);

} // namespace tooling
} // namespace clang
