// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/syntaxhighlighter.h>

namespace GlslEditor {
namespace Internal {

class GlslHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    GlslHighlighter();

protected:
    void highlightBlock(const QString &text) override;
    void highlightLine(const QString &text, int position, int length, const QTextCharFormat &format);
    bool isPPKeyword(QStringView text) const;
};

} // namespace Internal
} // namespace GlslEditor
