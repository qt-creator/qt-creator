// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <texteditor/syntaxhighlighter.h>

QT_BEGIN_NAMESPACE
class QRegularExpression;
QT_END_NAMESPACE

namespace VcsBase {

class DiffAndLogHighlighterPrivate;

class VCSBASE_EXPORT DiffAndLogHighlighter : public TextEditor::SyntaxHighlighter
{
public:
    explicit DiffAndLogHighlighter(const QRegularExpression &filePattern,
                                   const QRegularExpression &changePattern);
    ~DiffAndLogHighlighter() override;

    void highlightBlock(const QString &text) override;

    void setFontSettings(const TextEditor::FontSettings &fontSettings) override;

    void setEnabled(bool e);

private:
    friend class DiffAndLogHighlighterPrivate;
    DiffAndLogHighlighterPrivate *const d;
};

} // namespace VcsBase
