/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "classviewsymbollocation.h"
#include <QPair>
#include <QHash>

namespace ClassView {
namespace Internal {

SymbolLocation::SymbolLocation() :
    m_line(0),
    m_column(0),
    m_hash(0)
{
}

SymbolLocation::SymbolLocation(QString file, int lineNumber, int columnNumber) :
    m_fileName(file),
    m_line(lineNumber),
    m_column(columnNumber)
{
    if (m_column < 0)
        m_column = 0;

    // pre-computate hash value
    m_hash = qHash(qMakePair(m_fileName, qMakePair(m_line, m_column)));
}

} // namespace Internal
} // namespace ClassView
