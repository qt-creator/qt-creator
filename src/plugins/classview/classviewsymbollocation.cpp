/**************************************************************************
**
** Copyright (C) 2015 Denis Mingulov
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "classviewsymbollocation.h"
#include <QPair>
#include <QHash>

namespace ClassView {
namespace Internal {

/*!
    \class SymbolLocation
    \brief The SymbolLocation class stores information about symbol location
    to know the exact location to open when the user clicks on a tree item.

    This class might be used in QSet and QHash.
*/

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
