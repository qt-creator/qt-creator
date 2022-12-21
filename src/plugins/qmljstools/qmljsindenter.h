// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <texteditor/textindenter.h>

namespace QmlJSEditor {
namespace Internal {

class QMLJSTOOLS_EXPORT Indenter : public TextEditor::TextIndenter
{
public:
    explicit Indenter(QTextDocument *doc);
    ~Indenter() override;

    bool isElectricCharacter(const QChar &ch) const override;
    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) override;
    void invalidateCache() override;

    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;
    int visualIndentFor(const QTextBlock &block,
                        const TextEditor::TabSettings &tabSettings) override;
    TextEditor::IndentationForBlock indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                         const TextEditor::TabSettings &tabSettings,
                                                         int cursorPositionInEditor = -1) override;
};

} // Internal
} // QmlJSEditor
