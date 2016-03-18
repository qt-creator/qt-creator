/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
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

#pragma once

#include <QMetaType>
#include <QString>

namespace ClassView {
namespace Internal {

class SymbolLocation
{
public:
    //! Default constructor
    SymbolLocation();

    //! Constructor
    explicit SymbolLocation(QString file, int lineNumber = 0, int columnNumber = 0);

    inline const QString &fileName() const { return m_fileName; }
    inline int line() const { return m_line; }
    inline int column() const { return m_column; }
    inline int hash() const { return m_hash; }
    inline bool operator==(const SymbolLocation &other) const
    {
        return line() == other.line() && column() == other.column()
            && fileName() == other.fileName();
    }

private:
    QString m_fileName; //!< file name
    int m_line;         //!< line number
    int m_column;       //!< column
    int m_hash;         //!< precalculated hash value for the object, to speed up qHash
};

//! qHash overload for QHash/QSet
inline uint qHash(const SymbolLocation &location)
{
    return location.hash();
}

} // namespace Internal
} // namespace ClassView

Q_DECLARE_METATYPE(ClassView::Internal::SymbolLocation)
