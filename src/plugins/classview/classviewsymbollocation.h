/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
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

#ifndef CLASSVIEWSYMBOLLOCATION_H
#define CLASSVIEWSYMBOLLOCATION_H

#include <QMetaType>
#include <QString>

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
