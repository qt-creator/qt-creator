/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    void formatOrIndent(const QTextCursor &cursor,
                        const TextEditor::TabSettings &tabSettings,
                        int cursorPositionInEditor = -1) override;
    TextEditor::Replacements format(
        const TextEditor::RangesInLines &rangesInLines = TextEditor::RangesInLines()) override;

    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) override;

    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;

    bool isElectricCharacter(const QChar &ch) const override;

protected:
    virtual clang::format::FormatStyle styleForFile() const;
    virtual bool formatCodeInsteadOfIndent() const { return false; }
    virtual bool formatWhileTyping() const { return false; }
    virtual int lastSaveRevision() const { return 0; }

private:
    void indent(const QTextCursor &cursor, const QChar &typedChar, int cursorPositionInEditor);
    void indentBlocks(const QTextBlock &startBlock,
                      const QTextBlock &endBlock,
                      const QChar &typedChar,
                      int cursorPositionInEditor);
    TextEditor::Replacements indentsFor(QTextBlock startBlock,
                                        const QTextBlock &endBlock,
                                        const QChar &typedChar,
                                        int cursorPositionInEditor);
    TextEditor::Replacements replacements(QByteArray buffer,
                                          const QTextBlock &startBlock,
                                          const QTextBlock &endBlock,
                                          int cursorPositionInEditor,
                                          ReplacementsToKeep replacementsToKeep,
                                          const QChar &typedChar = QChar::Null,
                                          bool secondTry = false) const;
};

} // namespace ClangFormat
