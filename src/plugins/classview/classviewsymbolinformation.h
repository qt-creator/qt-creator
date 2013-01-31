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

#ifndef CLASSVIEWSYMBOLINFORMATION_H
#define CLASSVIEWSYMBOLINFORMATION_H

#include <QMetaType>
#include <QString>

#include <limits.h>

namespace ClassView {
namespace Internal {

/*!
   \class SymbolInformation
   \brief Provides name, type and icon for single item in Class View tree
 */

class SymbolInformation
{
public:
    SymbolInformation();
    SymbolInformation(const QString &name, const QString &type, int iconType = INT_MIN);

    bool operator<(const SymbolInformation &other) const;

    inline const QString &name() const { return m_name; }
    inline const QString &type() const { return m_type; }
    inline int iconType() const { return m_iconType; }
    inline uint hash() const { return m_hash; }
    inline bool operator==(const SymbolInformation &other) const
    {
        return iconType() == other.iconType() && name() == other.name()
            && type() == other.type();
    }

    /*!
       \brief Get an icon type sort order. Not pre-calculated - is needed for converting
              to Standard Item only.
       \return Sort order number.
     */
    int iconTypeSortOrder() const;

private:
    int m_iconType; //!< icon type
    QString m_name; //!< symbol name (e.g. SymbolInformation)
    QString m_type; //!< symbol type (e.g. (int char))

    uint m_hash;    //!< precalculated hash value - to speed up qHash
};

//! qHash overload for QHash/QSet
inline uint qHash(const SymbolInformation &information)
{
    return information.hash();
}


} // namespace Internal
} // namespace ClassView

Q_DECLARE_METATYPE(ClassView::Internal::SymbolInformation)

#endif // CLASSVIEWSYMBOLINFORMATION_H
