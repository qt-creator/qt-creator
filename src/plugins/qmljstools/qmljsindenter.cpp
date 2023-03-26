// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsindenter.h"

#include <qmljstools/qmljsqtstylecodeformatter.h>
#include <texteditor/tabsettings.h>

#include <QChar>
#include <QTextDocument>
#include <QTextBlock>

using namespace QmlJSEditor;
using namespace Internal;

Indenter::Indenter(QTextDocument *doc)
    : TextEditor::TextIndenter(doc)
{}

Indenter::~Indenter() = default;

bool Indenter::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{')
            || ch == QLatin1Char('}')
            || ch == QLatin1Char(']')
            || ch == QLatin1Char(':'))
        return true;
    return false;
}

void Indenter::indentBlock(const QTextBlock &block,
                           const QChar &typedChar,
                           const TextEditor::TabSettings &tabSettings,
                           int /*cursorPositionInEditor*/)
{
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

void Indenter::invalidateCache()
{
    QmlJSTools::CreatorCodeFormatter codeFormatter;
    codeFormatter.invalidateCache(m_doc);
}

int Indenter::indentFor(const QTextBlock &block,
                        const TextEditor::TabSettings &tabSettings,
                        int /*cursorPositionInEditor*/)
{
    QmlJSTools::CreatorCodeFormatter codeFormatter(tabSettings);
    codeFormatter.updateStateUntil(block);
    return codeFormatter.indentFor(block);
}

int Indenter::visualIndentFor(const QTextBlock &block, const TextEditor::TabSettings &tabSettings)
{
    return indentFor(block, tabSettings);
}

TextEditor::IndentationForBlock Indenter::indentationForBlocks(
    const QVector<QTextBlock> &blocks,
    const TextEditor::TabSettings &tabSettings,
    int /*cursorPositionInEditor*/)
{
    QmlJSTools::CreatorCodeFormatter codeFormatter(tabSettings);

    codeFormatter.updateStateUntil(blocks.last());

    TextEditor::IndentationForBlock ret;
    for (QTextBlock block : blocks)
        ret.insert(block.blockNumber(), codeFormatter.indentFor(block));
    return ret;
}
