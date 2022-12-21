// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textindenter.h>

namespace Android {
namespace Internal {
class JavaIndenter : public TextEditor::TextIndenter
{
public:
    explicit JavaIndenter(QTextDocument *doc);
    ~JavaIndenter() override;

    bool isElectricCharacter(const QChar &ch) const override;

    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) override;

    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;
};
} // namespace Internal
} // namespace Android
