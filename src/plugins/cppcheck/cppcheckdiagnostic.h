// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace Cppcheck::Internal {

class Diagnostic final
{
public:
    enum class Severity {
        Error, Warning, Performance, Portability, Style, Information
    };
    bool isValid() const {
        return !fileName.isEmpty() && lineNumber > 0;
    }
    bool operator==(const Diagnostic& diagnostic) const;

    Severity severity = Severity::Information;
    QString severityText;
    QString checkId;
    QString message;
    Utils::FilePath fileName;
    int lineNumber = 0;
};

size_t qHash(const Diagnostic &diagnostic);

} // Cppcheck::Internal
