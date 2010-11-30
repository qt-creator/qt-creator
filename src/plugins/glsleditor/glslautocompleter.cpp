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

#include "glslautocompleter.h"

#include <Token.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/BackwardsScanner.h>

#include <QtCore/QLatin1Char>
#include <QtGui/QTextCursor>

using namespace GLSLEditor;
using namespace Internal;
using namespace CPlusPlus;

GLSLCompleter::GLSLCompleter()
{}

GLSLCompleter::~GLSLCompleter()
{}

bool GLSLCompleter::contextAllowsAutoParentheses(const QTextCursor &cursor,
                                                 const QString &textToInsert) const
{
    QChar ch;

    if (! textToInsert.isEmpty())
        ch = textToInsert.at(0);

    if (! (MatchingText::shouldInsertMatchingText(cursor)
           || ch == QLatin1Char('\'')
           || ch == QLatin1Char('"')))
        return false;
    else if (isInComment(cursor))
        return false;

    return true;
}

bool GLSLCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    const Token tk = SimpleLexer::tokenAt(cursor.block().text(), cursor.positionInBlock(),
                                          BackwardsScanner::previousBlockState(cursor.block()));

    // XXX Duplicated from CPPEditor::isInComment to avoid tokenizing twice
    if (tk.isComment()) {
        const unsigned pos = cursor.selectionEnd() - cursor.block().position();

        if (pos == tk.end()) {
            if (tk.is(T_CPP_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))
                return false;

            const int state = cursor.block().userState() & 0xFF;
            if (state > 0)
                return false;
        }

        if (pos < tk.end())
            return false;
    }
    else if (tk.is(T_STRING_LITERAL) || tk.is(T_WIDE_STRING_LITERAL)
        || tk.is(T_CHAR_LITERAL) || tk.is(T_WIDE_CHAR_LITERAL)) {

        const unsigned pos = cursor.selectionEnd() - cursor.block().position();
        if (pos <= tk.end())
            return false;
    }

    return true;
}

bool GLSLCompleter::isInComment(const QTextCursor &cursor) const
{
    const Token tk = SimpleLexer::tokenAt(cursor.block().text(), cursor.positionInBlock(),
                                          BackwardsScanner::previousBlockState(cursor.block()));

    if (tk.isComment()) {
        const unsigned pos = cursor.selectionEnd() - cursor.block().position();

        if (pos == tk.end()) {
            if (tk.is(T_CPP_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))
                return true;

            const int state = cursor.block().userState() & 0xFF;
            if (state > 0)
                return true;
        }

        if (pos < tk.end())
            return true;
    }

    return false;
}

QString GLSLCompleter::insertMatchingBrace(const QTextCursor &cursor,
                                           const QString &text,
                                           QChar la,
                                           int *skippedChars) const
{
    MatchingText m;
    return m.insertMatchingBrace(cursor, text, la, skippedChars);
}

QString GLSLCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    MatchingText m;
    return m.insertParagraphSeparator(cursor);
}
