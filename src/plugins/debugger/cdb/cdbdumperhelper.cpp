/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdbdumperhelper.h"
#include "cdbmodules.h"
#include "cdbdebugengine_p.h"
#include "cdbdebugoutput.h"
#include "cdbdebugeventcallback.h"

#include <QtCore/QRegExp>

enum { loadDebug = 0 };

namespace Debugger {
namespace Internal {

// Alloc memory in debuggee using the ".dvalloc" command as
// there seems to be no API for it.
static bool allocDebuggeeMemory(CIDebugControl *ctl,
                                CIDebugClient *client,
                                int size, ULONG64 *address, QString *errorMessage)
{
    *address = 0;
    const QString allocCmd = QLatin1String(".dvalloc ") + QString::number(size);
    StringOutputHandler stringHandler;
    OutputRedirector redir(client, &stringHandler);
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(ctl, allocCmd, errorMessage))
        return false;
   // "Allocated 1000 bytes starting at 003a0000" .. hopefully never localized
    bool ok = false;
    const QString output = stringHandler.result();
    const int lastBlank = output.lastIndexOf(QLatin1Char(' '));
    if (lastBlank != -1) {
        const qint64 addri = output.mid(lastBlank + 1).toLongLong(&ok, 16);
        if (ok)
            *address = addri;
    }
    if (!ok) {
        *errorMessage = QString::fromLatin1("Failed to parse output '%1'").arg(output);
        return false;
    }
    return true;
}

// Alloc an AscII string in debuggee
static bool createDebuggeeAscIIString(CIDebugControl *ctl,
                                CIDebugClient *client,
                                CIDebugDataSpaces *data,
                                const QString &s,
                                ULONG64 *address,
                                QString *errorMessage)
{
    QByteArray sAsciiData = s.toLocal8Bit();
    sAsciiData += '\0';
    if (!allocDebuggeeMemory(ctl, client, sAsciiData.size(), address, errorMessage))
        return false;
    const HRESULT hr = data->WriteVirtual(*address, sAsciiData.data(), sAsciiData.size(), 0);
    if (FAILED(hr)) {
        *errorMessage= msgComFailed("WriteVirtual", hr);
        return false;
    }
    return true;
}

// Locate 'qstrdup' in the (potentially namespaced) corelib. For some
// reason, the symbol is present in QtGui as well without type information.
static inline QString resolveStrdup(CIDebugSymbols *syms, QString *errorMessage)
{
    QStringList matches;
    const QString pattern = QLatin1String("*qstrdup");
    const QRegExp corelibPattern(QLatin1String("QtCore[d]*4!"));
    Q_ASSERT(corelibPattern.isValid());
    if (!searchSymbols(syms, pattern, &matches, errorMessage))
        return QString();
    QStringList corelibStrdup = matches.filter(corelibPattern);
    if (corelibStrdup.isEmpty()) {
        *errorMessage = QString::fromLatin1("Unable to locate '%1' in '%2' (%3)").
                        arg(pattern, corelibPattern.pattern(), matches.join(QString(QLatin1Char(','))));
        return QString();
    }
    return corelibStrdup.front();
}

// Load a library into the debuggee. Currently requires
// the QtCored4.pdb file to be present as we need "qstrdup"
// as dummy symbol. This is ok ATM since dumpers only
// make sense for Qt apps.
static bool debuggeeLoadLibrary(CIDebugControl *ctl,
                         CIDebugClient *client,
                         CIDebugSymbols *syms,
                         CIDebugDataSpaces *data,
                         const QString &moduleName, QString *errorMessage)
{
    if (loadDebug)
        qDebug() << Q_FUNC_INFO << moduleName;
    // Try to ignore the breakpoints
    IgnoreDebugEventCallback devNull;
    EventCallbackRedirector eventRedir(client, &devNull);
    // Make a call to LoadLibraryA. First, reserve memory in debugger
    // and copy name over.
    ULONG64 nameAddress;
    if (!createDebuggeeAscIIString(ctl, client, data, moduleName, &nameAddress, errorMessage))
        return false;
    // We want to call "HMODULE LoadLibraryA(LPCTSTR lpFileName)"
    // (void* LoadLibraryA(char*)). However, despite providing a symbol
    // server, the debugger refuses to recognize it as a function.
    // Set up the call stack with a function of same signature (qstrdup)
    // and change the call register to LoadLibraryA() before executing "g".
    // Prepare call.
    const QString dummyFunc = resolveStrdup(syms, errorMessage);
    if (dummyFunc.isEmpty())
        return false;
    QString callCmd = QLatin1String(".call ");
    callCmd += dummyFunc;
    callCmd += QLatin1String("(0x");
    callCmd += QString::number(nameAddress, 16);
    callCmd += QLatin1Char(')');
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(ctl, callCmd, errorMessage))
        return false;
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(ctl, QLatin1String("r eip=Kernel32!LoadLibraryA"), errorMessage))
        return false;
    // This will hit a breakpoint
    if (loadDebug)
        qDebug() << " executing 'g'";
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(ctl, QString(QLatin1Char('g')), errorMessage))
        return false;
    // @Todo: We cannot evaluate output here as it is asynchronous
    return true;
}

CdbDumperHelper::CdbDumperHelper(CdbComInterfaces *cif) :
    m_state(NotLoaded),
    m_cif(cif)
{
}

void CdbDumperHelper::reset(const QString &library, bool enabled)
{
    m_library = library;
    m_state = enabled ? NotLoaded : Disabled;
    m_dumpObjectSymbol = QLatin1String("qDumpObjectData440");
    m_errorMessage.clear();
}

bool CdbDumperHelper::moduleLoadHook(const QString &name, bool *ignoreNextBreakPoint)
{
    *ignoreNextBreakPoint = false;
    bool ok = true; // report failure only once
    switch (m_state) {
    case Disabled:
        break;
    case NotLoaded:
        // Load once QtCore is there.
        if (name.contains(QLatin1String("QtCore"))) {
            if (loadDebug)
                qDebug() << Q_FUNC_INFO << '\n' << name << m_state << executionStatusString(m_cif->debugControl);
            ok = debuggeeLoadLibrary(m_cif->debugControl, m_cif->debugClient, m_cif->debugSymbols, m_cif->debugDataSpaces,
                                     m_library, &m_errorMessage);
            if (ok) {
                m_state = Loading;
                *ignoreNextBreakPoint = true;
            } else {
                m_state = Failed;
            }
            if (loadDebug)
                qDebug() << m_state << executionStatusString(m_cif->debugControl);
        }
        break;
    case Loading:
        // Hurray, loaded. Now resolve the symbols we need
        if (name.contains(QLatin1String("gdbmacros"))) {
            ok = resolveSymbols(&m_errorMessage);
            if (ok) {
                m_state = Loaded;
            } else {
                m_state = Failed;
            }
        }
        break;
    case Loaded:
        break;
    case Failed:
        break;
    };    
    return ok;
}

bool CdbDumperHelper::resolveSymbols(QString *errorMessage)
{    
    // Resolve the symbols we need
    m_dumpObjectSymbol = QLatin1String("qDumpObjectData440");
    const bool rc = resolveSymbol(m_cif->debugSymbols, &m_dumpObjectSymbol, errorMessage) == ResolveSymbolOk;
    if (loadDebug)
        qDebug() << Q_FUNC_INFO << '\n' << rc << m_dumpObjectSymbol;
    return rc;
}

} // namespace Internal
} // namespace Debugger
