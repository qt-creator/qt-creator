// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/syntaxhighlighter.h>
#include <texteditor/codeassist/keywordscompletionassist.h>

namespace QmakeProjectManager {
namespace Internal {

class ProFileHighlighter : public TextEditor::SyntaxHighlighter
{
public:
    enum ProfileFormats {
        ProfileVariableFormat,
        ProfileFunctionFormat,
        ProfileCommentFormat,
        ProfileVisualWhitespaceFormat,
        NumProfileFormats
    };

    ProFileHighlighter();
    void highlightBlock(const QString &text) override;

private:
    const TextEditor::Keywords m_keywords;
};

} // namespace Internal
} // namespace QmakeProjectManager
