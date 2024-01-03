// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/indenter.h>

#include <clang/Format/Format.h>

namespace ClangFormat {

enum class ReplacementsToKeep { OnlyIndent, IndentAndBefore, All };

class ClangFormatBaseIndenter : public TextEditor::Indenter
{
public:
    ClangFormatBaseIndenter(QTextDocument *doc);

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
    void indent(const QTextCursor &cursor, const QChar &typedChar, int cursorPositionInEditor);
    void indentBlocks(const QTextBlock &startBlock,
                      const QTextBlock &endBlock,
                      const QChar &typedChar,
                      int cursorPositionInEditor);
    Utils::ChangeSet indentsFor(QTextBlock startBlock,
                                const QTextBlock &endBlock,
                                const QChar &typedChar,
                                int cursorPositionInEditor,
                                bool trimTrailingWhitespace = true);
    Utils::ChangeSet replacements(QByteArray buffer,
                                  const QTextBlock &startBlock,
                                  const QTextBlock &endBlock,
                                  int cursorPositionInEditor,
                                  ReplacementsToKeep replacementsToKeep,
                                  const QChar &typedChar = QChar::Null,
                                  bool secondTry = false) const;

    struct CachedStyle {
        clang::format::FormatStyle style = clang::format::getNoStyle();
        QDateTime expirationTime;
        void setCache(clang::format::FormatStyle newStyle, std::chrono::milliseconds timeout)
        {
            style = newStyle;
            expirationTime = QDateTime::currentDateTime().addMSecs(timeout.count());
        }
    };

    mutable CachedStyle m_cachedStyle;

    clang::format::FormatStyle overrideStyle(const Utils::FilePath &fileName) const;
    TextEditor::ICodeStylePreferences *m_overriddenPreferences = nullptr;
};

} // namespace ClangFormat
