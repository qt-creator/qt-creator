// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppcheckdiagnostic.h"

#include <texteditor/textmark.h>

namespace Cppcheck::Internal {

class CppcheckTextMark final : public TextEditor::TextMark
{
public:
    explicit CppcheckTextMark(const Diagnostic &diagnostic);

    bool operator==(const Diagnostic &r) const {
        return lineNumber() == r.lineNumber
                && (std::tie(m_severity, m_checkId, m_message) ==
                    std::tie(r.severity, r.checkId, r.message));
    }

private:
    QString toolTipText(const QString &severityText) const;

    Diagnostic::Severity m_severity = Diagnostic::Severity::Information;
    QString m_checkId;
    QString m_message;
};

} // Cppcheck::Internal
