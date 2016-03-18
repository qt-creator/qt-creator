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

#include <limits.h>

namespace ClassView {
namespace Internal {

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

    int iconTypeSortOrder() const;

private:
    int m_iconType; //!< icon type
    uint m_hash;    //!< precalculated hash value - to speed up qHash
    QString m_name; //!< symbol name (e.g. SymbolInformation)
    QString m_type; //!< symbol type (e.g. (int char))

};

//! qHash overload for QHash/QSet
inline uint qHash(const SymbolInformation &information)
{
    return information.hash();
}


} // namespace Internal
} // namespace ClassView

Q_DECLARE_METATYPE(ClassView::Internal::SymbolInformation)
