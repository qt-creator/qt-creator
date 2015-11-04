/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QARK_REFERENCE_H
#define QARK_REFERENCE_H

#include "parameters.h"

#include <QString>

namespace qark {

template<typename T>
class Ref {
public:
    Ref(const QString &qualifiedName, T *value)
        : m_qualifiedName(qualifiedName),
          m_value(value)
    {
    }

    Ref(const QString &qualifiedName, T *value, const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_value(value),
          m_parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return m_qualifiedName; }

    T *getValue() const { return m_value; }

    Parameters getParameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    T *m_value;
    Parameters m_parameters;
};

template<typename T>
Ref<T * const> ref(const QString &qualifiedName, T * const &value)
{
    return Ref<T * const>(qualifiedName, &value);
}

template<typename T>
Ref<T * const> ref(const QString &qualifiedName, T * const &value, const Parameters &parameters)
{
    return Ref<T * const>(qualifiedName, &value, parameters);
}

template<typename T>
Ref<T *> ref(const QString &qualifiedName, T *&value)
{
    return Ref<T *>(qualifiedName, &value);
}

template<typename T>
Ref<T *> ref(const QString &qualifiedName, T *&value, const Parameters &parameters)
{
    return Ref<T *>(qualifiedName, &value, parameters);
}


template<class U, typename T>
class GetterRef {
public:
    GetterRef(const QString &qualifiedName, const U &u, T (U::*getter)() const)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getter(getter)
    {
    }

    GetterRef(const QString &qualifiedName, const U &u, T (U::*getter)() const, const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getter(getter),
          m_parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return m_qualifiedName; }

    const U &getObject() const { return m_u; }

    T (U::*getGetter() const)() const { return m_getter; }

    Parameters getParameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    const U &m_u;
    T (U::*m_getter)() const;
    Parameters m_parameters;
};

template<class U, typename T>
GetterRef<U, T *> ref(const QString &qualifiedName, const U &u, T *(U::*getter)() const)
{
    return GetterRef<U, T *>(qualifiedName, u, getter);
}

template<class U, typename T>
GetterRef<U, T *> ref(const QString &qualifiedName, const U &u, T *(U::*getter)() const, const Parameters &parameters)
{
    return GetterRef<U, T *>(qualifiedName, u, getter, parameters);
}


template<class U, typename T>
class SetterRef {
public:
    SetterRef(const QString &qualifiedName, U &u, void (U::*setter)(T))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_setter(setter)
    {
    }

    SetterRef(const QString &qualifiedName, U &u, void (U::*setter)(T), const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_setter(setter),
          m_parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return m_qualifiedName; }

    U &getObject() const { return m_u; }

    void (U::*getSetter() const)(T) { return m_setter; }

    Parameters getParameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    void (U::*m_setter)(T);
    Parameters m_parameters;
};

template<class U, class T>
SetterRef<U, T *> ref(const QString &qualifiedName, U &u, void (U::*setter)(T *))
{
    return SetterRef<U, T *>(qualifiedName, u, setter);
}

template<class U, class T>
SetterRef<U, T *> ref(const QString &qualifiedName, U &u, void (U::*setter)(T *), const Parameters &parameters)
{
    return SetterRef<U, T *>(qualifiedName, u, setter, parameters);
}

template<class U, class T>
SetterRef<U, T * const &> ref(const QString &qualifiedName, U &u, void (U::*setter)(T * const &))
{
    return SetterRef<U, T * const &>(qualifiedName, u, setter);
}

template<class U, class T>
SetterRef<U, T * const &> ref(const QString &qualifiedName, U &u, void (U::*setter)(T * const &), const Parameters &parameters)
{
    return SetterRef<U, T * const &>(qualifiedName, u, setter, parameters);
}


template<class U, typename T, typename V>
class GetterSetterRef {
public:
    GetterSetterRef(const QString &qualifiedName, U &u, T (U::*getter)() const, void (U::*setter)(V))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getter(getter),
          m_setter(setter)
    {
    }

    GetterSetterRef(const QString &qualifiedName, U &u, T (U::*getter)() const, void (U::*setter)(V), const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getter(getter),
          m_setter(setter),
          m_parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return m_qualifiedName; }

    U &getObject() const { return m_u; }

    T (U::*getGetter() const)() const { return m_getter; }

    void (U::*getSetter() const)(V) { return m_setter; }

    Parameters getParameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    T (U::*m_getter)() const;
    void (U::*m_setter)(V);
    Parameters m_parameters;
};

template<class  U, typename T, typename V>
GetterSetterRef<U, T *, V *> ref(const QString &qualifiedName, U &u, T *(U::*getter)() const, void (U::*setter)(V *))
{
    return GetterSetterRef<U, T *, V *>(qualifiedName, u, getter, setter);
}

template<class  U, typename T, typename V>
GetterSetterRef<U, T *, V *> ref(const QString &qualifiedName, U &u, T *(U::*getter)() const, void (U::*setter)(V *), const Parameters &parameters)
{
    return GetterSetterRef<U, T *, V *>(qualifiedName, u, getter, setter, parameters);
}

template<class  U, typename T, typename V>
GetterSetterRef<U, T *, V * const &> ref(const QString &qualifiedName, U &u, T *(U::*getter)() const, void (U::*setter)(V * const &))
{
    return GetterSetterRef<U, T *, V * const &>(qualifiedName, u, getter, setter);
}

template<class  U, typename T, typename V>
GetterSetterRef<U, T *, V * const &> ref(const QString &qualifiedName, U &u, T *(U::*getter)() const, void (U::*setter)(V * const &), const Parameters &parameters)
{
    return GetterSetterRef<U, T *, V * const &>(qualifiedName, u, getter, setter, parameters);
}


template<class U, typename T>
class GetFuncRef {
public:
    GetFuncRef(const QString &qualifiedName, const U &u, T (*getFunc)(const U &))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getFunc(getFunc)
    {
    }

