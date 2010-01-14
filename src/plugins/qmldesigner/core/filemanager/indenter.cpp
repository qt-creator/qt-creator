/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "indenter.h"

using namespace QmlDesigner;

Indenter::Indenter()
{
}

//static int blockStartState(const QTextBlock &block)
//{
//    int state = block.userState();
//
//    if (state == -1)
//        return 0;
//    else
//        return state & 0xff;
//}
//
//void ScriptEditor::indentBlock(QTextDocument *, QTextBlock block, QChar typedChar)
//{
//    TextEditor::TabSettings ts = tabSettings();
//
//    QTextCursor tc(block);
//
//    const QString blockText = block.text();
//    int startState = blockStartState(block.previous());
//
//    QScriptIncrementalScanner scanner;
//    const QList<QScriptIncrementalScanner::Token> tokens = scanner(blockText, startState);
//
//    if (! tokens.isEmpty()) {
//        const QScriptIncrementalScanner::Token tk = tokens.first();
//
//        if (tk.is(QScriptIncrementalScanner::Token::RightBrace)
//                || tk.is(QScriptIncrementalScanner::Token::RightBracket)) {
//            if (TextEditor::TextBlockUserData::findPreviousBlockOpenParenthesis(&tc)) {
//                const QString text = tc.block().text();
//                int indent = ts.columnAt(text, ts.firstNonSpace(text));
//                ts.indentLine(block, indent);
//                return;
//            }
//        }
//    }
//
//    int initialIndent = 0;
//    for (QTextBlock it = block.previous(); it.isValid(); it = it.previous()) {
//        const QString text = it.text();
//
//        if (! text.isEmpty()) {
//            initialIndent = ts.columnAt(text, ts.firstNonSpace(text));
//            break;
//        }
//    }
//
//    const int braceDepth = blockBraceDepth(block.previous());
//    const int previousBraceDepth = blockBraceDepth(block.previous().previous());
//    const int delta = qMax(0, braceDepth - previousBraceDepth);
//    int indent = initialIndent + (delta * ts.m_indentSize);
//    ts.indentLine(block, indent);
//}
//
