// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/indenter.h>

namespace clang::format { struct FormatStyle; }

namespace ClangFormat {

class ClangFormatBaseIndenter : public TextEditor::Indenter
{
public:
    ClangFormatBaseIndenter(QTextDocument *doc);
    ~ClangFormatBaseIndenter();

    TextEditor::IndentationForBlock indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                         const TextEditor::TabSettings &tabSettings,
                                                         int cursorPositionInEditor = -1) override;
    void indent(const QTextCursor &cursor,
                const QChar &typedChar,
                const TextEditor::TabSettings &tabSettings,
                int cursorPositionInEditor = -1) override;

    void reindent(const QTextCursor &cursor,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;

    void autoIndent(const QTextCursor &cursor,
                    const TextEditor::TabSettings &tabSettings,
                    int cursorPositionInEditor = -1) override;
    Utils::EditOperations format(const TextEditor::RangesInLines &rangesInLines,
                                 FormattingMode mode = FormattingMode::Forced) override;

    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) override;

    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;

    bool isElectricCharacter(const QChar &ch) const override;

    std::optional<int> margin() const override;

    const clang::format::FormatStyle &styleForFile() const;

    void setOverriddenPreferences(TextEditor::ICodeStylePreferences *preferences);

protected:
    virtual bool formatCodeInsteadOfIndent() const { return false; }
    virtual bool formatWhileTyping() const { return false; }
    virtual int lastSaveRevision() const { return 0; }

private:
    friend class ClangFormatBaseIndenterPrivate;
    class ClangFormatBaseIndenterPrivate *d = nullptr;
};

} // namespace ClangFormat
