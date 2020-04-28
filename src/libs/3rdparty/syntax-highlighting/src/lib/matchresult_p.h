/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef KSYNTAXHIGHLIGHTING_MATCHRESULT_P_H
#define KSYNTAXHIGHLIGHTING_MATCHRESULT_P_H

#include <QStringList>

namespace KSyntaxHighlighting
{
/**
 * Storage for match result of a Rule.
 * Heavily used internally during highlightLine, therefore completely inline.
 */
class MatchResult
{
public:
    /**
     * Match at given offset found.
     * @param offset offset of match
     */
    MatchResult(const int offset)
        : m_offset(offset)
    {
    }

    /**
     * Match at given offset found with additional skip offset.
     */
    explicit MatchResult(const int offset, const int skipOffset)
        : m_offset(offset)
        , m_skipOffset(skipOffset)
    {
    }

    /**
     * Match at given offset found with additional captures.
     * @param offset offset of match
     * @param captures captures of the match
     */
    explicit MatchResult(const int offset, const QStringList &captures)
        : m_offset(offset)
        , m_captures(captures)
    {
    }

    /**
     * Offset of the match
     * @return offset of the match
     */
    int offset() const
    {
        return m_offset;
    }

    /**
     * Skip offset of the match
     * @return skip offset of the match, no match possible until this offset is reached
     */
    int skipOffset() const
    {
        return m_skipOffset;
    }

    /**
     * Captures of the match.
     * @return captured text of this match
     */
    const QStringList &captures() const
    {
        return m_captures;
    }

private:
    /**
     * match offset, filled in all constructors
     */
    int m_offset;

    /**
     * skip offset, optional
     */
    int m_skipOffset = 0;

    /**
     * captures, optional
     */
    QStringList m_captures;
};
}

#endif // KSYNTAXHIGHLIGHTING_MATCHRESULT_P_H
