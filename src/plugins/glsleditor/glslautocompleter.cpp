/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "glslautocompleter.h"

#include <cplusplus/Token.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/BackwardsScanner.h>

#include <QLatin1Char>
#include <QTextCursor>

using namespace CPlusPlus;

namespace GlslEditor {
namespace Internal {

GlslCompleter::GlslCompleter()
{}

GlslCompleter::~GlslCompleter()
{}

bool GlslCompleter::contextAllowsAutoParentheses(const QTextCursor &cursor,
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

bool GlslCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    const Token tk = SimpleLexer::tokenAt(cursor.block().text(), cursor.positionInBlock(),
                                          BackwardsScanner::previousBlockState(cursor.block()),
                                          LanguageFeatures::defaultFeatures());

    // XXX Duplicated from CppEditor::isInComment to avoid tokenizing twice
    if (tk.isComment()) {
        const unsigned pos = cursor.selectionEnd() - cursor.block().position();

        if (pos == tk.utf16charsEnd()) {
            if (tk.is(T_CPP_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))
                return false;

            const int state = cursor.block().userState() & 0xFF;
            if (state > 0)
                return false;
        }

        if (pos < tk.utf16charsEnd())
            return false;
    } else if (tk.isStringLiteral() || tk.isCharLiteral()) {
        const unsigned pos = cursor.selectionEnd() - cursor.block().position();
        if (pos <= tk.utf16charsEnd())
            return false;
    }

    return true;
}

bool GlslCompleter::isInComment(const QTextCursor &cursor) const
{
    const Token tk = SimpleLexer::tokenAt(cursor.block().text(), cursor.positionInBlock(),
                                          BackwardsScanner::previousBlockState(cursor.block()),
                                          LanguageFeatures::defaultFeatures());

    if (tk.isComment()) {
        const unsigned pos = cursor.selectionEnd() - cursor.block().position();

        if (pos == tk.utf16charsEnd()) {
            if (tk.is(T_CPP_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))
                return true;

            const int state = cursor.block().userState() & 0xFF;
            if (state > 0)
                return true;
        }

        if (pos < tk.utf16charsEnd())
            return true;
    }

    return false;
}

QString GlslCompleter::insertMatchingBrace(const QTextCursor &cursor,
                                           const QString &text,
                                           QChar la,
                                           int *skippedChars) const
{
    return MatchingText::insertMatchingBrace(cursor, text, la, skippedChars);
}

QString GlslCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    return MatchingText::insertParagraphSeparator(cursor);
}

} // namespace Internal
} // namespace GlslEditor
