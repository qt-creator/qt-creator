// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/syntaxhighlighter.h>

#include <QHash>
#include <QTextFormat>

namespace Haskell {
namespace Internal {

class Token;

class HaskellHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    HaskellHighlighter();

protected:
    void highlightBlock(const QString &text) override;

private:
    void setFontSettings(const TextEditor::FontSettings &fontSettings) override;
    void updateFormats(const TextEditor::FontSettings &fontSettings);
    void setTokenFormat(const Token &token, TextEditor::TextStyle style);
    void setTokenFormatWithSpaces(const QString &text, const Token &token,
                                  TextEditor::TextStyle style);
    QTextCharFormat m_toplevelDeclFormat;
};

} // Internal
} // Haskell
