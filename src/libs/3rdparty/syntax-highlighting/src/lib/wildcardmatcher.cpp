/*
    SPDX-FileCopyrightText: 2007 Sebastian Pipping <webmaster@hartwork.org>

    SPDX-License-Identifier: MIT
*/

#include "wildcardmatcher.h"

using namespace KSyntaxHighlighting;

#include <QChar>

namespace
{
bool wildcardMatch(QStringView candidate, QStringView wildcard, int candidatePosFromRight, int wildcardPosFromRight)
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
                if (wildcardMatch(candidate, wildcard, j, wildcardPosFromRight - 1)) {
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
            if (candidateCh == ch) {
                candidatePosFromRight--;
            } else {
                return false;
            }
        }
    }
    return candidatePosFromRight == -1;
}

} // unnamed namespace

bool WildcardMatcher::exactMatch(QStringView candidate, QStringView wildcard)
{
    return ::wildcardMatch(candidate, wildcard, candidate.length() - 1, wildcard.length() - 1);
}
