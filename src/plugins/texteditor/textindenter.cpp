/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "textindenter.h"

#include <QTextDocument>
#include <QTextCursor>

using namespace TextEditor;

TextIndenter::TextIndenter(QTextDocument *doc)
    : Indenter(doc)
{}

TextIndenter::~TextIndenter() = default;

// Indent a text block based on previous line.
// Simple text paragraph layout:
// aaaa aaaa
//
//   bbb bb
//   bbb bb
//
//  - list
//    list line2
//
//  - listn
//
// ccc
//
// @todo{Add formatting to wrap paragraphs. This requires some
// hoops as the current indentation routines are not prepared
// for additional block being inserted. It might be possible
// to do in 2 steps (indenting/wrapping)}
int TextIndenter::indentFor(const QTextBlock &block,
                            const TabSettings &tabSettings,
                            int cursorPositionInEditor)
{
    Q_UNUSED(tabSettings)
    Q_UNUSED(cursorPositionInEditor)

    QTextBlock previous = block.previous();
    if (!previous.isValid())
        return 0;

    const QString previousText = previous.text();
    // Empty line indicates a start of a new paragraph. Leave as is.
    if (previousText.isEmpty() || previousText.trimmed().isEmpty())
        return 0;

    return tabSettings.indentationColumn(previousText);
}

IndentationForBlock TextIndenter::indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                       const TabSettings &tabSettings,
                                                       int /*cursorPositionInEditor*/)
{
    IndentationForBlock ret;
    for (const QTextBlock &block : blocks)
        ret.insert(block.blockNumber(), indentFor(block, tabSettings));
    return ret;
}

void TextIndenter::indentBlock(const QTextBlock &block,
                               const QChar &typedChar,
                               const TabSettings &tabSettings,
                               int /*cursorPositionInEditor*/)
{
    Q_UNUSED(typedChar)
    const int indent = indentFor(block, tabSettings);
    if (indent < 0)
        return;
    tabSettings.indentLine(block, indent);
}

void TextIndenter::indent(const QTextCursor &cursor,
                          const QChar &typedChar,
                          const TabSettings &tabSettings,
                          int /*cursorPositionInEditor*/)
{
    if (cursor.hasSelection()) {
        QTextBlock block = m_doc->findBlock(cursor.selectionStart());
        const QTextBlock end = m_doc->findBlock(cursor.selectionEnd()).next();
        do {
            indentBlock(block, typedChar, tabSettings);
            block = block.next();
        } while (block.isValid() && block != end);
    } else {
        indentBlock(cursor.block(), typedChar, tabSettings);
    }
}

void TextIndenter::reindent(const QTextCursor &cursor,
                            const TabSettings &tabSettings,
                            int /*cursorPositionInEditor*/)
{
    if (cursor.hasSelection()) {
        QTextBlock block = m_doc->findBlock(cursor.selectionStart());
        const QTextBlock end = m_doc->findBlock(cursor.selectionEnd()).next();

        // skip empty blocks
        while (block.isValid() && block != end) {
            QString bt = block.text();
            if (tabSettings.firstNonSpace(bt) < bt.size())
                break;
            indentBlock(block, QChar::Null, tabSettings);
            block = block.next();
        }

        int previousIndentation = tabSettings.indentationColumn(block.text());
        indentBlock(block, QChar::Null, tabSettings);
        int currentIndentation = tabSettings.indentationColumn(block.text());
        int delta = currentIndentation - previousIndentation;

        block = block.next();
        while (block.isValid() && block != end) {
            tabSettings.reindentLine(block, delta);
            block = block.next();
        }
    } else {
        indentBlock(cursor.block(), QChar::Null, tabSettings);
    }
}

Utils::optional<TabSettings> TextIndenter::tabSettings() const
{
    return Utils::optional<TabSettings>();
}
