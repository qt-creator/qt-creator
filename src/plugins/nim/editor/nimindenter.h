// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textindenter.h>

namespace Nim {

class NimIndenter : public TextEditor::TextIndenter
{
public:
    explicit NimIndenter(QTextDocument *doc);

    bool isElectricCharacter(const QChar &ch) const override;

    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &settings,
                     int cursorPositionInEditor = -1) override;

private:
    static const QSet<QChar> &electricCharacters();

    bool startsBlock(const QString &line, int state) const;
    bool endsBlock(const QString &line, int state) const;

    int calculateIndentationDiff(const QString &previousLine,
                                 int previousState,
                                 int indentSize) const;

    static QString rightTrimmed(const QString &other);
};

} // namespace Nim
