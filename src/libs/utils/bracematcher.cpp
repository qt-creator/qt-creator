/**************************************************************************
**
** Copyright (c) 2013 Konstantin Tokarev <annulen@yandex.ru>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "bracematcher.h"

#include <QTextDocument>
#include <QTextCursor>

/*!
    \class Utils::BraceMatcher
    \brief Generic autocompleter of braces and quotes.

    This is a helper class for autocompleter implementations. To use it,
    define "brace", "quote", and "delimiter" characters for given language.
*/

namespace Utils {

/*!
 * Adds a pair of characters, corresponding to opening and closing braces.
 */
void BraceMatcher::addBraceCharPair(const QChar opening, const QChar closing)
{
    m_braceChars[opening] = closing;
}

/*!
 * Adds quote character
 */
void BraceMatcher::addQuoteChar(const QChar quote)
{
    m_quoteChars << quote;
}

/*!
 * Adds separator character that should be skipped when overtyping it, e.g.
 * it could be ';' or ',' in C-like languages.
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
 * Returns true if given character was added as one of character types.
 */
bool BraceMatcher::isKnownChar(const QChar c) const
{
    return isQuote(c) || isDelimiter(c) || isOpeningBrace(c) || isClosingBrace(c);
}

} // namespace Utils
