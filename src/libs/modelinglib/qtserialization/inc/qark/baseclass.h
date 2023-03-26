// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    return Base<BASE, DERIVED>(QString("base-%1").arg(typeUid<BASE>()), obj);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(DERIVED &obj, const Parameters &parameters)
{
    return Base<BASE, DERIVED>(QString("base-%1").arg(typeUid<BASE>()),
                               obj, parameters);
}

} // namespace qark
