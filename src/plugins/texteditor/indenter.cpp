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

#include "indenter.h"
#include "tabsettings.h"
#include "textdocumentlayout.h"

#include <QTextDocument>
#include <QTextCursor>

using namespace TextEditor;

Indenter::Indenter()
{}

Indenter::~Indenter()
{}

bool Indenter::isElectricCharacter(const QChar &) const
{
    return false;
}

void Indenter::indentBlock(QTextDocument *doc,
                             const QTextBlock &block,
                             const QChar &typedChar,
                             const TabSettings &tabSettings)
{
    Q_UNUSED(doc);
    Q_UNUSED(typedChar);
    const int indent = indentFor(block, tabSettings);
    if (indent < 0)
        return;
    tabSettings.indentLine(block, indent);
}

void Indenter::indent(QTextDocument *doc,
                      const QTextCursor &cursor,
                      const QChar &typedChar,
                      const TabSettings &tabSettings)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();
        do {
            indentBlock(doc, block, typedChar, tabSettings);
            block = block.next();
        } while (block.isValid() && block != end);
    } else {
        indentBlock(doc, cursor.block(), typedChar, tabSettings);
    }
}

void Indenter::reindent(QTextDocument *doc, const QTextCursor &cursor, const TabSettings &tabSettings)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();

        // skip empty blocks
        while (block.isValid() && block != end) {
            QString bt = block.text();
            if (tabSettings.firstNonSpace(bt) < bt.size())
                break;
            indentBlock(doc, block, QChar::Null, tabSettings);
            block = block.next();
        }

        int previousIndentation = tabSettings.indentationColumn(block.text());
        indentBlock(doc, block, QChar::Null, tabSettings);
        int currentIndentation = tabSettings.indentationColumn(block.text());
        int delta = currentIndentation - previousIndentation;

        block = block.next();
        while (block.isValid() && block != end) {
            tabSettings.reindentLine(block, delta);
            block = block.next();
        }
    } else {
        indentBlock(doc, cursor.block(), QChar::Null, tabSettings);
    }
}

void Indenter::setCodeStylePreferences(ICodeStylePreferences *)
{

}

void Indenter::invalidateCache(QTextDocument *)
{
}

int Indenter::indentFor(const QTextBlock &block, const TabSettings &tabSettings)
{
    Q_UNUSED(block)
    Q_UNUSED(tabSettings)
    return -1;
}

IndentationForBlock Indenter::indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                   const TabSettings &tabSettings)
{
    IndentationForBlock ret;
    foreach (QTextBlock block, blocks)
        ret.insert(block.blockNumber(), indentFor(block, tabSettings));
    return ret;
}
