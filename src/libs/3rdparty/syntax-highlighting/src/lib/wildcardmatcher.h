/*
    SPDX-FileCopyrightText: 2007 Sebastian Pipping <webmaster@hartwork.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_WILDCARDMATCHER_H
#define KSYNTAXHIGHLIGHTING_WILDCARDMATCHER_H

#include "ksyntaxhighlighting_export.h"

#include <QStringView>

namespace KSyntaxHighlighting
{
namespace WildcardMatcher
{
/**
 * Matches a string against a given wildcard case-sensitively.
 * The wildcard supports '*' (".*" in regex) and '?' ("." in regex), not more.
 *
 * @param candidate       Text to match
 * @param wildcard        Wildcard to use
 * @return                True for an exact match, false otherwise
 *
 * @since 5.86
 */
KSYNTAXHIGHLIGHTING_EXPORT bool exactMatch(QStringView candidate, QStringView wildcard);
}

}

#endif // KSYNTAXHIGHLIGHTING_WILDCARDMATCHER_H
