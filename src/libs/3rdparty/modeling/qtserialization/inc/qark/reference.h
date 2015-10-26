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
    Ref(const QString &qualified_name, T *value)
        : _qualified_name(qualified_name),
          _value(value)
    {
    }

    Ref(const QString &qualified_name, T *value, const Parameters &parameters)
        : _qualified_name(qualified_name),
          _value(value),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    T *getValue() const { return _value; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    T *_value;
    Parameters _parameters;
};

template<typename T>
Ref<T * const> ref(const QString &qualified_name, T * const &value)
{
    return Ref<T * const>(qualified_name, &value);
}

template<typename T>
Ref<T * const> ref(const QString &qualified_name, T * const &value, const Parameters &parameters)
{
    return Ref<T * const>(qualified_name, &value, parameters);
}

template<typename T>
Ref<T *> ref(const QString &qualified_name, T *&value)
{
    return Ref<T *>(qualified_name, &value);
}

template<typename T>
Ref<T *> ref(const QString &qualified_name, T *&value, const Parameters &parameters)
{
    return Ref<T *>(qualified_name, &value, parameters);
}


template<class U, typename T>
class GetterRef {
public:
    GetterRef(const QString &qualified_name, const U &u, T (U::*getter)() const)
        : _qualified_name(qualified_name),
          _u(u),
          _getter(getter)
    {
    }

    GetterRef(const QString &qualified_name, const U &u, T (U::*getter)() const, const Parameters &parameters)
        : _qualified_name(qualified_name),
          _u(u),
          _getter(getter),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    const U &getObject() const { return _u; }

    T (U::*getGetter() const)() const { return _getter; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    const U &_u;
    T (U::*_getter)() const;
    Parameters _parameters;
};

template<class U, typename T>
GetterRef<U, T *> ref(const QString &qualified_name, const U &u, T *(U::*getter)() const)
{
    return GetterRef<U, T *>(qualified_name, u, getter);
}

template<class U, typename T>
GetterRef<U, T *> ref(const QString &qualified_name, const U &u, T *(U::*getter)() const, const Parameters &parameters)
{
    return GetterRef<U, T *>(qualified_name, u, getter, parameters);
}


template<class U, typename T>
class SetterRef {
public:
    SetterRef(const QString &qualified_name, U &u, void (U::*setter)(T))
        : _qualified_name(qualified_name),
          _u(u),
          _setter(setter)
    {
    }

    SetterRef(const QString &qualified_name, U &u, void (U::*setter)(T), const Parameters &parameters)
        : _qualified_name(qualified_name),
          _u(u),
          _setter(setter),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    U &getObject() const { return _u; }

    void (U::*getSetter() const)(T) { return _setter; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    U &_u;
    void (U::*_setter)(T);
    Parameters _parameters;
};

template<class U, class T>
SetterRef<U, T *> ref(const QString &qualified_name, U &u, void (U::*setter)(T *))
{
    return SetterRef<U, T *>(qualified_name, u, setter);
}

template<class U, class T>
SetterRef<U, T *> ref(const QString &qualified_name, U &u, void (U::*setter)(T *), const Parameters &parameters)
{
    return SetterRef<U, T *>(qualified_name, u, setter, parameters);
}

template<class U, class T>
SetterRef<U, T * const &> ref(const QString &qualified_name, U &u, void (U::*setter)(T * const &))
{
    return SetterRef<U, T * const &>(qualified_name, u, setter);
}

template<class U, class T>
SetterRef<U, T * const &> ref(const QString &qualified_name, U &u, void (U::*setter)(T * const &), const Parameters &parameters)
{
    return SetterRef<U, T * const &>(qualified_name, u, setter, parameters);
}


template<class U, typename T, typename V>
class GetterSetterRef {
public:
    GetterSetterRef(const QString &qualified_name, U &u, T (U::*getter)() const, void (U::*setter)(V))
        : _qualified_name(qualified_name),
          _u(u),
          _getter(getter),
          _setter(setter)
    {
    }

    GetterSetterRef(const QString &qualified_name, U &u, T (U::*getter)() const, void (U::*setter)(V), const Parameters &parameters)
        : _qualified_name(qualified_name),
          _u(u),
          _getter(getter),
          _setter(setter),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    U &getObject() const { return _u; }

    T (U::*getGetter() const)() const { return _getter; }

    void (U::*getSetter() const)(V) { return _setter; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    U &_u;
    T (U::*_getter)() const;
    void (U::*_setter)(V);
    Parameters _parameters;
};

template<class  U, typename T, typename V>
GetterSetterRef<U, T *, V *> ref(const QString &qualified_name, U &u, T *(U::*getter)() const, void (U::*setter)(V *))
{
    return GetterSetterRef<U, T *, V *>(qualified_name, u, getter, setter);
}

template<class  U, typename T, typename V>
GetterSetterRef<U, T *, V *> ref(const QString &qualified_name, U &u, T *(U::*getter)() const, void (U::*setter)(V *), const Parameters &parameters)
{
    return GetterSetterRef<U, T *, V *>(qualified_name, u, getter, setter, parameters);
}

template<class  U, typename T, typename V>
GetterSetterRef<U, T *, V * const &> ref(const QString &qualified_name, U &u, T *(U::*getter)() const, void (U::*setter)(V * const &))
{
    return GetterSetterRef<U, T *, V * const &>(qualified_name, u, getter, setter);
}

template<class  U, typename T, typename V>
GetterSetterRef<U, T *, V * const &> ref(const QString &qualified_name, U &u, T *(U::*getter)() const, void (U::*setter)(V * const &), const Parameters &parameters)
{
    return GetterSetterRef<U, T *, V * const &>(qualified_name, u, getter, setter, parameters);
}


template<class U, typename T>
class GetFuncRef {
public:
    GetFuncRef(const QString &qualified_name, const U &u, T (*get_func)(const U &))
        : _qualified_name(qualified_name),
          _u(u),
          _get_func(get_func)
    {
    }

