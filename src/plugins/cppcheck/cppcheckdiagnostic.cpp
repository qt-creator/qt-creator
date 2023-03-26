// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckdiagnostic.h"

namespace Cppcheck::Internal {

bool Diagnostic::operator==(const Diagnostic &r) const
{
    return std::tie(severity, message, fileName, lineNumber)
           == std::tie(r.severity, r.message, r.fileName, r.lineNumber);
}

size_t qHash(const Diagnostic &diagnostic)
{
    return qHash(diagnostic.message) ^ qHash(diagnostic.fileName) ^ diagnostic.lineNumber;
}

} // namespace Cppcheck
