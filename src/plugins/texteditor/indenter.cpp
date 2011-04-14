/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "indenter.h"
#include "basetexteditor.h"
#include "tabsettings.h"

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
                             BaseTextEditorWidget *editor)
{
    Q_UNUSED(doc);
    Q_UNUSED(block);
    Q_UNUSED(typedChar);
    Q_UNUSED(editor);
}

void Indenter::indent(QTextDocument *doc,
                      const QTextCursor &cursor,
                      const QChar &typedChar,
                      BaseTextEditorWidget *editor)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();
        do {
            indentBlock(doc, block, typedChar, editor);
            block = block.next();
        } while (block.isValid() && block != end);
    } else {
        indentBlock(doc, cursor.block(), typedChar, editor);
    }
}

void Indenter::reindent(QTextDocument *doc, const QTextCursor &cursor, BaseTextEditorWidget *editor)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();

        const TabSettings &ts = editor->tabSettings();

        // skip empty blocks
        while (block.isValid() && block != end) {
            QString bt = block.text();
            if (ts.firstNonSpace(bt) < bt.size())
                break;
            indentBlock(doc, block, QChar::Null, editor);
            block = block.next();
        }

        int previousIndentation = ts.indentationColumn(block.text());
        indentBlock(doc, block, QChar::Null, editor);
        int currentIndentation = ts.indentationColumn(block.text());
        int delta = currentIndentation - previousIndentation;

        block = block.next();
        while (block.isValid() && block != end) {
            ts.reindentLine(block, delta);
            block = block.next();
        }
    } else {
        indentBlock(doc, cursor.block(), QChar::Null, editor);
    }
}
