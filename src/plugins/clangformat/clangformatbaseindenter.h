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

class ClangFormatBaseIndenter : public TextEditor::Indenter
{
public:
    ClangFormatBaseIndenter(QTextDocument *doc);

    TextEditor::IndentationForBlock indentationForBlocks(
        const QVector<QTextBlock> &blocks, const TextEditor::TabSettings & /*tabSettings*/) override;
    void indent(const QTextCursor &cursor,
                const QChar &typedChar,
                const TextEditor::TabSettings & /*tabSettings*/) override;

    void reindent(const QTextCursor &cursor,
                  const TextEditor::TabSettings & /*tabSettings*/) override;

    void formatOrIndent(const QTextCursor &cursor,
                        const TextEditor::TabSettings &tabSettings,
                        int cursorPositionInEditor = -1) override;
    TextEditor::Replacements format(const QTextCursor &cursor,
                                    const TextEditor::TabSettings & /*tabSettings*/) override;

    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings & /*tabSettings*/) override;

    int indentFor(const QTextBlock &block, const TextEditor::TabSettings & /*tabSettings*/) override;

    bool isElectricCharacter(const QChar &ch) const override;

protected:
    virtual clang::format::FormatStyle styleForFile() const;
    virtual bool formatCodeInsteadOfIndent() const { return false; }

private:
    TextEditor::Replacements format(const QTextCursor &cursor);
    void indent(const QTextCursor &cursor, const QChar &typedChar);
    void indentBlock(const QTextBlock &block, const QChar &typedChar);
    int indentFor(const QTextBlock &block);
    TextEditor::Replacements replacements(QByteArray buffer,
                                          int utf8Offset,
                                          int utf8Length,
                                          const QTextBlock &block,
                                          const QChar &typedChar = QChar::Null,
                                          bool onlyIndention = true,
                                          bool secondTry = false) const;
};

} // namespace ClangFormat