    GetFuncRef(const QString &qualifiedName, const U &u, T (*getFunc)(const U &), const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getFunc(getFunc),
          m_parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return m_qualifiedName; }

    const U &getObject() const { return m_u; }

    T (*getGetFunc() const)(const U &) { return m_getFunc; }

    Parameters getParameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    const U &m_u;
    T (*m_getFunc)(const U &);
    Parameters m_parameters;
};

template<class U, typename T>
GetFuncRef<U, T *> ref(const QString &qualifiedName, const U &u, T *(*getFunc)(const U &))
{
    return GetFuncRef<U, T *>(qualifiedName, u, getFunc);
}

template<class U, typename T>
GetFuncRef<U, T *> ref(const QString &qualifiedName, const U &u, T *(*getFunc)(const U &), const Parameters &parameters)
{
    return GetFuncRef<U, T *>(qualifiedName, u, getFunc, parameters);
}


template<class U, typename T>
class SetFuncRef {
public:
    SetFuncRef(const QString &qualifiedName, U &u, void (*setFunc)(U &, T))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_setFunc(setFunc)
    {
    }

    SetFuncRef(const QString &qualifiedName, U &u, void (*setFunc)(U &, T), const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_setFunc(setFunc),
          m_parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return m_qualifiedName; }

    U &getObject() const { return m_u; }

    void (*getSetFunc() const)(U &, T) { return m_setFunc; }

    Parameters getParameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    void (*m_setFunc)(U &, T);
    Parameters m_parameters;
};

template<class U, class T>
SetFuncRef<U, T *> ref(const QString &qualifiedName, U &u, void (*setFunc)(U &, T *))
{
    return SetFuncRef<U, T *>(qualifiedName, u, setFunc);
}

template<class U, class T>
SetFuncRef<U, T *> ref(const QString &qualifiedName, U &u, void (*setFunc)(U &, T *), const Parameters &parameters)
{
    return SetFuncRef<U, T *>(qualifiedName, u, setFunc, parameters);
}

template<class U, class T>
SetFuncRef<U, T * const &> ref(const QString &qualifiedName, U &u, void (*setFunc)(U &, T * const &))
{
    return SetFuncRef<U, T * const &>(qualifiedName, u, setFunc);
}

template<class U, class T>
SetFuncRef<U, T * const &> ref(const QString &qualifiedName, U &u, void (*setFunc)(U &, T * const &), const Parameters &parameters)
{
    return SetFuncRef<U, T * const &>(qualifiedName, u, setFunc, parameters);
}


template<class U, typename T, typename V>
class GetSetFuncRef {
public:
    GetSetFuncRef(const QString &qualifiedName, U &u, T (*getFunc)(const U &), void (*setFunc)(U &, V))
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getFunc(getFunc),
          m_setFunc(setFunc)
    {
    }

    GetSetFuncRef(const QString &qualifiedName, U &u, T (*getFunc)(const U &), void (*setFunc)(U &, V), const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_u(u),
          m_getFunc(getFunc),
          m_setFunc(setFunc),
          m_parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return m_qualifiedName; }

    U &getObject() const { return m_u; }

    T (*getGetFunc() const)(const U &) { return m_getFunc; }

    void (*getSetFunc() const)(U &, V) { return m_setFunc; }

    Parameters getParameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    U &m_u;
    T (*m_getFunc)(const U &);
    void (*m_setFunc)(U &, V);
    Parameters m_parameters;
};

template<class  U, typename T, typename V>
GetSetFuncRef<U, T *, V *> ref(const QString &qualifiedName, U &u, T *(*getFunc)(const U &), void (*setFunc)(U &, V *))
{
    return GetSetFuncRef<U, T *, V *>(qualifiedName, u, getFunc, setFunc);
}

template<class  U, typename T, typename V>
GetSetFuncRef<U, T *, V *> ref(const QString &qualifiedName, U &u, T *(*getFunc)(const U &), void (*setFunc)(U &, V *), const Parameters &parameters)
{
    return GetSetFuncRef<U, T *, V *>(qualifiedName, u, getFunc, setFunc, parameters);
}

template<class  U, typename T, typename V>
GetSetFuncRef<U, T *, V * const &> ref(const QString &qualifiedName, U &u, T *(*getFunc)(const U &), void (*setFunc)(U &, V * const &))
{
    return GetSetFuncRef<U, T *, V * const &>(qualifiedName, u, getFunc, setFunc);
}

template<class  U, typename T, typename V>
GetSetFuncRef<U, T *, V * const &> ref(const QString &qualifiedName, U &u, T *(*getFunc)(const U &), void (*setFunc)(U &, V * const &), const Parameters &parameters)
{
    return GetSetFuncRef<U, T *, V * const &>(qualifiedName, u, getFunc, setFunc, parameters);
}

}

#endif // QARK_REFERENCE_H
