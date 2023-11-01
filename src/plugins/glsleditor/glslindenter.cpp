// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glslindenter.h"

#include <cppeditor/cppcodeformatter.h>
#include <cppeditor/cpptoolssettings.h>
#include <cppeditor/cppcodestylepreferences.h>
#include <texteditor/tabsettings.h>

#include <QChar>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>

namespace GlslEditor {
namespace Internal {

GlslIndenter::GlslIndenter(QTextDocument *doc)
    : TextEditor::TextIndenter(doc)
{}
GlslIndenter::~GlslIndenter() = default;

bool GlslIndenter::isElectricCharacter(const QChar &ch) const
{
    return ch == QLatin1Char('{') || ch == QLatin1Char('}') || ch == QLatin1Char(':')
           || ch == QLatin1Char('#');
}

void GlslIndenter::indentBlock(const QTextBlock &block,
                               const QChar &typedChar,
                               const TextEditor::TabSettings &tabSettings,
                               int /*cursorPositionInEditor*/)
{
    // TODO: do something with it
    CppEditor::QtStyleCodeFormatter
        codeFormatter(tabSettings, CppEditor::CppToolsSettings::cppCodeStyle()->codeStyleSettings());

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

void GlslIndenter::indent(const QTextCursor &cursor,
                          const QChar &typedChar,
                          const TextEditor::TabSettings &tabSettings,
                          int /*cursorPositionInEditor*/)
{
    if (cursor.hasSelection()) {
        QTextBlock block = m_doc->findBlock(cursor.selectionStart());
        const QTextBlock end = m_doc->findBlock(cursor.selectionEnd()).next();

        // TODO: do something with it
        CppEditor::QtStyleCodeFormatter codeFormatter(tabSettings,
                                                     CppEditor::CppToolsSettings::cppCodeStyle()
                                                         ->codeStyleSettings());
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
        indentBlock(cursor.block(), typedChar, tabSettings);
    }
}

int GlslIndenter::indentFor(const QTextBlock &block,
                            const TextEditor::TabSettings &tabSettings,
                            int /*cursorPositionInEditor*/)
{
    CppEditor::QtStyleCodeFormatter
        codeFormatter(tabSettings, CppEditor::CppToolsSettings::cppCodeStyle()->codeStyleSettings());

    codeFormatter.updateStateUntil(block);
    int indent;
    int padding;
    codeFormatter.indentFor(block, &indent, &padding);

    return indent;
}

TextEditor::IndentationForBlock GlslIndenter::indentationForBlocks(
    const QVector<QTextBlock> &blocks,
    const TextEditor::TabSettings &tabSettings,
    int /*cursorPositionInEditor*/)
{
    CppEditor::QtStyleCodeFormatter
        codeFormatter(tabSettings, CppEditor::CppToolsSettings::cppCodeStyle()->codeStyleSettings());

    codeFormatter.updateStateUntil(blocks.last());

    TextEditor::IndentationForBlock ret;
    for (const QTextBlock &block : blocks) {
        int indent;
        int padding;
        codeFormatter.indentFor(block, &indent, &padding);
        ret.insert(block.blockNumber(), indent);
    }
    return ret;
}

} // namespace Internal
} // namespace GlslEditor
