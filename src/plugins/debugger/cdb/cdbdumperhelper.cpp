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
#include "watchutils.h"

#include <QtCore/QRegExp>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

enum { loadDebug = 0 };

static const char *dumperModuleNameC = "gdbmacros";
static const char *qtCoreModuleNameC = "QtCore";
static const ULONG waitTimeOutMS = 30000;

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
     if (loadDebug)
        qDebug() << Q_FUNC_INFO << '\n' << output << *address << ok;
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
    // Prepare call: Locate 'qstrdup' in the (potentially namespaced) corelib. For some
    // reason, the symbol is present in QtGui as well without type information.
    QString dummyFunc = QLatin1String("*qstrdup");
    if (resolveSymbol(syms, QLatin1String("QtCore[d]*4!"), &dummyFunc, errorMessage) != ResolveSymbolOk)
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
    // This will hit a breakpoint.
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(ctl, QString(QLatin1Char('g')), errorMessage))
        return false;    
    const HRESULT hr = ctl->WaitForEvent(0, waitTimeOutMS);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("WaitForEvent", hr);
        return false;
    }
    return true;
}

// ------------------- CdbDumperHelper::DumperInputParameters
struct CdbDumperHelper::DumperInputParameters {
    DumperInputParameters(int protocolVersion_ = 1,
                          int token_ = 1,
                          quint64 Address_ = 0,
                          bool dumpChildren_ = false,
                          int extraInt0_ = 0,
                          int extraInt1_ = 0,
                          int extraInt2_ = 0,
                          int extraInt3_ = 0);

    QString command(const QString &dumpFunction) const;

    int protocolVersion;
    int token;
    quint64 address;
    bool dumpChildren;
    int extraInt0;
    int extraInt1;
    int extraInt2;
    int extraInt3;
};

CdbDumperHelper::DumperInputParameters::DumperInputParameters(int protocolVersion_,
                                                              int token_,
                                                              quint64 address_,
                                                              bool dumpChildren_,
                                                              int extraInt0_,
                                                              int extraInt1_,
                                                              int extraInt2_,
                                                              int extraInt3_) :
    protocolVersion(protocolVersion_),
    token(token_),
    address(address_),
    dumpChildren(dumpChildren_),
    extraInt0(extraInt0_),
    extraInt1(extraInt1_),
    extraInt2(extraInt2_),
    extraInt3(extraInt3_)
{
}

QString CdbDumperHelper::DumperInputParameters::command(const QString &dumpFunction) const
{
    QString rc;
    QTextStream str(&rc);
    str.setIntegerBase(16);
    str << ".call " << dumpFunction << '(' << protocolVersion << ',' << token << ',';
    str.setIntegerBase(16);
    str << "0x" << address;
    str.setIntegerBase(10);
    str << ',' << (dumpChildren ? 1 : 0) << ',' << extraInt0 << ',' << extraInt1
            << ',' << extraInt2 << ',' << extraInt3 << ')';
    return rc;
}

// ------------------- CdbDumperHelper

CdbDumperHelper::CdbDumperHelper(CdbComInterfaces *cif) :    
    m_state(NotLoaded),
    m_cif(cif),
    m_inBufferAddress(0),
    m_inBufferSize(0),
    m_outBufferAddress(0),
    m_outBufferSize(0),
    m_buffer(0)
{
}

CdbDumperHelper::~CdbDumperHelper()
{
    clearBuffer();
}

void CdbDumperHelper::clearBuffer()
{
    if (m_buffer) {
        delete [] m_buffer;
        m_buffer = 0;
    }
}

void CdbDumperHelper::reset(const QString &library, bool enabled)
{
    m_library = library;
    m_state = enabled ? NotLoaded : Disabled;
    m_dumpObjectSymbol = QLatin1String("qDumpObjectData440");
    m_knownTypes.clear();
    m_qtVersion.clear();
    m_qtNamespace.clear();
    m_inBufferAddress = m_outBufferAddress = 0;
    m_inBufferSize = m_outBufferSize = 0;
    clearBuffer();
}

