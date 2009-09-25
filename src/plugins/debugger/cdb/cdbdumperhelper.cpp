/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdbdumperhelper.h"
#include "cdbmodules.h"
#include "cdbdebugengine_p.h"
#include "cdbdebugoutput.h"
#include "cdbdebugeventcallback.h"
#include "cdbsymbolgroupcontext.h"
#include "watchhandler.h"
#include "cdbexceptionutils.h"

#include "shared/sharedlibraryinjector.h"

#include <QtCore/QRegExp>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

enum { loadDebug = 0 };

static const char *dumperModuleNameC = "gdbmacros";
static const char *qtCoreModuleNameC = "QtCore";
static const ULONG waitTimeOutMS = 30000;
static const char *dumperPrefixC  = "dumper:";

/* Loading the dumpers is 2 step process:
 * 1) The library must be loaded into the debuggee, for which there are
 *    2 approaches:
 *     - Injection loading using the SharedLibraryInjector which
 *       launches a remote thread in the debuggee which loads the
 *       library. Drawbacks:
 *       * The remote thread must not starve.
 *       * It is not possible to wait loading and loading occurs late,
 *         after entering main()
 *     - Debugger Call loading, which has the debuggee execute
 *       a LoadLibrary call via debugger commands. Drawbacks:
 *       * Slow
 *       * Requires presence of a symbol of the same prototype as
 *         LoadLibraryA as the original prototype is not sufficient.
 * 2) Call a query function  (protocol 1 of the dumper) to obtain a list
 *    of handled types and a map of known sizes.
 *
 * The class currently launches injection loading from the module
 * load hook as soon as it sees a Qt module.
 * The dumpType() function performs the rest of the [delayed] initialization.
 * If the load has not finished, it attempts call loading and
 * executes the initial query protocol.
 *
 * Note: The main technique here is having the debuggee call functions
 * using the .call command (which takes a function with a known
 * prototype and simple, integer parameters).
 * This does not work from an IDebugEvent callback, as it will cause
 * WaitForEvent() to fail with unknown errors.
 * It mostly works from breakpoints, with the addditional restriction
 * that complex functions only work from 'well-defined'  breakpoints
 * (such as main()) and otherwise cause access violation exceptions
 * (for example LoadLibrary).
 * Exceptions occuring in the functions to be called must be handled
 * by __try/__except (they show up in the debugger and must acknowledged
 * by gN (go not handled).  */

