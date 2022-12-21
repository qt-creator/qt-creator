// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../tools/nimlexer.h"

#include <texteditor/syntaxhighlighter.h>

namespace Nim {

class NimHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    NimHighlighter();

protected:
    void highlightBlock(const QString &text) override;

private:
    TextEditor::TextStyle styleForToken(const NimLexer::Token &token, const QString &tokenValue);
    TextEditor::TextStyle styleForIdentifier(const NimLexer::Token &token, const QString &tokenValue);

    int highlightLine(const QString &text, int initialState);
};

}
