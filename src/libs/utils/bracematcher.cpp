/****************************************************************************
**
** Copyright (C) 2016 Konstantin Tokarev <annulen@yandex.ru>
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

#include "bracematcher.h"

#include <QTextDocument>
#include <QTextCursor>

/*!
    \class Utils::BraceMatcher
    \brief The BraceMatcher class implements a generic autocompleter of braces
    and quotes.

    This is a helper class for autocompleter implementations. To use it,
    define \e brace, \e quote, and \e delimiter characters for a given language.
*/

namespace Utils {

/*!
 * Adds a pair of characters, corresponding to \a opening and \a closing braces.
 */
void BraceMatcher::addBraceCharPair(const QChar opening, const QChar closing)
{
    m_braceChars[opening] = closing;
}

/*!
 * Adds a \a quote character.
 */
void BraceMatcher::addQuoteChar(const QChar quote)
{
    m_quoteChars << quote;
}

/*!
 * Adds a separator character \a sep that should be skipped when overtyping it.
 * For example, it could be ';' or ',' in C-like languages.
 */
void BraceMatcher::addDelimiterChar(const QChar sep)
{
    m_delimiterChars << sep;
}

bool BraceMatcher::shouldInsertMatchingText(const QTextCursor &tc) const
{
    QTextDocument *doc = tc.document();
    return shouldInsertMatchingText(doc->characterAt(tc.selectionEnd()));
}

bool BraceMatcher::shouldInsertMatchingText(const QChar lookAhead) const
{
    return lookAhead.isSpace()
        || isQuote(lookAhead)
        || isDelimiter(lookAhead)
        || isClosingBrace(lookAhead);
}

QString BraceMatcher::insertMatchingBrace(const QTextCursor &cursor,
                                          const QString &text,
                                          const QChar la,
                                          int *skippedChars) const
{
    if (text.length() != 1)
        return QString();

    if (!shouldInsertMatchingText(cursor))
        return QString();

    const QChar ch = text.at(0);
    if (isQuote(ch)) {
        if (la != ch)
            return QString(ch);
        ++*skippedChars;
        return QString();
    }

    if (isOpeningBrace(ch))
        return QString(m_braceChars[ch]);

    if (isDelimiter(ch) || isClosingBrace(ch)) {
        if (la == ch)
            ++*skippedChars;
    }

    return QString();
}

/*!
 * Returns true if the character \a c was added as one of character types.
 */
bool BraceMatcher::isKnownChar(const QChar c) const
{
    return isQuote(c) || isDelimiter(c) || isOpeningBrace(c) || isClosingBrace(c);
}

} // namespace Utils
