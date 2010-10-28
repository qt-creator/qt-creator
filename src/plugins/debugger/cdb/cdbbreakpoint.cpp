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
#include "cdbmodules.h"

#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

enum { debugBP = 0 };

// Convert breakpoint structs
CdbCore::BreakPoint breakPointFromBreakPointData(const BreakpointData &bpd)
{
    CdbCore::BreakPoint rc;
    rc.type = bpd.type == Watchpoint ?
                  CdbCore::BreakPoint::Data :
                  CdbCore::BreakPoint::Code ;

    rc.address = bpd.address;
    if (!bpd.threadSpec.isEmpty()) {
        bool ok;
        rc.threadId = bpd.threadSpec.toInt(&ok);
        if (!ok)
            qWarning("Cdb: Cannot convert breakpoint thread specification '%s'", bpd.threadSpec.constData());
    }
    rc.fileName = QDir::toNativeSeparators(bpd.fileName);
    rc.condition = bpd.condition;
    // Resolved function goes to bpd.bpFuncName.
    rc.funcName = bpd.bpFuncName.isEmpty() ? bpd.funcName : bpd.bpFuncName;
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

static bool addBreakpoint(CIDebugControl* debugControl,
                          CIDebugSymbols *syms,
                          BreakpointData *nbd,
                          QString *warning)
{
    warning->clear();
    // Function breakpoints: Are the module names specified?
    if (!nbd->funcName.isEmpty()) {
        nbd->bpFuncName = nbd->funcName;
        switch (resolveSymbol(syms, &nbd->bpFuncName, warning)) {
        case ResolveSymbolOk:
            break;
        case ResolveSymbolAmbiguous:
            *warning = msgCannotSetBreakAtFunction(nbd->funcName, *warning);
            break;
        case ResolveSymbolNotFound:
        case ResolveSymbolError:
            *warning = msgCannotSetBreakAtFunction(nbd->funcName, *warning);
            return false;
        };
    } // function breakpoint
    // Now add...
    quint64 address;
    unsigned long id;
    const CdbCore::BreakPoint ncdbbp = breakPointFromBreakPointData(*nbd);
    if (!ncdbbp.add(debugControl, warning, &id, &address))
        return false;
    if (debugBP)
        qDebug("Added %lu at 0x%lx %s", id, address, qPrintable(ncdbbp.toString()));
    nbd->pending = false;
    nbd->bpNumber = QByteArray::number(uint(id));
    nbd->bpAddress = address;
    // Take over rest as is
    nbd->bpCondition = nbd->condition;
    nbd->bpIgnoreCount = nbd->ignoreCount;
    nbd->bpThreadSpec = nbd->threadSpec;
    nbd->bpFileName = nbd->fileName;
    nbd->bpLineNumber = nbd->lineNumber;
    return true;
}

// Synchronize (halted) engine breakpoints with those of the BreakHandler.
bool synchronizeBreakPoints(CIDebugControl* debugControl,
                            CIDebugSymbols *syms,
                            BreakHandler *handler,
                            QString *errorMessage,
                            QStringList *warnings)
{
    errorMessage->clear();
    warnings->clear();
    // Do an initial check whether we are in a state that allows
    // for modifying  breakPoints
    ULONG engineCount;
    if (!CdbCore::BreakPoint::getBreakPointCount(debugControl, &engineCount, errorMessage)) {
        *errorMessage = QString::fromLatin1("Cannot modify breakpoints: %1").arg(*errorMessage);
        return false;
    }
    // Delete all breakpoints and re-insert all enabled breakpoints. This is the simplest
    // way to apply changes since CDB ids shift when removing breakpoints and there is no
    // easy way to re-match them.
    if (engineCount) {
        for (int b = engineCount - 1; b >= 0 ; b--)
            if (!CdbCore::BreakPoint::removeBreakPointById(debugControl, b, errorMessage))
                return false;
    }
    qDeleteAll(handler->takeRemovedBreakpoints());
    // Mark disabled ones
    foreach(BreakpointData *dbd, handler->takeDisabledBreakpoints())
        dbd->bpEnabled = false;

    // Insert all enabled ones as new
    QString warning;
    bool updateMarkers = false;
    const int size = handler->size();
    for (int i = 0; i < size; i++) {
        BreakpointData *breakpoint = handler->at(i);
        if (breakpoint->enabled)
            if (addBreakpoint(debugControl, syms, breakpoint, &warning)) {
                updateMarkers = true;
            } else {
                warnings->push_back(warning);
            }
    }
    // Mark enabled ones
    foreach(BreakpointData *ebd, handler->takeEnabledBreakpoints())
        ebd->bpEnabled = true;

    if (updateMarkers)
        handler->updateMarkers();

    if (debugBP > 1) {
        QList<CdbCore::BreakPoint> bps;
        CdbCore::BreakPoint::getBreakPoints(debugControl, &bps, errorMessage);
        QDebug nsp = qDebug().nospace();
        const int count = bps.size();
        nsp <<"### Breakpoints in engine: " << count << '\n';
        for (int i = 0; i < count; i++)
            nsp << "  #" << i << ' ' << bps.at(i) << '\n';
    }
    return true;
}

} // namespace Internal
} // namespace Debugger
