// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <texteditor/syntaxhighlighter.h>

namespace VcsBase {
class BaseAnnotationHighlighterPrivate;

class VCSBASE_EXPORT BaseAnnotationHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    typedef  QSet<QString> ChangeNumbers;

    explicit BaseAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                       QTextDocument *document = nullptr);
    ~BaseAnnotationHighlighter() override;

    void setChangeNumbers(const ChangeNumbers &changeNumbers);

    void highlightBlock(const QString &text) override;

    void setFontSettings(const TextEditor::FontSettings &fontSettings) override;

private:
    // Implement this to return the change number of a line
    virtual QString changeNumber(const QString &block) const = 0;

    BaseAnnotationHighlighterPrivate *const d;
    friend class BaseAnnotationHighlighterPrivate;
};

} // namespace VcsBase
