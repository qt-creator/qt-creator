// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <texteditor/syntaxhighlighter.h>

namespace GLSL {
class Token;
}

namespace EffectComposer {

class SyntaxHighlighter : public TextEditor::SyntaxHighlighter
{
    using Super = TextEditor::SyntaxHighlighter;

public:
    SyntaxHighlighter();

protected:
    void highlightBlock(const QString &text) override;

private:
    void highlightToken(const GLSL::Token &token);
};

} // namespace EffectComposer
