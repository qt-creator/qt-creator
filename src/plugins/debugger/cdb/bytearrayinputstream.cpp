/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "bytearrayinputstream.h"

#include <ctype.h>

namespace Debugger {
namespace Internal {

ByteArrayInputStream::ByteArrayInputStream(QByteArray &ba) :
    m_target(ba), m_integerBase(10), m_hexPrefix(false), m_width(0)
{
}

void ByteArrayInputStream::appendSeparator(char c)
{
    if (!m_target.isEmpty() && !m_target.endsWith(c))
        m_target.append(c);
}

void hexPrefixOn(ByteArrayInputStream &bs)
{
    bs.setHexPrefix(true);
}

void hexPrefixOff(ByteArrayInputStream &bs)
{
    bs.setHexPrefix(false);
}

void hex(ByteArrayInputStream &bs)
{
    bs.setIntegerBase(16);
}

void dec(ByteArrayInputStream &bs)
{
    bs.setIntegerBase(10);
}

void blankSeparator(ByteArrayInputStream &bs)
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
