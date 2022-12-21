// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangtoolsdiagnostic.h"

#include <texteditor/textmark.h>

namespace ClangTools {
namespace Internal {


class DiagnosticMark : public TextEditor::TextMark
{
    Q_DECLARE_TR_FUNCTIONS(ClangTools::Internal::DiagnosticMark)
public:
    explicit DiagnosticMark(const Diagnostic &diagnostic);

    void disable();
    bool enabled() const;

    Diagnostic diagnostic() const;

    QString source;

private:
    const Diagnostic m_diagnostic;
    bool m_enabled = true;
};

} // namespace Internal
} // namespace ClangTools