namespace Debugger {
namespace Internal {

// ------- Call load helpers
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
static bool debuggeeLoadLibrary(DebuggerManager *manager,
                                CdbComInterfaces *cif,
                                const QString &moduleName,
                                QString *errorMessage)
{
    if (loadDebug > 1)
        qDebug() << Q_FUNC_INFO << moduleName;
    // Try to ignore the breakpoints
    CdbExceptionLoggerEventCallback exLogger(LogWarning, manager);
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

// ---- Load messages
static inline QString msgMethod(bool injectOrCall)
{
    return injectOrCall ?
            QCoreApplication::translate("Debugger::Internal::CdbDumperHelper", "injection") :
            QCoreApplication::translate("Debugger::Internal::CdbDumperHelper", "debugger call");
}

static QString msgLoading(const QString &library, bool injectOrCall)
{
    return QCoreApplication::translate("Debugger::Internal::CdbDumperHelper",
                                       "Loading the custom dumper library '%1' (%2) ...").
                                       arg(library, msgMethod(injectOrCall));
}

static QString msgLoadFailed(const QString &library, bool injectOrCall, const QString &why)
{
    return QCoreApplication::translate("Debugger::Internal::CdbDumperHelper",
                                       "Loading of the custom dumper library '%1' (%2) failed: %3").
                                       arg(library, msgMethod(injectOrCall), why);
}

static QString msgLoadSucceeded(const QString &library, bool injectOrCall)
{
        return QCoreApplication::translate("Debugger::Internal::CdbDumperHelper",
                                       "Loaded the custom dumper library '%1' (%2).").
                                       arg(library, msgMethod(injectOrCall));
}

// ------------------- CdbDumperHelper

CdbDumperHelper::CdbDumperHelper(DebuggerManager *manager,
                                 CdbComInterfaces *cif) :
    m_tryInjectLoad(true),
    m_state(NotLoaded),
    m_manager(manager),
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

void CdbDumperHelper::disable()
{
    if (loadDebug)
        qDebug() << Q_FUNC_INFO;
    m_manager->showDebuggerOutput(LogMisc, QCoreApplication::translate("Debugger::Internal::CdbDumperHelper", "Disabling dumpers due to debuggee crash..."));
    m_state = Disabled;
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
     if (loadDebug)
        qDebug() << Q_FUNC_INFO << '\n' << library << enabled;
    m_library = library;
    m_state = enabled ? NotLoaded : Disabled;
    m_dumpObjectSymbol = QLatin1String("qDumpObjectData440");
    m_helper.clear();
    m_inBufferAddress = m_outBufferAddress = 0;
    m_inBufferSize = m_outBufferSize = 0;
    m_failedTypes.clear();
    clearBuffer();
}

void CdbDumperHelper::moduleLoadHook(const QString &module, HANDLE debuggeeHandle)
{
    if (loadDebug > 1)
        qDebug() << "moduleLoadHook" << module << m_state << debuggeeHandle;
    switch (m_state) {
    case Disabled:
    case Initialized:
        break;
    case NotLoaded:
        // Try an inject load as soon as a Qt lib is loaded.
        // for the thread to finish as this would lock up.
        if (m_tryInjectLoad && module.contains(QLatin1String("Qt"), Qt::CaseInsensitive)) {
            // Also shows up in the log window.
            m_manager->showStatusMessage(msgLoading(m_library, true), 10000);
            QString errorMessage;
            SharedLibraryInjector sh(GetProcessId(debuggeeHandle));
            if (sh.remoteInject(m_library, false, &errorMessage)) {
                m_state = InjectLoading;
            } else {
                m_state = InjectLoadFailed;
                // Ok, try call loading...
                m_manager->showDebuggerOutput(LogMisc, msgLoadFailed(m_library, true, errorMessage));
            }
        }
        break;
    case InjectLoading:
        // check if gdbmacros.dll loaded
        if (module.contains(QLatin1String(dumperModuleNameC), Qt::CaseInsensitive)) {
            m_state = Loaded;
            m_manager->showDebuggerOutput(LogMisc, msgLoadSucceeded(m_library, true));
        }
        break;
    }
}

// Try to load dumpers by triggering calls using the debugger
CdbDumperHelper::CallLoadResult CdbDumperHelper::initCallLoad(QString *errorMessage)
{
    if (loadDebug)
        qDebug() << Q_FUNC_INFO;
    // Do we have Qt and are we already loaded by accident?
    QStringList modules;
    if (!getModuleNameList(m_cif->debugSymbols, &modules, errorMessage))
        return CallLoadError;
    // Are we already loaded by some accident?
    if (!modules.filter(QLatin1String(dumperModuleNameC), Qt::CaseInsensitive).isEmpty())
        return CallLoadAlreadyLoaded;
    // Is that Qt application at all?
    if (modules.filter(QLatin1String(qtCoreModuleNameC), Qt::CaseInsensitive).isEmpty())
        return CallLoadNoQtApp;
    // Try to load
    if (!debuggeeLoadLibrary(m_manager, m_cif, m_library, errorMessage))
        return CallLoadError;
    return CallLoadOk;
}

bool CdbDumperHelper::ensureInitialized(QString *errorMessage)
{
    if (loadDebug)
        qDebug() << Q_FUNC_INFO << '\n' << m_state;

    switch (m_state) {
    case Disabled:
        *errorMessage = QLatin1String("Internal error, attempt to call disabled dumper");
        return false;
    case Initialized:
        return true;
        // Injection load failed or disabled: Try a call load.
    case NotLoaded:
    case InjectLoading:
    case InjectLoadFailed:
        // Also shows up in the log window.
        m_manager->showStatusMessage(msgLoading(m_library, false), 10000);
        switch (initCallLoad(errorMessage)) {
            case CallLoadOk:
            case CallLoadAlreadyLoaded:
                m_manager->showDebuggerOutput(LogMisc, msgLoadSucceeded(m_library, false));
                m_state = Loaded;
                break;
            case CallLoadError:
                *errorMessage = msgLoadFailed(m_library, false, *errorMessage);
                m_manager->showDebuggerOutput(LogError, *errorMessage);
                m_manager->showQtDumperLibraryWarning(*errorMessage);
                m_state = Disabled; // No message here, no point in retrying
                return false;
            case CallLoadNoQtApp:
                m_manager->showDebuggerOutput(LogMisc, QCoreApplication::translate("Debugger::Internal::CdbDumperHelper", "The debuggee does not appear to be Qt application."));
                m_state = Disabled; // No message here
                return true;
            }
        break;
    case Loaded: // Injection load succeeded, ideally
        break;
    }
    // Perform remaining initialization
    m_manager->showStatusMessage(QCoreApplication::translate("Debugger::Internal::CdbDumperHelper", "Initializing dumpers..."), 10000);
    const bool ok = initResolveSymbols(errorMessage) && initKnownTypes(errorMessage);
    if (ok) {
        m_manager->showDebuggerOutput(LogMisc, QCoreApplication::translate("Debugger::Internal::CdbDumperHelper", "Custom dumper library initialized."));
        m_manager->showDebuggerOutput(LogMisc, m_helper.toString());
        m_state = Initialized;
    } else {
        m_state = Disabled; // No message here
        *errorMessage = QCoreApplication::translate("Debugger::Internal::CdbDumperHelper", "The custom dumper library could not be initialized: %1").arg(*errorMessage);
        m_manager->showDebuggerOutput(LogMisc, *errorMessage);
        m_manager->showQtDumperLibraryWarning(*errorMessage);
    }
    return ok;
}

// Retrieve address and optionally size of a symbol.
static inline bool getSymbolAddress(CIDebugSymbols *sg,
                                    const QString &name,
                                    quint64 *address,
                                    ULONG *size /* = 0*/,
                                    QString *errorMessage)
{
    // Get address
    HRESULT hr = sg->GetOffsetByNameWide(reinterpret_cast<PCWSTR>(name.utf16()), address);
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

bool CdbDumperHelper::initResolveSymbols(QString *errorMessage)
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

// Call query protocol to retrieve known types and sizes
bool CdbDumperHelper::initKnownTypes(QString *errorMessage)
{
    const double dumperVersionRequired = 1.3;
    QByteArray output;
    QString callCmd;
    QTextStream(&callCmd) << ".call " << m_dumpObjectSymbol << "(1,0,0,0,0,0,0,0)";
    const char *outData;
    if (!callDumper(callCmd, QByteArray(), &outData, false, errorMessage)) {
        return false;
    }
    if (!m_helper.parseQuery(outData, QtDumperHelper::CdbDebugger)) {
     *errorMessage = QString::fromLatin1("Unable to parse the dumper output: '%1'").arg(QString::fromAscii(output));
    }
    if (m_helper.dumperVersion() < dumperVersionRequired) {
        *errorMessage = QtDumperHelper::msgDumperOutdated(dumperVersionRequired, m_helper.dumperVersion());
        return false;
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

bool CdbDumperHelper::callDumper(const QString &callCmd, const QByteArray &inBuffer, const char **outDataPtr,
                                 bool ignoreAccessViolation, QString *errorMessage)
{
    *outDataPtr = 0;
    CdbExceptionLoggerEventCallback exLogger(LogWarning, m_manager);
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
    // by using 'gN' (go not handled -> pass handling to dumper __try/__catch block)
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
        // If we are to ignore EXCEPTION_ACCESS_VIOLATION, check if anything
        // else occurred.
        if (ignoreAccessViolation) {
            const QList<ULONG> newExceptionCodes = exLogger.exceptionCodes().mid(oldExceptionCount);
            if (newExceptionCodes.count(EXCEPTION_ACCESS_VIOLATION) == newExceptionCodes.size())
                break;
        }
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

static inline QString msgDumpFailed(const WatchData &wd, const QString *why)
{
    return QString::fromLatin1("Unable to dump '%1' (%2): %3").arg(wd.name, wd.type, *why);
}

static inline QString msgNotHandled(const QString &type)
{
    return QString::fromLatin1("The type '%1' is not handled.").arg(type);
}

CdbDumperHelper::DumpResult CdbDumperHelper::dumpType(const WatchData &wd, bool dumpChildren,
                                                      QList<WatchData> *result, QString *errorMessage)
{
    // Check failure cache and supported types
    if (m_state == Disabled) {
        *errorMessage = QLatin1String("Dumpers are disabled");
        return DumpNotHandled;
    }
    if (m_failedTypes.contains(wd.type)) {
        *errorMessage = msgNotHandled(wd.type);
        return DumpNotHandled;
    }
    if (wd.addr.isEmpty()) {
        *errorMessage = QString::fromLatin1("Adress is missing for '%1' (%2).").arg(wd.exp, wd.type);
        return DumpNotHandled;
    }

    // Ensure types are parsed and known.
    if (!ensureInitialized(errorMessage)) {
        *errorMessage = msgDumpFailed(wd, errorMessage);
        m_manager->showDebuggerOutput(LogError, *errorMessage);
        return DumpError;
    }

    // Known type?
    const QtDumperHelper::TypeData td = m_helper.typeData(wd.type);
    if (loadDebug)
        qDebug() << "dumpType" << wd.type << td;
    if (td.type == QtDumperHelper::UnknownType) {
        *errorMessage = msgNotHandled(wd.type);
        return DumpNotHandled;
    }

    // Now evaluate
    const QString message = QCoreApplication::translate("Debugger::Internal::CdbDumperHelper",
                                                        "Querying dumpers for '%1'/'%2' (%3)").
                                                        arg(wd.name, wd.exp, wd.type);
    m_manager->showDebuggerOutput(LogMisc, message);

    const DumpExecuteResult der = executeDump(wd, td, dumpChildren, result, errorMessage);
    if (der == DumpExecuteOk)
        return DumpOk;
    // Cache types that fail due to complicated template size expressions.
    // Exceptions OTOH might occur when accessing variables that are not
    // yet initialized in a particular breakpoint. That should be ignored.
    // Also fail for complex expression that were not cached/replaced by the helper.
    if (der == DumpExecuteSizeFailed || der == DumpComplexExpressionEncountered)
        m_failedTypes.push_back(wd.type);
    // log error
    *errorMessage = msgDumpFailed(wd, errorMessage);
    m_manager->showDebuggerOutput(LogWarning, *errorMessage);
    return DumpError;
}

CdbDumperHelper::DumpExecuteResult
    CdbDumperHelper::executeDump(const WatchData &wd,
                                const QtDumperHelper::TypeData& td, bool dumpChildren,
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
            continue;
        }
        // We cannot evaluate any other expressions than 'sizeof()' ;-(
        if (!ep.isEmpty() && !ep.at(0).isDigit()) {
            *errorMessage = QString::fromLatin1("Unable to evaluate: '%1'").arg(ep);
            return DumpComplexExpressionEncountered;
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
    // Completely ignore EXCEPTION_ACCESS_VIOLATION crashes in the dumpers.
    ExceptionBlocker eb(m_cif->debugControl, EXCEPTION_ACCESS_VIOLATION, ExceptionBlocker::IgnoreException);
    if (!eb) {
        *errorMessage = eb.errorString();
        return DumpExecuteCallFailed;
    }
    if (!callDumper(callCmd, inBuffer, &outputData, true, errorMessage))
        return DumpExecuteCallFailed;
    if (!QtDumperHelper::parseValue(outputData, result)) {
        *errorMessage = QLatin1String("Parsing of value query output failed.");
        return DumpExecuteCallFailed;
    }
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
