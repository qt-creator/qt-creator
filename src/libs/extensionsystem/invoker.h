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
        return QMetaType(qMetaTypeId<T>()).name();
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
    template <typename ...Args>
    Invoker(QObject *target, const char *slot, const Args &...args)
    {
        setReturnValue(result);
        (addArgument(args), ...);
        InvokerBase::invoke(target, slot);
    }

    operator Result() const { return result; }

private:
    Result result;
};

template<> class Invoker<void> : public InvokerBase
{
public:
    template <typename ...Args>
    Invoker(QObject *target, const char *slot, const Args &...args)
    {
        (addArgument(args), ...);
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

template<class Result, typename ...Args>
Result invoke(QObject *target, const char *slot, const Args &...args)
{
    InvokerBase in;
    (in.addArgument(args), ...);
    return invokeHelper<Result>(in, target, slot);
}

} // namespace ExtensionSystem
