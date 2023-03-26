// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtest-llvm-printing.h"
#include "gtest-std-printing.h"

#include <gtest/gtest-printers.h>

#include <utils/smallstringio.h>

#include <clang/Tooling/CompilationDatabase.h>

namespace clang {
namespace tooling {
struct CompileCommand;

std::ostream &operator<<(std::ostream &out, const CompileCommand &command)
{
    return out << "(" << command.Directory << ", " << command.Filename << ", "
               << command.CommandLine << ", " << command.Output << ")";
}
} // namespace tooling
} // namespace clang
