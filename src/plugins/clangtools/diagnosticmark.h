// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangtoolsdiagnostic.h"

#include <cppeditor/clangdiagnosticconfig.h>
#include <texteditor/textmark.h>

namespace ClangTools {
namespace Internal {

class DiagnosticMark : public TextEditor::TextMark
{
public:
    DiagnosticMark(const Diagnostic &diagnostic, TextEditor::TextDocument *document);
    explicit DiagnosticMark(const Diagnostic &diagnostic);

    void disable();
    bool enabled() const;

    Diagnostic diagnostic() const;

    std::optional<CppEditor::ClangToolType> toolType;

private:
    const Diagnostic m_diagnostic;
    bool m_enabled = true;
};

} // namespace Internal
} // namespace ClangTools
