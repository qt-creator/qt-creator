// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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

SymbolLocation::SymbolLocation(const QString &file, int lineNumber, int columnNumber)
    : m_fileName(file)
    , m_line(lineNumber)
    , m_column(qMax(columnNumber, 0))
    , m_hash(qHashMulti(0, m_fileName, m_line, m_column))
{
}

} // namespace Internal
} // namespace ClassView
