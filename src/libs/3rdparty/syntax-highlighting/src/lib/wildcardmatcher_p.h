/*
    Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

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

#ifndef KSYNTAXHIGHLIGHTING_WILDCARDMATCHER_P_H
#define KSYNTAXHIGHLIGHTING_WILDCARDMATCHER_P_H

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace KSyntaxHighlighting {

namespace WildcardMatcher
{
    /**
     * Matches a string against a given wildcard.
     * The wildcard supports '*' (".*" in regex) and '?' ("." in regex), not more.
     *
     * @param candidate       Text to match
     * @param wildcard        Wildcard to use
     * @param caseSensitive   Case-sensitivity flag
     * @return                True for an exact match, false otherwise
     */
    bool exactMatch(const QString &candidate, const QString &wildcard, bool caseSensitive = true);
}

}

#endif // KSYNTAXHIGHLIGHTING_WILDCARDMATCHER_P_H
