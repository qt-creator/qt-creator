/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "glslindenter.h"

#include <cpptools/cppcodeformatter.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/tabsettings.h>

#include <QtCore/QChar>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>

using namespace GLSLEditor;
using namespace Internal;

GLSLIndenter::GLSLIndenter()
{}

GLSLIndenter::~GLSLIndenter()
{}

bool GLSLIndenter::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{') ||
        ch == QLatin1Char('}') ||
        ch == QLatin1Char(':') ||
        ch == QLatin1Char('#')) {
        return true;
    }
    return false;
}

void GLSLIndenter::indentBlock(QTextDocument *doc,
                               const QTextBlock &block,
                               const QChar &typedChar,
                               TextEditor::BaseTextEditor *editor)
{
    Q_UNUSED(doc)

    const TextEditor::TabSettings &ts = editor->tabSettings();
    CppTools::QtStyleCodeFormatter codeFormatter(ts);

    codeFormatter.updateStateUntil(block);
    int indent;
    int padding;
    codeFormatter.indentFor(block, &indent, &padding);

    // only reindent the current line when typing electric characters if the
    // indent is the same it would be if the line were empty
    if (isElectricCharacter(typedChar)) {
        int newlineIndent;
        int newlinePadding;
        codeFormatter.indentForNewLineAfter(block.previous(), &newlineIndent, &newlinePadding);
        if (ts.indentationColumn(block.text()) != newlineIndent + newlinePadding)
            return;
    }

    ts.indentLine(block, indent + padding, padding);
}

void GLSLIndenter::indent(QTextDocument *doc,
                          const QTextCursor &cursor,
                          const QChar &typedChar,
                          TextEditor::BaseTextEditor *editor)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();

        const TextEditor::TabSettings &ts = editor->tabSettings();
        CppTools::QtStyleCodeFormatter codeFormatter(ts);
        codeFormatter.updateStateUntil(block);

        QTextCursor tc = editor->textCursor();
        tc.beginEditBlock();
        do {
            int indent;
            int padding;
            codeFormatter.indentFor(block, &indent, &padding);
            ts.indentLine(block, indent + padding, padding);
            codeFormatter.updateLineStateChange(block);
            block = block.next();
        } while (block.isValid() && block != end);
        tc.endEditBlock();
    } else {
        indentBlock(doc, cursor.block(), typedChar, editor);
    }
}
