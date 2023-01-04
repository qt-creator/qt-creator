// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invoker.h"

namespace ExtensionSystem {

/*!
    \class ExtensionSystem::InvokerBase
    \internal
*/

/*!
    \class ExtensionSystem::Invoker
    \internal
*/

/*!
    \fn template <class Result> Result ExtensionSystem::invoke(QObject *target, const char *slot)
    Invokes \a slot on \a target by name via Qt's meta method system.

    Returns the result of the meta call.
*/

/*!
    \fn template <class Result, class T0> Result ExtensionSystem::invoke(QObject *target, const char *slot, const T0 &t0)
    Invokes \a slot on \a target with argument \a t0 by name via Qt's meta method system.

    Returns the result of the meta call.
*/

/*!
    \fn template <class Result, class T0, class T1> Result ExtensionSystem::invoke(QObject *target, const char *slot, const T0 &t0, const T1 &t1)
    Invokes \a slot on \a target with arguments \a t0 and \a t1 by name via Qt's meta method system.

    Returns the result of the meta call.
*/

/*!
    \fn template <class Result, class T0, class T1, class T2> Result ExtensionSystem::invoke(QObject *target, const char *slot, const T0 &t0, const T1 &t1, const T2 &t2)
    Invokes \a slot on \a target with arguments \a t0, \a t1 and \a t2 by name
    via Qt's meta method system.

    Returns the result of the meta call.
*/

InvokerBase::InvokerBase()
{
    lastArg = 0;
    useRet = false;
    nag = true;
    success = true;
    connectionType = Qt::AutoConnection;
    target = nullptr;
}

InvokerBase::~InvokerBase()
{
    if (!success && nag)
        qWarning("Could not invoke function '%s' in object of type '%s'.",
            sig.constData(), target->metaObject()->className());
}

bool InvokerBase::wasSuccessful() const
{
    nag = false;
    return success;
}

void InvokerBase::setConnectionType(Qt::ConnectionType c)
{
    connectionType = c;
}

void InvokerBase::invoke(QObject *t, const char *slot)
{
    target = t;
    success = false;
    sig.append(slot, qstrlen(slot));
    sig.append('(');
    for (int paramCount = 0; paramCount < lastArg; ++paramCount) {
        if (paramCount)
            sig.append(',');
        const char *type = arg[paramCount].name();
        sig.append(type, int(strlen(type)));
    }
    sig.append(')');
    sig.append('\0');
    int idx = target->metaObject()->indexOfMethod(sig.constData());
    if (idx < 0)
        return;
    QMetaMethod method = target->metaObject()->method(idx);
    if (useRet)
        success = method.invoke(target, connectionType, ret,
           arg[0], arg[1], arg[2], arg[3], arg[4],
           arg[5], arg[6], arg[7], arg[8], arg[9]);
    else
        success = method.invoke(target, connectionType,
           arg[0], arg[1], arg[2], arg[3], arg[4],
           arg[5], arg[6], arg[7], arg[8], arg[9]);
}

} // namespace ExtensionSystem
