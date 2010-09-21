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
CdbCore::BreakPoint breakPointFromBreakPointData(const Debugger::Internal::BreakpointData &bpd)
{
    CdbCore::BreakPoint rc;
    rc.type = bpd.type == Debugger::Internal::BreakpointData::BreakpointType ?
              CdbCore::BreakPoint::Code : CdbCore::BreakPoint::Data;

    rc.address = bpd.address;
    if (!bpd.threadSpec.isEmpty()) {
        bool ok;
        rc.threadId = bpd.threadSpec.toInt(&ok);
        if (!ok)
            qWarning("Cdb: Cannot convert breakpoint thread specification '%s'", bpd.threadSpec.constData());
    }
    rc.fileName = QDir::toNativeSeparators(bpd.fileName);
    rc.condition = bpd.condition;
    rc.funcName = bpd.funcName;
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
                    qDebug("Added %lu at 0x%lx %s", id, address, qPrintable(ncdbbp.toString()));
                handler->takeInsertedBreakPoint(nbd);
                updateMarkers = true;
                nbd->pending = false;
                nbd->bpNumber = QByteArray::number(uint(id));
                nbd->bpAddress = address;
                // Take over rest as is
                nbd->bpCondition = nbd->condition;
                nbd->bpIgnoreCount = nbd->ignoreCount;
                nbd->bpThreadSpec = nbd->threadSpec;
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
    // Check for modified thread ids.
    for (int i = handler->size() - 1; i >= 0; i--) {
        BreakpointData *bpd = handler->at(i);
        if (bpd->threadSpec != bpd->bpThreadSpec) {
            const int newThreadSpec = bpd->threadSpec.isEmpty() ? -1 : bpd->threadSpec.toInt();
            if (CdbCore::BreakPoint::setBreakPointThreadById(debugControl, bpd->bpNumber.toUInt(), newThreadSpec, errorMessage)) {
                bpd->bpThreadSpec = bpd->threadSpec;
            } else {
                qWarning("%s", qPrintable(*errorMessage));
            }
        }
    }

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
