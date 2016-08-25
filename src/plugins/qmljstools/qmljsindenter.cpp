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

#include "qmljsindenter.h"

#include <qmljstools/qmljsqtstylecodeformatter.h>
#include <texteditor/tabsettings.h>

#include <QChar>
#include <QTextDocument>
#include <QTextBlock>

using namespace QmlJSEditor;
using namespace Internal;

Indenter::Indenter()
{}

Indenter::~Indenter()
{}

bool Indenter::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{')
            || ch == QLatin1Char('}')
            || ch == QLatin1Char(']')
            || ch == QLatin1Char(':'))
        return true;
    return false;
}

void Indenter::indentBlock(QTextDocument *doc,
                           const QTextBlock &block,
                           const QChar &typedChar,
                           const TextEditor::TabSettings &tabSettings)
{
    Q_UNUSED(doc)

    const int depth = indentFor(block, tabSettings);
    if (depth == -1)
        return;

    QmlJSTools::CreatorCodeFormatter codeFormatter(tabSettings);
    codeFormatter.updateStateUntil(block);

    if (isElectricCharacter(typedChar)) {
        // only reindent the current line when typing electric characters if the
        // indent is the same it would be if the line were empty
        const int newlineIndent = codeFormatter.indentForNewLineAfter(block.previous());
        if (tabSettings.indentationColumn(block.text()) != newlineIndent)
            return;
    }

    tabSettings.indentLine(block, depth);
}

void Indenter::invalidateCache(QTextDocument *doc)
{
    QmlJSTools::CreatorCodeFormatter codeFormatter;
    codeFormatter.invalidateCache(doc);
}


int Indenter::indentFor(const QTextBlock &block,
                        const TextEditor::TabSettings &tabSettings)
{
    QmlJSTools::CreatorCodeFormatter codeFormatter(tabSettings);
    codeFormatter.updateStateUntil(block);
    return codeFormatter.indentFor(block);
}


TextEditor::IndentationForBlock
Indenter::indentationForBlocks(const QVector<QTextBlock> &blocks,
                               const TextEditor::TabSettings &tabSettings)
{
    QmlJSTools::CreatorCodeFormatter codeFormatter(tabSettings);


    codeFormatter.updateStateUntil(blocks.last());

    TextEditor::IndentationForBlock ret;
    foreach (QTextBlock block, blocks)
        ret.insert(block.blockNumber(), codeFormatter.indentFor(block));
    return ret;
}
