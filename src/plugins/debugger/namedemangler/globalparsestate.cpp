/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
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

void GlobalParseState::addSubstitution(const ParseTreeNode *node)
{
    addSubstitution(node->toByteArray());
}

void GlobalParseState::addSubstitution(const QByteArray &symbol)
{
    if (!symbol.isEmpty() && !m_substitutions.contains(symbol))
        m_substitutions.append(symbol);
}

} // namespace Internal
} // namespace Debugger
