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

#include "glslindenter.h"

#include <cpptools/cppcodeformatter.h>
#include <cpptools/cpptoolssettings.h>
#include <cpptools/cppcodestylepreferences.h>
#include <texteditor/tabsettings.h>

#include <QChar>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>

namespace GlslEditor {
namespace Internal {

GlslIndenter::GlslIndenter()
{}

GlslIndenter::~GlslIndenter()
{}

bool GlslIndenter::isElectricCharacter(const QChar &ch) const
{
    return ch == QLatin1Char('{')
        || ch == QLatin1Char('}')
        || ch == QLatin1Char(':')
        || ch == QLatin1Char('#');
}

void GlslIndenter::indentBlock(QTextDocument *doc,
                               const QTextBlock &block,
                               const QChar &typedChar,
                               const TextEditor::TabSettings &tabSettings)
{
    Q_UNUSED(doc)

    // TODO: do something with it
    CppTools::QtStyleCodeFormatter codeFormatter(tabSettings,
              CppTools::CppToolsSettings::instance()->cppCodeStyle()->codeStyleSettings());

    codeFormatter.updateStateUntil(block);
    int indent;
    int padding;
    codeFormatter.indentFor(block, &indent, &padding);

    // Only reindent the current line when typing electric characters if the
    // indent is the same it would be if the line were empty.
    if (isElectricCharacter(typedChar)) {
        int newlineIndent;
        int newlinePadding;
        codeFormatter.indentForNewLineAfter(block.previous(), &newlineIndent, &newlinePadding);
        if (tabSettings.indentationColumn(block.text()) != newlineIndent + newlinePadding)
            return;
    }

    tabSettings.indentLine(block, indent + padding, padding);
}

void GlslIndenter::indent(QTextDocument *doc,
                          const QTextCursor &cursor,
                          const QChar &typedChar,
                          const TextEditor::TabSettings &tabSettings)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();

        // TODO: do something with it
        CppTools::QtStyleCodeFormatter codeFormatter(tabSettings,
                  CppTools::CppToolsSettings::instance()->cppCodeStyle()->codeStyleSettings());
        codeFormatter.updateStateUntil(block);

        QTextCursor tc = cursor;
        tc.beginEditBlock();
        do {
            int indent;
            int padding;
            codeFormatter.indentFor(block, &indent, &padding);
            tabSettings.indentLine(block, indent + padding, padding);
            codeFormatter.updateLineStateChange(block);
            block = block.next();
        } while (block.isValid() && block != end);
        tc.endEditBlock();
    } else {
        indentBlock(doc, cursor.block(), typedChar, tabSettings);
    }
}

int GlslIndenter::indentFor(const QTextBlock &block, const TextEditor::TabSettings &tabSettings)
{
    CppTools::QtStyleCodeFormatter codeFormatter(tabSettings,
              CppTools::CppToolsSettings::instance()->cppCodeStyle()->codeStyleSettings());

    codeFormatter.updateStateUntil(block);
    int indent;
    int padding;
    codeFormatter.indentFor(block, &indent, &padding);

    return indent;
}

TextEditor::IndentationForBlock
GlslIndenter::indentationForBlocks(const QVector<QTextBlock> &blocks,
                                   const TextEditor::TabSettings &tabSettings)
{
    CppTools::QtStyleCodeFormatter codeFormatter(tabSettings,
              CppTools::CppToolsSettings::instance()->cppCodeStyle()->codeStyleSettings());

    codeFormatter.updateStateUntil(blocks.last());

    TextEditor::IndentationForBlock ret;
    foreach (QTextBlock block, blocks) {
        int indent;
        int padding;
        codeFormatter.indentFor(block, &indent, &padding);
        ret.insert(block.blockNumber(), indent);
    }
    return ret;
}

} // namespace Internal
} // namespace GlslEditor
