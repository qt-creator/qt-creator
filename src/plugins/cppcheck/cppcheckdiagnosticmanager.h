// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Cppcheck::Internal {

class Diagnostic;

class CppcheckDiagnosticManager
{
public:
    virtual ~CppcheckDiagnosticManager() = default;
    virtual void add(const Diagnostic &diagnostic) = 0;
};

} // Cppcheck::Internal
