// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
