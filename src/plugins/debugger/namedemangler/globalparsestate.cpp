/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "globalparsestate.h"

#include "demanglerexceptions.h"
#include "parsetreenodes.h"

namespace Debugger {
namespace Internal {

char GlobalParseState::peek(int ahead)
{
    Q_ASSERT(m_pos >= 0);
    if (m_pos + ahead < m_mangledName.size())
        return m_mangledName[m_pos + ahead];
    return eoi;
}

char GlobalParseState::advance(int steps)
{
    Q_ASSERT(steps > 0);
    if (m_pos + steps > m_mangledName.size())
        throw ParseException(QLatin1String("Unexpected end of input"));

    const char c = m_mangledName[m_pos];
    m_pos += steps;
    return c;
}

QByteArray GlobalParseState::readAhead(int charCount) const
{
    QByteArray str;
    if (m_pos + charCount <= m_mangledName.size())
        str = m_mangledName.mid(m_pos, charCount);
    else
        str.fill(eoi, charCount);
    return str;
}

void GlobalParseState::addSubstitution(const QSharedPointer<ParseTreeNode> &node)
{
    m_substitutions << node->clone();
}

} // namespace Internal
} // namespace Debugger