// Attempt to load and initialize dumpers, give feedback
// to user.
void CdbDumperHelper::load(DebuggerManager *manager, IDebuggerManagerAccessForEngines *access)
{
    enum Result { Failed, Succeeded, NoQtApp };

    if (m_state != NotLoaded)
        return;
    manager->showStatusMessage(QCoreApplication::translate("CdbDumperHelper", "Loading dumpers..."), 10000);
    const QString messagePrefix = QLatin1String("dumper:");
    QString message;
    Result result = Failed;
    do {
        // Do we have Qt and are we already loaded by accident?
        QStringList modules;
        if (!getModuleNameList(m_cif->debugSymbols, &modules, &message))
            break;
        if (modules.filter(QLatin1String(qtCoreModuleNameC)).isEmpty()) {
            message = QCoreApplication::translate("CdbDumperHelper", "The debugger does not appear to be Qt application.");
            result = NoQtApp;
        }
        // Make sure the dumper lib is loaded.
        if (modules.filter(QLatin1String(dumperModuleNameC), Qt::CaseInsensitive).isEmpty()) {
            // Try to load
            if (!debuggeeLoadLibrary(m_cif->debugControl, m_cif->debugClient, m_cif->debugSymbols, m_cif->debugDataSpaces,
                                     m_library, &message)) {
                break;
            }
        } else {
            access->showDebuggerOutput(messagePrefix, QCoreApplication::translate("CdbDumperHelper", "The dumper module appears to be already loaded."));
        }
        // Resolve symbols and do call to get types
        if (!resolveSymbols(&message))
            break;
        if (!getKnownTypes(&message))
            break;
        message = QCoreApplication::translate("CdbDumperHelper", "Dumper library '%1' loaded.").arg(m_library);
        result = Succeeded;
    } while (false);
    // eval state and notify user
    switch (result) {
    case Failed:
        message = QCoreApplication::translate("CdbDumperHelper", "The dumper library '%1' could not be loaded:\n%2").arg(m_library, message);
        access->showDebuggerOutput(messagePrefix, message);
        access->showQtDumperLibraryWarning(message);
        m_state = Disabled;
        break;
    case Succeeded:
        access->showDebuggerOutput(messagePrefix, message);
        access->showDebuggerOutput(messagePrefix, statusMessage());
        m_state = Loaded;
        break;
    case NoQtApp:
        access->showDebuggerOutput(messagePrefix, message);
        m_state = Disabled;
        break;
    }    
    if (loadDebug)
        qDebug() << Q_FUNC_INFO << "\n<" << result << '>' << m_state << message;
}

QString CdbDumperHelper::statusMessage() const
{
    const QString nameSpace = m_qtNamespace.isEmpty() ? QCoreApplication::translate("CdbDumperHelper", "<none>") : m_qtNamespace;
    return QCoreApplication::translate("CdbDumperHelper",
                                       "%n known types, Qt version: %1, Qt namespace: %2",
                                       0, QCoreApplication::CodecForTr,
                                       m_knownTypes.size()).arg(m_qtVersion, nameSpace);
}

// Retrieve address and optionally size of a symbol.
static inline bool getSymbolAddress(CIDebugSymbols *sg,
                                    const QString &name,
                                    quint64 *address,
                                    ULONG *size /* = 0*/,
                                    QString *errorMessage)
{
    // Get address
    HRESULT hr = sg->GetOffsetByNameWide(name.utf16(), address);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetOffsetByNameWide", hr);
        return false;
    }
    // Get size. Even works for arrays
    if (size) {
        ULONG64 moduleAddress;
        ULONG type;
        hr = sg->GetOffsetTypeId(*address, &type, &moduleAddress);
        if (FAILED(hr)) {
            *errorMessage = msgComFailed("GetOffsetTypeId", hr);
            return false;
        }
        hr = sg->GetTypeSize(moduleAddress, type, size);
        if (FAILED(hr)) {
            *errorMessage = msgComFailed("GetTypeSize", hr);
            return false;
        }
    } // size desired
    return true;
}

