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

#include "glslautocompleter.h"

#include <cplusplus/MatchingText.h>

#include <QTextCursor>

namespace GlslEditor {
namespace Internal {

bool GlslCompleter::contextAllowsAutoBrackets(const QTextCursor &cursor,
                                              const QString &textToInsert) const
{
    return CPlusPlus::MatchingText::contextAllowsAutoParentheses(cursor, textToInsert);
}

bool GlslCompleter::contextAllowsAutoQuotes(const QTextCursor &cursor,
                                            const QString &textToInsert) const
{
    return CPlusPlus::MatchingText::contextAllowsAutoQuotes(cursor, textToInsert);
}

bool GlslCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    return CPlusPlus::MatchingText::contextAllowsElectricCharacters(cursor);
}

bool GlslCompleter::isInComment(const QTextCursor &cursor) const
{
    return CPlusPlus::MatchingText::isInCommentHelper(cursor);
}

QString GlslCompleter::insertMatchingBrace(const QTextCursor &cursor, const QString &text,
                                           QChar lookAhead, bool skipChars, int *skippedChars) const
{
    return CPlusPlus::MatchingText::insertMatchingBrace(cursor, text, lookAhead,
                                                        skipChars, skippedChars);
}

QString GlslCompleter::insertMatchingQuote(const QTextCursor &cursor, const QString &text,
                                           QChar lookAhead, bool skipChars, int *skippedChars) const
{
    return CPlusPlus::MatchingText::insertMatchingQuote(cursor, text, lookAhead,
                                                        skipChars, skippedChars);
}

QString GlslCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    return CPlusPlus::MatchingText::insertParagraphSeparator(cursor);
}

} // namespace Internal
} // namespace GlslEditor
