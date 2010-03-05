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


namespace Debugger {
namespace Internal {

enum { debugBP = 0 };


static inline QString msgCannotSetBreakAtFunction(const QString &func, const QString &why)
{
    return QString::fromLatin1("Cannot set a breakpoint at '%1': %2").arg(func, why);
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
    QString warning;
    // Insert new ones
    bool updateMarkers = false;
    foreach (BreakpointData *nbd, handler->insertedBreakpoints()) {
        warning.clear();
        // Function breakpoints: Are the module names specified?
        bool breakPointOk = false;
        if (nbd->funcName.isEmpty()) {
            breakPointOk = true;
        } else {
            switch (resolveSymbol(syms, &nbd->funcName, &warning)) {
            case ResolveSymbolOk:
                breakPointOk = true;
                break;
            case ResolveSymbolAmbiguous:
                warnings->push_back(msgCannotSetBreakAtFunction(nbd->funcName, warning));
                warning.clear();
                breakPointOk = true;
                break;
            case ResolveSymbolNotFound:
            case ResolveSymbolError:
                warnings->push_back(msgCannotSetBreakAtFunction(nbd->funcName, warning));
                warning.clear();
                break;
            };
        } // function breakpoint
        // Now add...
        if (breakPointOk) {
            quint64 address;
            unsigned long id;
            const CdbCore::BreakPoint ncdbbp = breakPointFromBreakPointData(*nbd);
            breakPointOk = ncdbbp.add(debugControl, &warning, &id, &address);
            if (breakPointOk) {
                if (debugBP)
                    qDebug() << "Added " << id << " at " << address << ncdbbp;
                handler->takeInsertedBreakPoint(nbd);
                updateMarkers = true;
                nbd->pending = false;
                nbd->bpNumber = QByteArray::number(uint(id));
                nbd->bpAddress = "0x" + QByteArray::number(address, 16);
                // Take over rest as is
                nbd->bpCondition = nbd->condition;
                nbd->bpIgnoreCount = nbd->ignoreCount;
                nbd->bpFileName = nbd->fileName;
                nbd->bpLineNumber = nbd->lineNumber;
                nbd->bpFuncName = nbd->funcName;
            }
        } // had symbol
        if (!breakPointOk && !warning.isEmpty())
            warnings->push_back(warning);    }
    // Delete
    foreach (BreakpointData *rbd, handler->takeRemovedBreakpoints()) {
        if (!CdbCore::BreakPoint::removeBreakPointById(debugControl, rbd->bpNumber.toUInt(), &warning))
            warnings->push_back(warning);
        delete rbd;
    }
    // Enable/Disable
    foreach (BreakpointData *ebd, handler->takeEnabledBreakpoints())
        if (!CdbCore::BreakPoint::setBreakPointEnabledById(debugControl, ebd->bpNumber.toUInt(), true, &warning))
            warnings->push_back(warning);
    foreach (BreakpointData *dbd, handler->takeDisabledBreakpoints())
        if (!CdbCore::BreakPoint::setBreakPointEnabledById(debugControl, dbd->bpNumber.toUInt(), false, &warning))
            warnings->push_back(warning);

    if (updateMarkers)
        handler->updateMarkers();

    if (debugBP > 1) {
        QList<CdbCore::BreakPoint> bps;
        CdbCore::BreakPoint::getBreakPoints(debugControl, &bps, errorMessage);
        qDebug().nospace() << "### Breakpoints in engine: " << bps;
    }
    return true;
}

} // namespace Internal
} // namespace Debugger
