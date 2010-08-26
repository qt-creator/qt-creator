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

#ifndef CLASSVIEWSYMBOLLOCATION_H
#define CLASSVIEWSYMBOLLOCATION_H

#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace ClassView {
namespace Internal {

/*!
   \class SymbolLocation
   \brief Special struct to store information about symbol location (to find which exactly location
   has to be open when the user clicks on any tree item. It might be used in QSet/QHash.
 */
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

#endif // CLASSVIEWSYMBOLLOCATION_H
