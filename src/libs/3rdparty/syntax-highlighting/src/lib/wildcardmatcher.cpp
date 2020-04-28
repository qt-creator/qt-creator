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

#include "wildcardmatcher_p.h"

using namespace KSyntaxHighlighting;

#include <QChar>
#include <QString>

static bool exactMatch(const QString &candidate, const QString &wildcard, int candidatePosFromRight, int wildcardPosFromRight, bool caseSensitive = true)
{
    for (; wildcardPosFromRight >= 0; wildcardPosFromRight--) {
        const auto ch = wildcard.at(wildcardPosFromRight).unicode();
        switch (ch) {
        case L'*':
            if (candidatePosFromRight == -1) {
                break;
            }

            if (wildcardPosFromRight == 0) {
                return true;
            }

            // Eat all we can and go back as far as we have to
            for (int j = -1; j <= candidatePosFromRight; j++) {
                if (exactMatch(candidate, wildcard, j, wildcardPosFromRight - 1)) {
                    return true;
                }
            }
            return false;

        case L'?':
            if (candidatePosFromRight == -1) {
                return false;
            }

            candidatePosFromRight--;
            break;

        default:
            if (candidatePosFromRight == -1) {
                return false;
            }

            const auto candidateCh = candidate.at(candidatePosFromRight).unicode();
            const auto match = caseSensitive ? (candidateCh == ch) : (QChar::toLower(candidateCh) == QChar::toLower(ch));
            if (match) {
                candidatePosFromRight--;
            } else {
                return false;
            }
        }
    }
    return true;
}

bool WildcardMatcher::exactMatch(const QString &candidate, const QString &wildcard, bool caseSensitive)
{
    return ::exactMatch(candidate, wildcard, candidate.length() - 1, wildcard.length() - 1, caseSensitive);
}
