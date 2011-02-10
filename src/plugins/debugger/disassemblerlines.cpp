/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "disassemblerlines.h"

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

namespace Debugger {
namespace Internal {

DisassemblerLine::DisassemblerLine(const QString &unparsed)
{
    int pos = -1;
    for (int i = 0; i != unparsed.size(); ++i) {
        uint c = unparsed.at(i).unicode();
        if (c == ' ' || c == ':' || c == '\t') {
            pos = i;
            break;
        }
    }

    // Mac gdb has an overflow reporting 64bit addresses causing the instruction
    // to follow the last digit "0x000000013fff4810mov 1,1". Truncate here.
    if (pos > 19 && unparsed.mid(3, 16).toULongLong())
        pos = 19;

    QString addr = unparsed.left(pos);
    // MSVC 64bit: Remove 64bit separator 00000000`00a45000'.
    if (addr.size() >= 9 && addr.at(8) == QLatin1Char('`'))
        addr.remove(8, 1);

    if (addr.endsWith(':')) // clang
        addr.chop(1);
    if (addr.startsWith(QLatin1String("0x")))
        addr.remove(0, 2);
    address = addr.toULongLong(0, 16);
    if (address)
        data = unparsed.mid(pos + 1);
    else
        data = unparsed;
}

int DisassemblerLines::lineForAddress(quint64 address) const
{
    return m_rowCache.value(address);
}

bool DisassemblerLines::coversAddress(quint64 address) const
{
    return m_rowCache.value(address) != 0;
}

void DisassemblerLines::appendComment(const QString &comment)
{
    DisassemblerLine dl;
    dl.data = comment;
    m_data.append(dl);
}

void DisassemblerLines::appendLine(const DisassemblerLine &dl)
{
    m_data.append(dl);
    m_rowCache[dl.address] = m_data.size();
}

} // namespace Internal
} // namespace Debugger
