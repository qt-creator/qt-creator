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
#include "cdbsymbolgroupcontext.h"
#include "watchhandler.h"

#include <QtCore/QRegExp>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

enum { loadDebug = 0 };

static const char *dumperModuleNameC = "gdbmacros";
static const char *qtCoreModuleNameC = "QtCore";
static const ULONG waitTimeOutMS = 30000;
static const char *dumperPrefixC  = "dumper:";
namespace Debugger {
namespace Internal {

// Alloc memory in debuggee using the ".dvalloc" command as
// there seems to be no API for it.
static bool allocDebuggeeMemory(CdbComInterfaces *cif,
                                int size, ULONG64 *address, QString *errorMessage)
{
    *address = 0;
    const QString allocCmd = QLatin1String(".dvalloc ") + QString::number(size);
    StringOutputHandler stringHandler;
    OutputRedirector redir(cif->debugClient, &stringHandler);
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(cif->debugControl, allocCmd, errorMessage))
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
     if (loadDebug > 1)
        qDebug() << Q_FUNC_INFO << '\n' << output << *address << ok;
    if (!ok) {
        *errorMessage = QString::fromLatin1("Failed to parse output '%1'").arg(output);
        return false;
    }
    return true;
}

// Alloc an AscII string in debuggee
static bool createDebuggeeAscIIString(CdbComInterfaces *cif,
                                      const QString &s,
                                      ULONG64 *address,
                                      QString *errorMessage)
{
    QByteArray sAsciiData = s.toLocal8Bit();
    sAsciiData += '\0';
    if (!allocDebuggeeMemory(cif, sAsciiData.size(), address, errorMessage))
        return false;
    const HRESULT hr = cif->debugDataSpaces->WriteVirtual(*address, sAsciiData.data(), sAsciiData.size(), 0);
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
static bool debuggeeLoadLibrary(IDebuggerManagerAccessForEngines *access,
                                CdbComInterfaces *cif,
                                const QString &moduleName,
                                QString *errorMessage)
{
    if (loadDebug > 1)
        qDebug() << Q_FUNC_INFO << moduleName;
    // Try to ignore the breakpoints
    CdbExceptionLoggerEventCallback exLogger(QLatin1String(dumperPrefixC), access);
    EventCallbackRedirector eventRedir(cif->debugClient, &exLogger);
    // Make a call to LoadLibraryA. First, reserve memory in debugger
    // and copy name over.
    ULONG64 nameAddress;
    if (!createDebuggeeAscIIString(cif, moduleName, &nameAddress, errorMessage))
        return false;
    // We want to call "HMODULE LoadLibraryA(LPCTSTR lpFileName)"
    // (void* LoadLibraryA(char*)). However, despite providing a symbol
    // server, the debugger refuses to recognize it as a function.
    // Set up the call stack with a function of same signature (qstrdup)
    // and change the call register to LoadLibraryA() before executing "g".
    // Prepare call: Locate 'qstrdup' in the (potentially namespaced) corelib. For some
    // reason, the symbol is present in QtGui as well without type information.
    QString dummyFunc = QLatin1String("*qstrdup");
    if (resolveSymbol(cif->debugSymbols, QLatin1String("QtCore[d]*4!"), &dummyFunc, errorMessage) != ResolveSymbolOk)
        return false;
    QString callCmd = QLatin1String(".call ");
    callCmd += dummyFunc;
    callCmd += QLatin1String("(0x");
    callCmd += QString::number(nameAddress, 16);
    callCmd += QLatin1Char(')');
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(cif->debugControl, callCmd, errorMessage))
        return false;
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(cif->debugControl, QLatin1String("r eip=Kernel32!LoadLibraryA"), errorMessage))
        return false;
    // This will hit a breakpoint.
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(cif->debugControl, QString(QLatin1Char('g')), errorMessage))
        return false;    
    const HRESULT hr = cif->debugControl->WaitForEvent(0, waitTimeOutMS);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("WaitForEvent", hr);
        return false;
    }
    return true;
}

// ------------------- CdbDumperHelper

CdbDumperHelper::CdbDumperHelper(IDebuggerManagerAccessForEngines *access,
                                 CdbComInterfaces *cif) :
    m_messagePrefix(QLatin1String(dumperPrefixC)),
    m_state(NotLoaded),
    m_access(access),
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
    m_helper.clear();
    m_inBufferAddress = m_outBufferAddress = 0;
    m_inBufferSize = m_outBufferSize = 0;
    m_failedTypes.clear();
    clearBuffer();
}

