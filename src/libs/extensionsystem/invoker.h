/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef EXTENSIONSYSTEM_INVOKER_H
#define EXTENSIONSYSTEM_INVOKER_H

#include "extensionsystem_global.h"

#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaType>
#include <QVarLengthArray>

namespace ExtensionSystem {

class EXTENSIONSYSTEM_EXPORT InvokerBase
{
public:
    InvokerBase();
    ~InvokerBase();

    bool wasSuccessful() const;
    void setConnectionType(Qt::ConnectionType connectionType);

    template <class T> void addArgument(const T &t)
    {
        arg[lastArg++] = QGenericArgument(typeName<T>(), &t);
    }

    template <class T> void setReturnValue(T &t)
    {
        useRet = true;
        ret = QGenericReturnArgument(typeName<T>(), &t);
    }

    void invoke(QObject *target, const char *slot);

private:
    InvokerBase(const InvokerBase &); // Unimplemented.
    template <class T> const char *typeName()
    {
        return QMetaType::typeName(qMetaTypeId<T>());
    }
    QObject *target;
    QGenericArgument arg[10];
    QGenericReturnArgument ret;
    QVarLengthArray<char, 512> sig;
    int lastArg;
    bool success;
    bool useRet;
    Qt::ConnectionType connectionType;
    mutable bool nag;
};

template <class Result>
class Invoker : public InvokerBase
{
public:
    Invoker(QObject *target, const char *slot)
    {
        InvokerBase::invoke(target, slot);
    }

    template <class T0>
    Invoker(QObject *target, const char *slot, const T0 &t0)
    {
        setReturnValue(result);
        addArgument(t0);
        InvokerBase::invoke(target, slot);
    }

    template <class T0, class T1>
    Invoker(QObject *target, const char *slot, const T0 &t0, const T1 &t1)
    {
        setReturnValue(result);
        addArgument(t0);
        addArgument(t1);
        InvokerBase::invoke(target, slot);
    }

    template <class T0, class T1, class T2>
    Invoker(QObject *target, const char *slot, const T0 &t0,
        const T1 &t1, const T2 &t2)
    {
        setReturnValue(result);
        addArgument(t0);
        addArgument(t1);
        addArgument(t2);
        InvokerBase::invoke(target, slot);
    }

    operator Result() const { return result; }

private:
    Result result;
};

template<> class Invoker<void> : public InvokerBase
{
public:
    Invoker(QObject *target, const char *slot)
    {
        InvokerBase::invoke(target, slot);
    }

    template <class T0>
    Invoker(QObject *target, const char *slot, const T0 &t0)
    {
        addArgument(t0);
        InvokerBase::invoke(target, slot);
    }

    template <class T0, class T1>
    Invoker(QObject *target, const char *slot, const T0 &t0, const T1 &t1)
    {
        addArgument(t0);
        addArgument(t1);
        InvokerBase::invoke(target, slot);
    }

    template <class T0, class T1, class T2>
    Invoker(QObject *target, const char *slot, const T0 &t0,
        const T1 &t1, const T2 &t2)
    {
        addArgument(t0);
        addArgument(t1);
        addArgument(t2);
        InvokerBase::invoke(target, slot);
    }
};

template <class Result>
Result invokeHelper(InvokerBase &in, QObject *target, const char *slot)
{
    Result result;
    in.setReturnValue(result);
    in.invoke(target, slot);
    return result;
}

template <>
inline void invokeHelper<void>(InvokerBase &in, QObject *target, const char *slot)
{
    in.invoke(target, slot);
}

template<class Result>
Result invoke(QObject *target, const char *slot)
{
    InvokerBase in;
    return invokeHelper<Result>(in, target, slot);
}

template<class Result, class T0>
Result invoke(QObject *target, const char *slot, const T0 &t0)
{
    InvokerBase in;
    in.addArgument(t0);
    return invokeHelper<Result>(in, target, slot);
}

template<class Result, class T0, class T1>
Result invoke(QObject *target, const char *slot, const T0 &t0, const T1 &t1)
{
    InvokerBase in;
    in.addArgument(t0);
    in.addArgument(t1);
    return invokeHelper<Result>(in, target, slot);
}

template<class Result, class T0, class T1, class T2>
Result invoke(QObject *target, const char *slot,
    const T0 &t0, const T1 &t1, const T2 &t2)
{
    InvokerBase in;
    in.addArgument(t0);
    in.addArgument(t1);
    in.addArgument(t2);
    return invokeHelper<Result>(in, target, slot);
}

} // namespace ExtensionSystem

#endif // EXTENSIONSYSTEM_INVOKER_H