    GetFuncRef(const QString &qualified_name, const U &u, T (*get_func)(const U &), const Parameters &parameters)
        : _qualified_name(qualified_name),
          _u(u),
          _get_func(get_func),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    const U &getObject() const { return _u; }

    T (*getGetFunc() const)(const U &) { return _get_func; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    const U &_u;
    T (*_get_func)(const U &);
    Parameters _parameters;
};

template<class U, typename T>
GetFuncRef<U, T *> ref(const QString &qualified_name, const U &u, T *(*get_func)(const U &))
{
    return GetFuncRef<U, T *>(qualified_name, u, get_func);
}

template<class U, typename T>
GetFuncRef<U, T *> ref(const QString &qualified_name, const U &u, T *(*get_func)(const U &), const Parameters &parameters)
{
    return GetFuncRef<U, T *>(qualified_name, u, get_func, parameters);
}


template<class U, typename T>
class SetFuncRef {
public:
    SetFuncRef(const QString &qualified_name, U &u, void (*set_func)(U &, T))
        : _qualified_name(qualified_name),
          _u(u),
          _set_func(set_func)
    {
    }

    SetFuncRef(const QString &qualified_name, U &u, void (*set_func)(U &, T), const Parameters &parameters)
        : _qualified_name(qualified_name),
          _u(u),
          _set_func(set_func),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    U &getObject() const { return _u; }

    void (*getSetFunc() const)(U &, T) { return _set_func; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    U &_u;
    void (*_set_func)(U &, T);
    Parameters _parameters;
};

template<class U, class T>
SetFuncRef<U, T *> ref(const QString &qualified_name, U &u, void (*set_func)(U &, T *))
{
    return SetFuncRef<U, T *>(qualified_name, u, set_func);
}

template<class U, class T>
SetFuncRef<U, T *> ref(const QString &qualified_name, U &u, void (*set_func)(U &, T *), const Parameters &parameters)
{
    return SetFuncRef<U, T *>(qualified_name, u, set_func, parameters);
}

template<class U, class T>
SetFuncRef<U, T * const &> ref(const QString &qualified_name, U &u, void (*set_func)(U &, T * const &))
{
    return SetFuncRef<U, T * const &>(qualified_name, u, set_func);
}

template<class U, class T>
SetFuncRef<U, T * const &> ref(const QString &qualified_name, U &u, void (*set_func)(U &, T * const &), const Parameters &parameters)
{
    return SetFuncRef<U, T * const &>(qualified_name, u, set_func, parameters);
}


template<class U, typename T, typename V>
class GetSetFuncRef {
public:
    GetSetFuncRef(const QString &qualified_name, U &u, T (*get_func)(const U &), void (*set_func)(U &, V))
        : _qualified_name(qualified_name),
          _u(u),
          _get_func(get_func),
          _set_func(set_func)
    {
    }

    GetSetFuncRef(const QString &qualified_name, U &u, T (*get_func)(const U &), void (*set_func)(U &, V), const Parameters &parameters)
        : _qualified_name(qualified_name),
          _u(u),
          _get_func(get_func),
          _set_func(set_func),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    U &getObject() const { return _u; }

    T (*getGetFunc() const)(const U &) { return _get_func; }

    void (*getSetFunc() const)(U &, V) { return _set_func; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    U &_u;
    T (*_get_func)(const U &);
    void (*_set_func)(U &, V);
    Parameters _parameters;
};

template<class  U, typename T, typename V>
GetSetFuncRef<U, T *, V *> ref(const QString &qualified_name, U &u, T *(*get_func)(const U &), void (*set_func)(U &, V *))
{
    return GetSetFuncRef<U, T *, V *>(qualified_name, u, get_func, set_func);
}

template<class  U, typename T, typename V>
GetSetFuncRef<U, T *, V *> ref(const QString &qualified_name, U &u, T *(*get_func)(const U &), void (*set_func)(U &, V *), const Parameters &parameters)
{
    return GetSetFuncRef<U, T *, V *>(qualified_name, u, get_func, set_func, parameters);
}

template<class  U, typename T, typename V>
GetSetFuncRef<U, T *, V * const &> ref(const QString &qualified_name, U &u, T *(*get_func)(const U &), void (*set_func)(U &, V * const &))
{
    return GetSetFuncRef<U, T *, V * const &>(qualified_name, u, get_func, set_func);
}

template<class  U, typename T, typename V>
GetSetFuncRef<U, T *, V * const &> ref(const QString &qualified_name, U &u, T *(*get_func)(const U &), void (*set_func)(U &, V * const &), const Parameters &parameters)
{
    return GetSetFuncRef<U, T *, V * const &>(qualified_name, u, get_func, set_func, parameters);
}

}

#endif // QARK_REFERENCE_H
