// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/syntaxhighlighter.h>

namespace Python {
namespace Internal {

class Scanner;

class PythonHighlighter : public TextEditor::SyntaxHighlighter
{
public:
    PythonHighlighter();

private:
    void highlightBlock(const QString &text) override;
    int highlightLine(const QString &text, int initialState);
    void highlightImport(Internal::Scanner &scanner);

    int m_lastIndent = 0;
    bool withinLicenseHeader = false;
};

} // namespace Internal
} // namespace Python
