// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

#ifndef Q_QDOC
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
#endif

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
