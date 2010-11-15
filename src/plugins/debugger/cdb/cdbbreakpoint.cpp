/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cdbbreakpoint.h"
#include "cdbengine_p.h"
#include "corebreakpoint.h"
#include "cdbmodules.h"
#include "breakhandler.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>

namespace Debugger {
namespace Internal {

// Convert breakpoint structs
static CdbCore::BreakPoint breakPointFromBreakPointData(const BreakpointData &bpd, const QString &functionName)
{
    CdbCore::BreakPoint rc;
    rc.type = bpd.type() == Watchpoint ?
                  CdbCore::BreakPoint::Data :
                  CdbCore::BreakPoint::Code ;

    rc.address = bpd.address();
    if (!bpd.threadSpec().isEmpty()) {
        bool ok;
        rc.threadId = bpd.threadSpec().toInt(&ok);
        if (!ok)
            qWarning("Cdb: Cannot convert breakpoint thread specification '%s'", bpd.threadSpec().constData());
    }
    rc.fileName = QDir::toNativeSeparators(bpd.fileName());
    rc.condition = bpd.condition();
    rc.funcName = functionName.isEmpty() ? bpd.functionName() : functionName;
    rc.ignoreCount = bpd.ignoreCount();
    rc.lineNumber  = bpd.lineNumber();
    rc.oneShot = false;
    rc.enabled = bpd.isEnabled();
    return rc;
}

static inline QString msgCannotSetBreakAtFunction(const QString &func, const QString &why)
{
    return QString::fromLatin1("Cannot set a breakpoint at '%1': %2").arg(func, why);
}

void setBreakpointResponse(const BreakpointData *nbd, int number, BreakpointResponse *response)
{
    response->bpAddress = nbd->address();
    response->bpNumber = number;
    response->bpFuncName = nbd->functionName();
    response->bpType = nbd->type();
    response->bpCondition = nbd->condition();
    response->bpIgnoreCount = nbd->ignoreCount();
    response->bpFullName = response->bpFileName = nbd->fileName();
    response->bpLineNumber = nbd->lineNumber();
    response->bpThreadSpec = nbd->threadSpec();
    response->bpEnabled = nbd->isEnabled();
}

bool addCdbBreakpoint(CIDebugControl* debugControl,
                      CIDebugSymbols *syms,
                      const BreakpointData *nbd,
                      BreakpointResponse *response,
                      QString *errorMessage)
{
    errorMessage->clear();
    // Function breakpoints: Are the module names specified?
    QString resolvedFunction;
    if (nbd->type() == BreakpointByFunction) {
        resolvedFunction = nbd->functionName();
        switch (resolveSymbol(syms, &resolvedFunction, errorMessage)) {
        case ResolveSymbolOk:
            break;
        case ResolveSymbolAmbiguous:
            break;
        case ResolveSymbolNotFound:
        case ResolveSymbolError:
            *errorMessage = msgCannotSetBreakAtFunction(nbd->functionName(), *errorMessage);
            return false;
        }
        if (debugBreakpoints)
            qDebug() << nbd->functionName() << " resolved to " << resolvedFunction;
    } // function breakpoint
    // Now add...
    quint64 address;
    unsigned long id;
    const CdbCore::BreakPoint ncdbbp = breakPointFromBreakPointData(*nbd, resolvedFunction);
    if (!ncdbbp.add(debugControl, errorMessage, &id, &address))
        return false;
    if (debugBreakpoints)
        qDebug("Added %lu at 0x%lx %s", id, address, qPrintable(ncdbbp.toString()));
    setBreakpointResponse(nbd, id, response);
    response->bpAddress = address;
    response->bpFuncName = resolvedFunction;
    return true;
}

// Delete all breakpoints
bool deleteCdbBreakpoints(CIDebugControl* debugControl,
                       QString *errorMessage)
{
    errorMessage->clear();
    // Do an initial check whether we are in a state that allows
    // for modifying  breakPoints
    ULONG engineCount;
    if (!CdbCore::BreakPoint::getBreakPointCount(debugControl, &engineCount, errorMessage)) {
        *errorMessage = QString::fromLatin1("Cannot modify breakpoints: %1").arg(*errorMessage);
        return false;
    }
    if (debugBreakpoints)
        qDebug("Deleting breakpoints 0..%lu", engineCount);

    if (engineCount) {
        for (int b = engineCount - 1; b >= 0 ; b--)
            if (!CdbCore::BreakPoint::removeBreakPointById(debugControl, b, errorMessage))
                return false;
    }
    return true;
}

void debugCdbBreakpoints(CIDebugControl* debugControl)
{
    QString errorMessage;
    QList<CdbCore::BreakPoint> bps;
    CdbCore::BreakPoint::getBreakPoints(debugControl, &bps, &errorMessage);
    QDebug nsp = qDebug().nospace();
    const int count = bps.size();
    nsp <<"### Breakpoints in engine: " << count << '\n';
    for (int i = 0; i < count; i++)
        nsp << "  #" << i << ' ' << bps.at(i) << '\n';
}

} // namespace Internal
} // namespace Debugger
