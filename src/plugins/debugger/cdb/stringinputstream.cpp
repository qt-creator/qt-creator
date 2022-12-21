// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stringinputstream.h"

#include <ctype.h>

namespace Debugger {
namespace Internal {

StringInputStream::StringInputStream(QString &str) :
    m_target(str)
{
}

void StringInputStream::appendSeparator(char c)
{
    if (!m_target.isEmpty() && !m_target.endsWith(c))
        m_target.append(c);
}

void hexPrefixOn(StringInputStream &bs)
{
    bs.setHexPrefix(true);
}

void hexPrefixOff(StringInputStream &bs)
{
    bs.setHexPrefix(false);
}

void hex(StringInputStream &bs)
{
    bs.setIntegerBase(16);
}

void dec(StringInputStream &bs)
{
    bs.setIntegerBase(10);
}

void blankSeparator(StringInputStream &bs)
{
    bs.appendSeparator();
}

QByteArray trimFront(QByteArray in)
{
    if (in.isEmpty())
        return in;
    const int size = in.size();
    int pos = 0;
    for ( ; pos < size && isspace(in.at(pos)); pos++) ;
    if (pos)
        in.remove(0, pos);
    return in;
}

QByteArray trimBack(QByteArray in)
{
    if (in.isEmpty())
        return in;
    const int size = in.size();
    int pos = size - 1;
    for ( ; pos >= 0 && isspace(in.at(pos)); pos--) ;
    if (pos != size - 1)
        in.truncate(pos + 1);
    return in;
}


// Simplify: replace tabs, find all occurrences
// of 2 blanks, check further up for blanks and remove that bit.
QByteArray simplify(const QByteArray &inIn)
{
    if (inIn.isEmpty())
        return inIn;
    QByteArray in = trimFront(trimBack(inIn));
    in.replace('\t', ' ');
    in.replace('\n', ' ');
    in.replace('\r', ' ');
    const QByteArray twoBlanks = "  ";
    while (true) {
        const int pos = in.indexOf(twoBlanks);
        if (pos != -1) {
            const int size = in.size();
            int endPos = pos + twoBlanks.size();
            for ( ; endPos < size && in.at(endPos) == ' '; endPos++) ;
            in.remove(pos + 1, endPos - pos - 1);
        } else {
            break;
        }
    }
    return in;
}

} // namespace Internal
} // namespace Debugger
