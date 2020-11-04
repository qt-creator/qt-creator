/*
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "worddelimiters_p.h"

using namespace KSyntaxHighlighting;

WordDelimiters::WordDelimiters()
    : asciiDelimiters{}
{
    for(const char *p = "\t !%&()*+,-./:;<=>?[\\]^{|}~"; *p; ++p)
        // int(*p) fix -Wchar-subscripts
        asciiDelimiters[int(*p)] = true;
}

bool WordDelimiters::contains(QChar c) const
{
    if (c.unicode() < 128)
        return asciiDelimiters[c.unicode()];
    // perf tells contains is MUCH faster than binary search here, very short array
    return notAsciiDelimiters.contains(c);
}

void WordDelimiters::append(QChar c)
{
    if (c.unicode() < 128) {
        asciiDelimiters[c.unicode()] = true;
    } else {
        notAsciiDelimiters.append(c);
    }
}

void WordDelimiters::remove(QChar c)
{
    if (c.unicode() < 128) {
        asciiDelimiters[c.unicode()] = false;
    } else {
        notAsciiDelimiters.remove(c);
    }
}
