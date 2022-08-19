// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textindenter.h>

namespace TextEditor { class ICodeStylePreferences; }

namespace CppEditor {
class CppCodeStyleSettings;
class CppCodeStylePreferences;

namespace Internal {
class CppQtStyleIndenter : public TextEditor::TextIndenter
{
public:
    explicit CppQtStyleIndenter(QTextDocument *doc);
    ~CppQtStyleIndenter() override;

    bool isElectricCharacter(const QChar &ch) const override;
    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) override;

    void indent(const QTextCursor &cursor,
                const QChar &typedChar,
                const TextEditor::TabSettings &tabSettings,
                int cursorPositionInEditor = -1) override;

    void setCodeStylePreferences(TextEditor::ICodeStylePreferences *preferences) override;
    void invalidateCache() override;
    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;
    TextEditor::IndentationForBlock indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                         const TextEditor::TabSettings &tabSettings,
                                                         int cursorPositionInEditor = -1) override;

private:
    CppCodeStyleSettings codeStyleSettings() const;
    CppCodeStylePreferences *m_cppCodeStylePreferences = nullptr;
};

} // namespace Internal
} // namespace CppEditor
