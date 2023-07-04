// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangformatbaseindenter.h"

#include <texteditor/tabsettings.h>

namespace ClangFormat {

class ClangFormatIndenter final : public ClangFormatBaseIndenter
{
public:
    ClangFormatIndenter(QTextDocument *doc);
    std::optional<TextEditor::TabSettings> tabSettings() const override;
    bool formatOnSave() const override;

private:
    bool formatCodeInsteadOfIndent() const override;
    bool formatWhileTyping() const override;
    int lastSaveRevision() const override;
};

class ClangFormatForwardingIndenter : public TextEditor::Indenter
{
public:
    explicit ClangFormatForwardingIndenter(QTextDocument *doc);
    ~ClangFormatForwardingIndenter() override;

    void setFileName(const Utils::FilePath &fileName) override;
    bool isElectricCharacter(const QChar &ch) const override;
    void setCodeStylePreferences(TextEditor::ICodeStylePreferences *preferences) override;
    void invalidateCache() override;
    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;
    int visualIndentFor(const QTextBlock &block,
                        const TextEditor::TabSettings &tabSettings) override;
    void autoIndent(const QTextCursor &cursor,
                    const TextEditor::TabSettings &tabSettings,
                    int cursorPositionInEditor = -1) override;
    Utils::EditOperations format(const TextEditor::RangesInLines &rangesInLines,
                                 FormattingMode mode) override;
    bool formatOnSave() const override;
    TextEditor::IndentationForBlock indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                         const TextEditor::TabSettings &tabSettings,
                                                         int cursorPositionInEditor = -1) override;
    std::optional<TextEditor::TabSettings> tabSettings() const override;
    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) override;
    void indent(const QTextCursor &cursor,
                const QChar &typedChar,
                const TextEditor::TabSettings &tabSettings,
                int cursorPositionInEditor = -1) override;
    virtual void reindent(const QTextCursor &cursor,
                          const TextEditor::TabSettings &tabSettings,
                          int cursorPositionInEditor = -1) override;
    std::optional<int> margin() const override;

private:
    TextEditor::Indenter *currentIndenter() const;

    std::unique_ptr<TextEditor::Indenter> m_clangFormatIndenter;
    std::unique_ptr<TextEditor::Indenter> m_cppIndenter;
};

} // namespace ClangFormat
