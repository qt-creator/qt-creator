/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "invoker.h"

namespace ExtensionSystem {

InvokerBase::InvokerBase()
{
    lastArg = 0;
    useRet = false;
    nag = true;
    success = true;
    connectionType = Qt::AutoConnection;
    target = 0;
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
