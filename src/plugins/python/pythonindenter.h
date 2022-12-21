// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textindenter.h>

namespace Python {

class PythonIndenter : public TextEditor::TextIndenter
{
public:
    explicit PythonIndenter(QTextDocument *doc);
private:
    bool isElectricCharacter(const QChar &ch) const override;
    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;

    bool isElectricLine(const QString &line) const;
    int getIndentDiff(const QString &previousLine,
                      const TextEditor::TabSettings &tabSettings) const;
};

} // namespace Python