// Attempt to load and initialize dumpers, give feedback
// to user.
void CdbDumperHelper::load(DebuggerManager *manager)
{
    enum Result { Failed, Succeeded, NoQtApp };

    if (m_state != NotLoaded)
        return;
    manager->showStatusMessage(QCoreApplication::translate("CdbDumperHelper", "Loading dumpers..."), 10000);
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
            if (!debuggeeLoadLibrary(m_access, m_cif, m_library, &message)) {
                break;
            }
        } else {
            m_access->showDebuggerOutput(m_messagePrefix, QCoreApplication::translate("CdbDumperHelper", "The dumper module appears to be already loaded."));
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
        m_access->showDebuggerOutput(m_messagePrefix, message);
        m_access->showQtDumperLibraryWarning(message);
        m_state = Disabled;
        break;
    case Succeeded:
        m_access->showDebuggerOutput(m_messagePrefix, message);
        m_access->showDebuggerOutput(m_messagePrefix, m_helper.toString());
        m_state = Loaded;
        break;
    case NoQtApp:
        m_access->showDebuggerOutput(m_messagePrefix, message);
        m_state = Disabled;
        break;
    }    
    if (loadDebug)
        qDebug() << Q_FUNC_INFO << "\n<" << result << '>' << m_state << message;
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
    QString callCmd;
    QTextStream(&callCmd) << ".call " << m_dumpObjectSymbol << "(1,0,0,0,0,0,0,0)";
    const char *outData;
    if (!callDumper(callCmd, QByteArray(), &outData, errorMessage)) {
        return false;
    }
    if (!m_helper.parseQuery(outData, QtDumperHelper::CdbDebugger)) {
     *errorMessage = QString::fromLatin1("Unable to parse the dumper output: '%1'").arg(QString::fromAscii(output));
    }
    if (loadDebug)
        qDebug() << Q_FUNC_INFO << m_helper.toString(true);
    return true;
}

// Write to debuggee memory in chunks
bool CdbDumperHelper::writeToDebuggee(CIDebugDataSpaces *ds, const QByteArray &buffer, quint64 address, QString *errorMessage)
{
    char *ptr = const_cast<char*>(buffer.data());
    ULONG bytesToWrite = buffer.size();
    while (bytesToWrite > 0) {
        ULONG bytesWritten = 0;
        const HRESULT hr = ds->WriteVirtual(address, ptr, bytesToWrite, &bytesWritten);
        if (FAILED(hr)) {
            *errorMessage = msgComFailed("WriteVirtual", hr);
            return false;
        }
        bytesToWrite -= bytesWritten;
        ptr += bytesWritten;
    }
    return true;
}

bool CdbDumperHelper::callDumper(const QString &callCmd, const QByteArray &inBuffer, const char **outDataPtr, QString *errorMessage)
{
    *outDataPtr = 0;
    CdbExceptionLoggerEventCallback exLogger(m_messagePrefix, m_access);
    EventCallbackRedirector eventRedir(m_cif->debugClient, &exLogger);
    // write input buffer
    if (!inBuffer.isEmpty()) {
        if (!writeToDebuggee(m_cif->debugDataSpaces, inBuffer, m_inBufferAddress, errorMessage))
            return false;
    }
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(m_cif->debugControl, callCmd, errorMessage))
        return false;
    // Set up call and a temporary breakpoint after it.
    // Try to skip debuggee crash exceptions and dumper exceptions
    // by using 'gh' (go handled)
    for (int i = 0; i < 10; i++) {
        const int oldExceptionCount = exLogger.exceptionCount();
        // Go. If an exception occurs in loop 2, let the dumper handle it.
        const QString goCmd = i ? QString(QLatin1String("gN")) : QString(QLatin1Char('g'));
        if (!CdbDebugEnginePrivate::executeDebuggerCommand(m_cif->debugControl, goCmd, errorMessage))
            return false;
        HRESULT hr = m_cif->debugControl->WaitForEvent(0, waitTimeOutMS);
        if (FAILED(hr)) {
            *errorMessage = msgComFailed("WaitForEvent", hr);
            return false;
        }
        const int newExceptionCount = exLogger.exceptionCount();
        // no new exceptions? -> break
        if (oldExceptionCount == newExceptionCount)
            break;
    }
    if (exLogger.exceptionCount()) {
        const QString exMsgs = exLogger.exceptionMessages().join(QString(QLatin1Char(',')));
        *errorMessage = QString::fromLatin1("Exceptions occurred during the dumper call: %1").arg(exMsgs);
        return false;
    }
    // Read output
    const HRESULT hr = m_cif->debugDataSpaces->ReadVirtual(m_outBufferAddress, m_buffer, m_outBufferSize, 0);
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
    *outDataPtr = m_buffer + 1;
    return true;
}

CdbDumperHelper::DumpResult CdbDumperHelper::dumpType(const WatchData &wd, bool dumpChildren, int source,
                                                      QList<WatchData> *result, QString *errorMessage)
{
    // Check failure cache and supported types
    if (m_failedTypes.contains(wd.type))
        return DumpNotHandled;
    const QtDumperHelper::TypeData td = m_helper.typeData(wd.type);    
    if (loadDebug)
        qDebug() << wd.type << td;
    if (td.type == QtDumperHelper::UnknownType)
        return DumpNotHandled;

    // Now evaluate
    const QString message = QCoreApplication::translate("CdbDumperHelper",
                                                        "Querying dumpers for '%1'/'%2' (%3)").
                                                        arg(wd.name, wd.exp, wd.type);
    m_access->showDebuggerOutput(m_messagePrefix, message);
    const DumpExecuteResult der = executeDump(wd, td, dumpChildren, source, result, errorMessage);
    if (der == DumpExecuteOk)
        return DumpOk;
    // Cache types that fail due to complicated template size expressions.
    // Exceptions OTOH might occur when accessing variables that are not
    // yet initialized in a particular breakpoint. That should be ignored
    if (der == DumpExecuteSizeFailed)
        m_failedTypes.push_back(wd.type);    
    // log error
    *errorMessage = QString::fromLatin1("Unable to dump '%1' (%2): %3").arg(wd.name, wd.type, *errorMessage);
    m_access->showDebuggerOutput(m_messagePrefix, *errorMessage);
    return DumpError;
}

