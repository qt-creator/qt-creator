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
#include "MatchingText.h"
#include "BackwardsScanner.h"

#include <Token.h>
#include <QtDebug>

using namespace CPlusPlus;

MatchingText::MatchingText()
{ }

QString MatchingText::insertParagraphSeparator(const QTextCursor &tc) const
{
    BackwardsScanner tk(tc, 400);
    int index = tk.startToken();

    if (tk[index - 1].isNot(T_LBRACE))
        return QString(); // nothing to do.

    --index; // consume the `{'

    const SimpleToken &token = tk[index - 1];

    if (token.is(T_STRING_LITERAL) && tk[index - 2].is(T_EXTERN)) {
        // recognized extern "C"
        return QLatin1String("}");

    } else if (token.is(T_IDENTIFIER)) {
        int i = index - 1;

        forever {
            const SimpleToken &current = tk[i - 1];

            if (current.is(T_EOF_SYMBOL))
                break;

            else if (current.is(T_CLASS) || current.is(T_STRUCT) || current.is(T_UNION))
                return QLatin1String("};"); // found a class key.

            else if (current.is(T_NAMESPACE))
                return QLatin1String("}"); // found a namespace declaration

            else if (current.is(T_SEMICOLON))
                break; // found the `;' sync token

            else if (current.is(T_LBRACE) || current.is(T_RBRACE))
                break; // braces are considered sync tokens

            else if (current.is(T_LPAREN) || current.is(T_RPAREN))
                break; // sync token

            else if (current.is(T_LBRACKET) || current.is(T_RBRACKET))
                break; // sync token

            --i;
        }
    }

    if (token.is(T_NAMESPACE)) {
        // anonymous namespace
        return QLatin1String("}");

    } else if (token.is(T_CLASS) || token.is(T_STRUCT) || token.is(T_UNION)) {
        if (tk[index - 2].is(T_TYPEDEF)) {
            // recognized:
            //   typedef struct {
            //
            // in this case we don't want to insert the extra semicolon+newline.
            return QLatin1String("}");
        }

        // anonymous class
        return QLatin1String("};");

    } else if (token.is(T_RPAREN)) {
        // search the matching brace.
        const int lparenIndex = tk.startOfMatchingBrace(index);

        if (lparenIndex == index) {
            // found an unmatched brace. We don't really know to do in this case.
            return QString();
        }

        // look at the token before the matched brace
        const SimpleToken &tokenBeforeBrace = tk[lparenIndex - 1];

        if (tokenBeforeBrace.is(T_IF)) {
            // recognized an if statement
            return QLatin1String("}");

        } else if (tokenBeforeBrace.is(T_FOR) || tokenBeforeBrace.is(T_WHILE)) {
            // recognized a for-like statement
            return QLatin1String("}");

        }

        // if we reached this point there is a good chance that we are parsing a function definition
        return QLatin1String("}");
    }

    // match the block
    return QLatin1String("}");
}
