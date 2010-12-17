/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cdbbreakpoint.h"
#include "cdbengine_p.h"
#include "corebreakpoint.h"
#include "cdbmodules.h"
#include "breakhandler.h"
#include "shared/dbgwinutils.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>

namespace Debugger {
namespace Internal {

// Convert breakpoint structs
static CdbCore::BreakPoint breakPointFromBreakPointData(const BreakpointParameters &bpd, const QString &functionName)
{
    CdbCore::BreakPoint rc;
    rc.type = bpd.type == Watchpoint ?
                  CdbCore::BreakPoint::Data :
                  CdbCore::BreakPoint::Code ;

    rc.address = bpd.address;
    rc.threadId = bpd.threadSpec;
    rc.fileName = QDir::toNativeSeparators(bpd.fileName);
    rc.condition = bpd.condition;
    rc.funcName = functionName.isEmpty() ? bpd.functionName : functionName;
    rc.ignoreCount = bpd.ignoreCount;
    rc.lineNumber  = bpd.lineNumber;
    rc.oneShot = false;
    rc.enabled = bpd.enabled;
    return rc;
}

static inline QString msgCannotSetBreakAtFunction(const QString &func, const QString &why)
{
    return QString::fromLatin1("Cannot set a breakpoint at '%1': %2").arg(func, why);
}

bool addCdbBreakpoint(CIDebugControl* debugControl,
                      CIDebugSymbols *syms,
                      const BreakpointParameters &bpIn,
                      BreakpointResponse *response,
                      QString *errorMessage)
{
    const BreakpointParameters bp = fixWinMSVCBreakpoint(bpIn);
    errorMessage->clear();
    // Function breakpoints: Are the module names specified?
    QString resolvedFunction;
    if (bp.type == BreakpointByFunction) {
        resolvedFunction = bp.functionName;
        switch (resolveSymbol(syms, &resolvedFunction, errorMessage)) {
        case ResolveSymbolOk:
            break;
        case ResolveSymbolAmbiguous:
            break;
        case ResolveSymbolNotFound:
        case ResolveSymbolError:
            *errorMessage = msgCannotSetBreakAtFunction(bp.functionName, *errorMessage);
            return false;
        }
        if (debugBreakpoints)
            qDebug() << bp.functionName << " resolved to " << resolvedFunction;
    } // function breakpoint
    // Now add...
    quint64 address;
    unsigned long id;
    const CdbCore::BreakPoint ncdbbp = breakPointFromBreakPointData(bp, resolvedFunction);
    if (!ncdbbp.add(debugControl, errorMessage, &id, &address))
        return false;
    if (debugBreakpoints)
        qDebug("Added %lu at 0x%lx %s", id, address, qPrintable(ncdbbp.toString()));

    response->fromParameters(bp);
    response->number = id;
    response->address = address;
    response->functionName = resolvedFunction;
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