CdbDumperHelper::DumpExecuteResult
    CdbDumperHelper::executeDump(const WatchData &wd,
                                const QtDumperHelper::TypeData& td, bool dumpChildren, int source,
                                QList<WatchData> *result, QString *errorMessage)
{
    QByteArray inBuffer;
    QStringList extraParameters;    
    // Build parameter list.
    m_helper.evaluationParameters(wd, td, QtDumperHelper::CdbDebugger, &inBuffer, &extraParameters);
    // If the parameter list contains sizeof-expressions, execute them separately
    // and replace them by the resulting numbers
    const QString sizeOfExpr = QLatin1String("sizeof");
    const QStringList::iterator eend = extraParameters.end();
    for (QStringList::iterator it = extraParameters.begin() ; it != eend; ++it) {
        // Strip 'sizeof(X)' to 'X' and query size
        QString &ep = *it;
        if (ep.startsWith(sizeOfExpr)) {
            int size;
            ep.truncate(ep.lastIndexOf(QLatin1Char(')')));
            ep.remove(0, ep.indexOf(QLatin1Char('(')) + 1);
            const bool sizeOk = getTypeSize(ep, &size, errorMessage);
            if (loadDebug)
                qDebug() << "Size" << sizeOk << size << ep;
            if (!sizeOk)
                return DumpExecuteSizeFailed;
            ep = QString::number(size);
        }
    }
    // Execute call
    QString callCmd;
    QTextStream(&callCmd) << ".call " << m_dumpObjectSymbol
            << "(2,0," << wd.addr << ','
            << (dumpChildren ? 1 : 0) << ',' << extraParameters.join(QString(QLatin1Char(','))) << ')';
    if (loadDebug)
        qDebug() << "Query: " << wd.toString() << "\nwith: " << callCmd << '\n';
    const char *outputData;
    if (!callDumper(callCmd, inBuffer, &outputData, errorMessage))
        return DumpExecuteCallFailed;
    QtDumperResult dumpResult;
    if (!QtDumperHelper::parseValue(outputData, &dumpResult)) {
        *errorMessage = QLatin1String("Parsing of value query output failed.");
        return DumpExecuteCallFailed;
    }
    *result = dumpResult.toWatchData(source);
    return DumpExecuteOk;
}

// Simplify some types for sizeof expressions
static inline void simplifySizeExpression(QString *typeName)
{
    typeName->replace(QLatin1String("std::basic_string<char,std::char_traits<char>,std::allocator<char>>"),
                      QLatin1String("std::string"));
}

bool CdbDumperHelper::getTypeSize(const QString &typeNameIn, int *size, QString *errorMessage)
{
    if (loadDebug > 1)
        qDebug() << Q_FUNC_INFO << typeNameIn;
    // Look up cache
    QString typeName = typeNameIn;
    simplifySizeExpression(&typeName);
    // "std::" types sometimes only work without namespace.
    // If it fails, try again with stripped namespace
    *size = 0;
    bool success = false;
    do {
        if (runTypeSizeQuery(typeName, size, errorMessage)) {
            success = true;
            break;
        }
        const QString stdNameSpace = QLatin1String("std::");
        if (!typeName.contains(stdNameSpace))
            break;
        typeName.remove(stdNameSpace);
        errorMessage->clear();
        if (!runTypeSizeQuery(typeName, size, errorMessage))
            break;
        success = true;
    } while (false);
    // Cache in dumper helper
    if (success)
        m_helper.addSize(typeName, *size);
    return success;
}

bool CdbDumperHelper::runTypeSizeQuery(const QString &typeName, int *size, QString *errorMessage)
{
    // Retrieve by C++ expression. If we knew the module, we could make use
    // of the TypeId query mechanism provided by the IDebugSymbolGroup.
    DEBUG_VALUE sizeValue;
    QString expression = QLatin1String("sizeof(");
    expression += typeName;
    expression += QLatin1Char(')');
    if (!CdbDebugEnginePrivate::evaluateExpression(m_cif->debugControl,
                                                  expression, &sizeValue, errorMessage))
        return false;
    qint64 size64;
    if (!CdbSymbolGroupContext::debugValueToInteger(sizeValue, &size64)) {
        *errorMessage = QLatin1String("Expression result is not an integer");
        return false;
    }
    *size = static_cast<int>(size64);
    return true;
}

} // namespace Internal
} // namespace Debugger
