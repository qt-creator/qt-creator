/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "classviewsymbollocation.h"
#include <QtCore/QPair>
#include <QtCore/QHash>

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
