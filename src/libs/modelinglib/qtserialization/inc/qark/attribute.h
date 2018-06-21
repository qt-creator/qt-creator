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

#include "parameters.h"

#include <QString>

namespace qark {

template<typename T>
class Attr
{
public:
    Attr(const QString &qualifiedName, T *value)
        : m_qualifiedName(qualifiedName),
          m_value(value)
    {
    }

    Attr(const QString &qualifiedName, T *value, const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_value(value),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    T *value() const { return m_value; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    T *m_value = nullptr;
    Parameters m_parameters;
};

template<typename T>
Attr<T * const> attr(const QString &qualifiedName, T * const &value)
{
    return Attr<T * const>(qualifiedName, &value);
}

template<typename T>
Attr<T * const> attr(const QString &qualifiedName, T * const &value, const Parameters &parameters)
{
    return Attr<T * const>(qualifiedName, &value, parameters);
}

template<typename T>
Attr<T> attr(const QString &qualifiedName, T &value)
{
    return Attr<T>(qualifiedName, &value);
}

template<typename T>
Attr<T> attr(const QString &qualifiedName, T &value, const Parameters &parameters)
{
    return Attr<T>(qualifiedName, &value, parameters);
}

template<class U, typename T>
class GetterAttr
{
public:
    GetterAttr(const QString &qualifiedName, const U &u, T (U::*getter)() const)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getter(getter)
    {
    }

    GetterAttr(const QString &qualifiedName, const U &u, T (U::*getter)() const,
               const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getter(getter),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    const U &object() const { return m_u; }
    T (U::*getter() const)() const { return m_getter; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    const U &m_u;
    T (U::*m_getter)() const = nullptr;
    Parameters m_parameters;
};

template<class U, typename T>
GetterAttr<U, T> attr(const QString &qualifiedName, const U &u, T (U::*getter)() const)
{
    return GetterAttr<U, T>(qualifiedName, u, getter);
}

template<class U, typename T>
GetterAttr<U, T> attr(const QString &qualifiedName, const U &u, T (U::*getter)() const,
                      const Parameters &parameters)
{
    return GetterAttr<U, T>(qualifiedName, u, getter, parameters);
}

template<class U, typename T>
class SetterAttr
{
public:
    SetterAttr(const QString &qualifiedName, U &u, void (U::*setter)(T))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_setter(setter)
    {
    }

    SetterAttr(const QString &qualifiedName, U &u, void (U::*setter)(T),
               const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_setter(setter),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    U &object() const { return m_u; }
    void (U::*setter() const)(T) { return m_setter; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    void (U::*m_setter)(T) = nullptr;
    Parameters m_parameters;
};

template<class U, typename T>
SetterAttr<U, T> attr(const QString &qualifiedName, U &u, void (U::*setter)(T))
{
    return SetterAttr<U, T>(qualifiedName, u, setter);
}

template<class U, typename T>
SetterAttr<U, T> attr(const QString &qualifiedName, U &u, void (U::*setter)(T),
                      const Parameters &parameters)
{
    return SetterAttr<U, T>(qualifiedName, u, setter, parameters);
}

template<class U, typename T, typename V>
class GetterSetterAttr
{
public:
    GetterSetterAttr(const QString &qualifiedName, U &u, T (U::*getter)() const,
                     void (U::*setter)(V))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getter(getter),
          m_setter(setter)
    {
    }

    GetterSetterAttr(const QString &qualifiedName, U &u, T (U::*getter)() const,
                     void (U::*setter)(V),
                     const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getter(getter),
          m_setter(setter),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    U &object() const { return m_u; }
    T (U::*getter() const)() const { return m_getter; }
    void (U::*setter() const)(V) { return m_setter; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    T (U::*m_getter)() const = nullptr;
    void (U::*m_setter)(V) = nullptr;
    Parameters m_parameters;
};

template<class U, typename T, typename V>
GetterSetterAttr<U, T, V> attr(const QString &qualifiedName, U &u, T (U::*getter)() const,
                               void (U::*setter)(V))
{
    return GetterSetterAttr<U, T, V>(qualifiedName, u, getter, setter);
}

template<class U, typename T, typename V>
GetterSetterAttr<U, T, V> attr(const QString &qualifiedName, U &u, T (U::*getter)() const,
                               void (U::*setter)(V),
                               const Parameters &parameters)
{
    return GetterSetterAttr<U, T, V>(qualifiedName, u, getter, setter, parameters);
}

template<class U, typename T>
class GetFuncAttr
{
public:
    GetFuncAttr(const QString &qualifiedName, U &u, T (*func)(const U &))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getFunc(func)
    {
    }

    GetFuncAttr(const QString &qualifiedName, U &u, T (*func)(const U &),
                const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getFunc(func),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    U &object() const { return m_u; }
    T (*getterFunc() const)(const U &) { return m_getFunc; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    T (*m_getFunc)(const U &) = nullptr;
    Parameters m_parameters;
};

template<class U, typename T>
GetFuncAttr<U, T> attr(const QString &qualifiedName, const U &u, T (*func)(const U &))
{
    return GetFuncAttr<U, T>(qualifiedName, u, func);
}

template<class U, typename T>
GetFuncAttr<U, T> attr(const QString &qualifiedName, const U &u, T (*func)(const U &),
                       const Parameters &parameters)
{
    return GetFuncAttr<U, T>(qualifiedName, u, func, parameters);
}

template<class U, typename T>
class SetFuncAttr
{
public:
    SetFuncAttr(const QString &qualifiedName, U &u, void (*setFunc)(U &, T))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_setFunc(setFunc)
    {
    }

    SetFuncAttr(const QString &qualifiedName, U &u, void (*setFunc)(U &, T),
                const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_setFunc(setFunc),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    U &object() const { return m_u; }
    void (*setterFunc() const)(U &, T) { return m_setFunc; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    void (*m_setFunc)(U &, T) = nullptr;
    Parameters m_parameters;
};

template<class U, typename T>
SetFuncAttr<U, T> attr(const QString &qualifiedName, U &u, void (*setFunc)(U &, T))
{
    return SetFuncAttr<U, T>(qualifiedName, u, setFunc);
}

template<class U, typename T>
SetFuncAttr<U, T> attr(const QString &qualifiedName, U &u, void (*setFunc)(U &, T),
                       const Parameters &parameters)
{
    return SetFuncAttr<U, T>(qualifiedName, u, setFunc, parameters);
}

template<class U, typename T, typename V>
class GetSetFuncAttr
{
public:
    GetSetFuncAttr(const QString &qualifiedName, U &u, T (*func)(const U &),
                   void (*setFunc)(U &, V))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getFunc(func),
          m_setFunc(setFunc)
    {
    }

    GetSetFuncAttr(const QString &qualifiedName, U &u, T (*func)(const U &),
                   void (*setFunc)(U &, V), const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getFunc(func),
          m_setFunc(setFunc),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    U &object() const { return m_u; }
    T (*getterFunc() const)(const U &) { return m_getFunc; }
    void (*setterFunc() const)(U &, V) { return m_setFunc; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    T (*m_getFunc)(const U &) = nullptr;
    void (*m_setFunc)(U &, V) = nullptr;
    Parameters m_parameters;
};

template<class U, typename T, typename V>
GetSetFuncAttr<U, T, V> attr(const QString &qualifiedName, U &u, T (*func)(const U &),
                             void (*setFunc)(U &, V))
{
    return GetSetFuncAttr<U, T, V>(qualifiedName, u, func, setFunc);
}

template<class U, typename T, typename V>
GetSetFuncAttr<U, T, V> attr(const QString &qualifiedName, U &u, T (*func)(const U &),
                             void (*setFunc)(U &, V), const Parameters &parameters)
{
    return GetSetFuncAttr<U, T, V>(qualifiedName, u, func, setFunc, parameters);
}

} // namespace qark
