/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "typeregistry.h"
#include "parameters.h"

#include <QString>

namespace qark {

template<class BASE, class DERIVED>
class Base
{
public:
    Base(const QString &qualifiedName, DERIVED &obj)
        : m_qualifiedName(qualifiedName),
          m_base(obj)
    {
    }

    Base(const QString &qualifiedName, DERIVED &obj, const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_base(obj),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    const BASE &base() const { return m_base; }
    BASE &base() { return m_base; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    BASE &m_base;
    Parameters m_parameters;
};

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(const QString &qualifiedName, DERIVED &obj)
{
    return Base<BASE, DERIVED>(qualifiedName, obj);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(const QString &qualifiedName, DERIVED &obj, const Parameters &parameters)
{
    return Base<BASE, DERIVED>(qualifiedName, obj, parameters);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(const QString &qualifiedName, DERIVED *&obj)
{
    return Base<BASE, DERIVED>(qualifiedName, *obj);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(const QString &qualifiedName, DERIVED *&obj, const Parameters &parameters)
{
    return Base<BASE, DERIVED>(qualifiedName, *obj, parameters);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(DERIVED &obj)
{
    return Base<BASE, DERIVED>(QString(QStringLiteral("base-%1")).arg(typeUid<BASE>()), obj);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(DERIVED &obj, const Parameters &parameters)
{
    return Base<BASE, DERIVED>(QString(QStringLiteral("base-%1")).arg(typeUid<BASE>()),
                               obj, parameters);
}

} // namespace qark