bool CdbDumperHelper::resolveSymbols(QString *errorMessage)
{    
    // Resolve the symbols we need (potentially namespaced).
    // There is a 'qDumpInBuffer' in QtCore as well.
    m_dumpObjectSymbol = QLatin1String("*qDumpObjectData440");
    QString inBufferSymbol = QLatin1String("*qDumpInBuffer");
    QString outBufferSymbol = QLatin1String("*qDumpOutBuffer");
    const QString dumperModuleName = QLatin1String(dumperModuleNameC);
    bool rc = resolveSymbol(m_cif->debugSymbols, &m_dumpObjectSymbol, errorMessage) == ResolveSymbolOk
                    && resolveSymbol(m_cif->debugSymbols, dumperModuleName, &inBufferSymbol, errorMessage) == ResolveSymbolOk
                    && resolveSymbol(m_cif->debugSymbols, dumperModuleName, &outBufferSymbol, errorMessage) == ResolveSymbolOk;
    if (!rc)
        return false;
    //  Determine buffer addresses, sizes and alloc buffer
    rc = getSymbolAddress(m_cif->debugSymbols, inBufferSymbol, &m_inBufferAddress, &m_inBufferSize, errorMessage)
         && getSymbolAddress(m_cif->debugSymbols, outBufferSymbol, &m_outBufferAddress, &m_outBufferSize, errorMessage);
    if (!rc)
        return false;
    m_buffer = new char[qMax(m_inBufferSize, m_outBufferSize)];
    if (loadDebug)
        qDebug().nospace() << Q_FUNC_INFO << '\n' << rc << m_dumpObjectSymbol
                << " buffers at 0x" << QString::number(m_inBufferAddress, 16) << ','
                << m_inBufferSize << "; 0x"
                << QString::number(m_outBufferAddress, 16) << ',' << m_outBufferSize << '\n';
    return true;
}

bool CdbDumperHelper::getKnownTypes(QString *errorMessage)
{
    QByteArray output;
    if (!callDumper(DumperInputParameters(1), &output, errorMessage)) {
        return false;
    }
    if (!parseQueryDumperOutput(output, &m_knownTypes, &m_qtVersion, &m_qtNamespace)) {
     *errorMessage = QString::fromLatin1("Unable to parse the dumper output: '%1'").arg(QString::fromAscii(output));
    }
    if (loadDebug)
        qDebug() << Q_FUNC_INFO << m_knownTypes << m_qtVersion  << m_qtNamespace;
    return true;
}

bool CdbDumperHelper::callDumper(const DumperInputParameters &p, QByteArray *output, QString *errorMessage)
{
    IgnoreDebugEventCallback devNull;
    EventCallbackRedirector eventRedir(m_cif->debugClient, &devNull);
    const QString callCmd = p.command(m_dumpObjectSymbol);
    // Set up call and a temporary breakpoint after it.
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(m_cif->debugControl, callCmd, errorMessage))
        return false;    
    // Go and filter away break event
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(m_cif->debugControl, QString(QLatin1Char('g')), errorMessage))
        return false;
    HRESULT hr = m_cif->debugControl->WaitForEvent(0, waitTimeOutMS);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("WaitForEvent", hr);
        return false;
    }
    // Read output
    hr = m_cif->debugDataSpaces->ReadVirtual(m_outBufferAddress, m_buffer, m_outBufferSize, 0);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("ReadVirtual", hr);
        return false;
    }
    // see QDumper implementation
    const char result = m_buffer[0];
    switch (result) {
    case 't':
        break;
    case '+':
        *errorMessage = QString::fromLatin1("Dumper call '%1' resulted in output overflow.").arg(callCmd);
        return false;
    case 'f':
        *errorMessage = QString::fromLatin1("Dumper call '%1' failed.").arg(callCmd);
        return false;
    default:
        *errorMessage = QString::fromLatin1("Dumper call '%1' failed ('%2').").arg(callCmd).arg(QLatin1Char(result));
        return false;
    }
    *output = QByteArray(m_buffer + 1);
    return true;
}

} // namespace Internal
} // namespace Debugger
