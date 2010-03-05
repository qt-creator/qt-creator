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

#include "coreengine.h"
#include "debugeventcallbackbase.h"
#include "debugoutputbase.h"

#include <utils/winutils.h>
#include <utils/abstractprocess.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtCore/QDebug>

#define DBGHELP_TRANSLATE_TCHAR
#include <inc/Dbghelp.h>

static const char *dbgHelpDllC = "dbghelp";
static const char *dbgEngineDllC = "dbgeng";
static const char *debugCreateFuncC = "DebugCreate";

enum { debug = 0 };

static inline QString msgLibLoadFailed(const QString &lib, const QString &why)
{
    return QCoreApplication::translate("Debugger::Cdb",
                                       "Unable to load the debugger engine library '%1': %2").
            arg(lib, why);
}

static inline ULONG getInterruptTimeOutSecs(CIDebugControl *ctl)
{
    ULONG rc = 0;
    ctl->GetInterruptTimeout(&rc);
    return rc;
}

namespace CdbCore {

QString msgDebugEngineComResult(HRESULT hr)
{
    switch (hr) {
    case S_OK:
        return QLatin1String("S_OK");
    case S_FALSE:
        return QLatin1String("S_FALSE");
    case E_FAIL:
        break;
    case E_INVALIDARG:
        return QLatin1String("E_INVALIDARG");
    case E_NOINTERFACE:
        return QLatin1String("E_NOINTERFACE");
    case E_OUTOFMEMORY:
        return QLatin1String("E_OUTOFMEMORY");
    case E_UNEXPECTED:
        return QLatin1String("E_UNEXPECTED");
    case E_NOTIMPL:
        return QLatin1String("E_NOTIMPL");
    }
    if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        return QLatin1String("ERROR_ACCESS_DENIED");;
    if (hr == HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
        return QLatin1String("STATUS_CONTROL_C_EXIT");
    return QLatin1String("E_FAIL ") + Utils::winErrorMessage(HRESULT_CODE(hr));
}

QString msgComFailed(const char *func, HRESULT hr)
{
    return QString::fromLatin1("%1 failed: %2").arg(QLatin1String(func), msgDebugEngineComResult(hr));
}

QString msgDebuggerCommandFailed(const QString &command, HRESULT hr)
{
    return QString::fromLatin1("Unable to execute '%1': %2").arg(command, msgDebugEngineComResult(hr));
}

const char *msgExecutionStatusString(ULONG executionStatus)
{
    switch (executionStatus) {
    case DEBUG_STATUS_NO_CHANGE:
        return "DEBUG_STATUS_NO_CHANGE";
    case DEBUG_STATUS_GO:
        return "DEBUG_STATUS_GO";
    case DEBUG_STATUS_GO_HANDLED:
        return "DEBUG_STATUS_GO_HANDLED";
    case DEBUG_STATUS_GO_NOT_HANDLED:
        return "DEBUG_STATUS_GO_NOT_HANDLED";
    case DEBUG_STATUS_STEP_OVER:
        return "DEBUG_STATUS_STEP_OVER";
    case DEBUG_STATUS_STEP_INTO:
        return "DEBUG_STATUS_STEP_INTO";
    case DEBUG_STATUS_BREAK:
        return "DEBUG_STATUS_BREAK";
    case DEBUG_STATUS_NO_DEBUGGEE:
        return "DEBUG_STATUS_NO_DEBUGGEE";
    case DEBUG_STATUS_STEP_BRANCH:
        return "DEBUG_STATUS_STEP_BRANCH";
    case DEBUG_STATUS_IGNORE_EVENT:
        return "DEBUG_STATUS_IGNORE_EVENT";
    case DEBUG_STATUS_RESTART_REQUESTED:
        return "DEBUG_STATUS_RESTART_REQUESTED";
    case DEBUG_STATUS_REVERSE_GO:
        return "DEBUG_STATUS_REVERSE_GO";
          case DEBUG_STATUS_REVERSE_STEP_BRANCH:
        return "DEBUG_STATUS_REVERSE_STEP_BRANCH";
    case DEBUG_STATUS_REVERSE_STEP_OVER:
        return "DEBUG_STATUS_REVERSE_STEP_OVER";
    case DEBUG_STATUS_REVERSE_STEP_INTO:
        return "DEBUG_STATUS_REVERSE_STEP_INTO";
        default:
        break;
    }
    return "<Unknown execution status>";
}

static const ULONG defaultSymbolOptions = SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME |
                                          SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST |
                                          SYMOPT_AUTO_PUBLICS;

// ComInterfaces
ComInterfaces::ComInterfaces() :
    debugClient(0),
    debugControl(0),
    debugSystemObjects(0),
    debugSymbols(0),
    debugRegisters(0),
    debugDataSpaces(0)
{
}

// Thin wrapper around the 'DBEng' debugger engine shared library
// which is loaded at runtime.

class DebuggerEngineLibrary
{
public:
    DebuggerEngineLibrary();
    bool init(const QString &path, QString *dbgEngDLL, QString *errorMessage);

    inline HRESULT debugCreate(REFIID interfaceId, PVOID *interfaceHandle) const
        { return m_debugCreate(interfaceId, interfaceHandle); }

private:
    // The exported functions of the library
    typedef HRESULT (STDAPICALLTYPE *DebugCreateFunction)(REFIID, PVOID *);

    DebugCreateFunction m_debugCreate;
};

// --------- DebuggerEngineLibrary
DebuggerEngineLibrary::DebuggerEngineLibrary() :
    m_debugCreate(0)
{
}

// Build a lib name as "Path\x.dll"
static inline QString libPath(const QString &libName, const QString &path = QString())
{
    QString rc = path;
    if (!rc.isEmpty())
        rc += QDir::separator();
    rc += libName;
    rc += QLatin1String(".dll");
    return rc;
}

bool DebuggerEngineLibrary::init(const QString &path,
                                 QString *dbgEngDLL,
                                 QString *errorMessage)
{
    // Load the dependent help lib first
    const QString helpLibPath = libPath(QLatin1String(dbgHelpDllC), path);
    QLibrary helpLib(helpLibPath, 0);
    if (!helpLib.isLoaded() && !helpLib.load()) {
        *errorMessage = msgLibLoadFailed(helpLibPath, helpLib.errorString());
        return false;
    }
    // Load dbgeng lib
    const QString engineLibPath = libPath(QLatin1String(dbgEngineDllC), path);
    QLibrary lib(engineLibPath, 0);
    if (!lib.isLoaded() && !lib.load()) {
        *errorMessage = msgLibLoadFailed(engineLibPath, lib.errorString());
        return false;
    }
    *dbgEngDLL = engineLibPath;
    // Locate symbols
    void *createFunc = lib.resolve(debugCreateFuncC);
    if (!createFunc) {
        *errorMessage = QCoreApplication::translate("Debugger::Cdb", "Unable to resolve '%1' in the debugger engine library '%2'").
                        arg(QLatin1String(debugCreateFuncC), QLatin1String(dbgEngineDllC));
        return false;
    }
    m_debugCreate = static_cast<DebugCreateFunction>(createFunc);
    return true;
}

// ------ Engine
CoreEngine::CoreEngine(QObject *parent) :
    QObject(parent),
    m_watchTimer(-1)
{
}

CoreEngine::~CoreEngine()
{

    if (m_cif.debugClient) {
        m_cif.debugClient->SetOutputCallbacksWide(0);
        m_cif.debugClient->SetEventCallbacksWide(0);
        m_cif.debugClient->Release();
    }
    if (m_cif.debugControl)
        m_cif.debugControl->Release();
    if (m_cif.debugSystemObjects)
        m_cif.debugSystemObjects->Release();
    if (m_cif.debugSymbols)
        m_cif.debugSymbols->Release();
    if (m_cif.debugRegisters)
        m_cif.debugRegisters->Release();
    if (m_cif.debugDataSpaces)
        m_cif.debugDataSpaces->Release();
}

bool CoreEngine::init(const QString &dllEnginePath, QString *errorMessage)
{
    enum {  bufLen = 10240 };
    // Load the DLL
    DebuggerEngineLibrary lib;
    if (!lib.init(dllEnginePath, &m_dbengDLL, errorMessage))
        return false;
    // Initialize the COM interfaces
    HRESULT hr;
    hr = lib.debugCreate( __uuidof(IDebugClient5), reinterpret_cast<void**>(&m_cif.debugClient));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugClient5 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    hr = lib.debugCreate( __uuidof(IDebugControl4), reinterpret_cast<void**>(&m_cif.debugControl));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugControl4 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    hr = lib.debugCreate( __uuidof(IDebugSystemObjects4), reinterpret_cast<void**>(&m_cif.debugSystemObjects));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugSystemObjects4 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    hr = lib.debugCreate( __uuidof(IDebugSymbols3), reinterpret_cast<void**>(&m_cif.debugSymbols));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugSymbols3 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }
    hr = m_cif.debugSymbols->SetSymbolOptions(defaultSymbolOptions);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("SetSymbolOptions", hr);
        return false;
    }

    WCHAR buf[bufLen];
    hr = m_cif.debugSymbols->GetImagePathWide(buf, bufLen, 0);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetImagePathWide", hr);
        return false;
    }
    m_baseImagePath = QString::fromUtf16(buf);

    hr = lib.debugCreate( __uuidof(IDebugRegisters2), reinterpret_cast<void**>(&m_cif.debugRegisters));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugRegisters2 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    hr = lib.debugCreate( __uuidof(IDebugDataSpaces4), reinterpret_cast<void**>(&m_cif.debugDataSpaces));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugDataSpaces4 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    if (debug)
        qDebug() << QString::fromLatin1("CDB Initialization succeeded, interrupt time out %1s.").arg(getInterruptTimeOutSecs(m_cif.debugControl));
    return true;
}

CoreEngine::DebugOutputBasePtr
        CoreEngine::setDebugOutput(const DebugOutputBasePtr &o)
{
    const CoreEngine::DebugOutputBasePtr old = m_debugOutput;
    m_debugOutput = o;
    m_cif.debugClient->SetOutputCallbacksWide(m_debugOutput.data());
    return old;
}

CoreEngine::DebugEventCallbackBasePtr
        CoreEngine::setDebugEventCallback(const DebugEventCallbackBasePtr &e)
{
    const CoreEngine::DebugEventCallbackBasePtr old = m_debugEventCallback;
    m_debugEventCallback = e;
    m_cif.debugClient->SetEventCallbacksWide(m_debugEventCallback.data());
    return old;
}

void CoreEngine::startWatchTimer()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;

    if (m_watchTimer == -1)
        m_watchTimer = startTimer(0);
}

void CoreEngine::killWatchTimer()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;

    if (m_watchTimer != -1) {
        killTimer(m_watchTimer);
        m_watchTimer = -1;
    }
}

HRESULT CoreEngine::waitForEvent(int timeOutMS)
{
    const HRESULT hr = m_cif.debugControl->WaitForEvent(0, timeOutMS);
    if (debug)
        if (debug > 1 || hr != S_FALSE)
            qDebug() << Q_FUNC_INFO << "WaitForEvent" << timeOutMS << msgDebugEngineComResult(hr);
    return hr;
}

void CoreEngine::timerEvent(QTimerEvent* te)
{
    // Fetch away the debug events and notify if debuggee
    // stops. Note that IDebugEventCallback does not
    // cover all cases of a debuggee stopping execution
    // (such as step over,etc).
    if (te->timerId() != m_watchTimer)
        return;

    switch (waitForEvent(1)) {
        case S_OK:
            killWatchTimer();
            emit watchTimerDebugEvent();
            break;
        case S_FALSE:
        case E_PENDING:
        case E_FAIL:
            break;
        case E_UNEXPECTED: // Occurs on ExitProcess.
            killWatchTimer();
            break;
    }
}

bool CoreEngine::startDebuggerWithExecutable(const QString &workingDirectory,
                                             const QString &filename,
                                             const QStringList &args,
                                             const QStringList &envList,
                                             QString *errorMessage)
{
    DEBUG_CREATE_PROCESS_OPTIONS dbgopts;
    memset(&dbgopts, 0, sizeof(dbgopts));
    dbgopts.CreateFlags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;

    // Set image path
    const QFileInfo fi(filename);
    QString imagePath = QDir::toNativeSeparators(fi.absolutePath());
    if (!m_baseImagePath.isEmpty()) {
        imagePath += QLatin1Char(';');
        imagePath += m_baseImagePath;
    }
    HRESULT hr = m_cif.debugSymbols->SetImagePathWide(reinterpret_cast<PCWSTR>(imagePath.utf16()));
    if (FAILED(hr)) {
        *errorMessage = tr("Unable to set the image path to %1: %2").arg(imagePath, msgComFailed("SetImagePathWide", hr));
        return false;
    }

    if (debug)
        qDebug() << Q_FUNC_INFO <<'\n' << filename << imagePath;

    const QString cmd = Utils::AbstractProcess::createWinCommandline(filename, args);
    if (debug)
        qDebug() << "Starting " << cmd;
    PCWSTR env = 0;
    QByteArray envData;
    if (!envList.empty()) {
        envData = Utils::AbstractProcess::createWinEnvironment(Utils::AbstractProcess::fixWinEnvironment(envList));
        env = reinterpret_cast<PCWSTR>(envData.data());
    }
    // The working directory cannot be empty.
    PCWSTR workingDirC = 0;
    const QString workingDirN = workingDirectory.isEmpty() ? QString() : QDir::toNativeSeparators(workingDirectory);
    if (!workingDirN.isEmpty())
        workingDirC = workingDirN.utf16();
    hr = m_cif.debugClient->CreateProcess2Wide(NULL,
                                               reinterpret_cast<PWSTR>(const_cast<ushort *>(cmd.utf16())),
                                               &dbgopts, sizeof(dbgopts),
                                               workingDirC, env);
    if (FAILED(hr)) {
        *errorMessage = tr("Unable to create a process '%1': %2").arg(cmd, msgDebugEngineComResult(hr));
        return false;
    }
    return true;
}

bool CoreEngine::startAttachDebugger(qint64 pid,
                                     bool suppressInitialBreakPoint,
                                     QString *errorMessage)
{
    // Need to attrach invasively, otherwise, no notification signals
    // for for CreateProcess/ExitProcess occur.
    // Initial breakpoint occur:
    // 1) Desired: When attaching to a crashed process
    // 2) Undesired: When starting up a console process, in conjunction
    //    with the 32bit Wow-engine
    //  As of version 6.11, the flag only affects 1). 2) Still needs to be suppressed
    // by lookup at the state of the application (startup trap). However,
    // there is no startup trap when attaching to a process that has been
    // running for a while. (see notifyException).
    ULONG flags = DEBUG_ATTACH_INVASIVE_RESUME_PROCESS;
    if (suppressInitialBreakPoint)
        flags |= DEBUG_ATTACH_INVASIVE_NO_INITIAL_BREAK;
    const HRESULT hr = m_cif.debugClient->AttachProcess(NULL, pid, flags);
    if (debug)
        qDebug() << "Attaching to " << pid << " using flags" << flags << " returns " << hr;
    if (FAILED(hr)) {
        *errorMessage = tr("Attaching to a process failed for process id %1: %2").arg(pid).arg(msgDebugEngineComResult(hr));
        return false;
    }
    return true;
}

static inline QString pathString(const QStringList &s)
{  return s.join(QString(QLatin1Char(';')));  }

QStringList CoreEngine::sourcePaths() const
{
    WCHAR wszBuf[MAX_PATH];
    if (SUCCEEDED(m_cif.debugSymbols->GetSourcePathWide(wszBuf, MAX_PATH, 0)))
        return QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf)).split(QLatin1Char(';'));
    return QStringList();
}

bool CoreEngine::setSourcePaths(const QStringList &s, QString *errorMessage)
{
    const HRESULT hr = m_cif.debugSymbols->SetSourcePathWide(reinterpret_cast<PCWSTR>(pathString(s).utf16()));
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage = msgComFailed("SetSourcePathWide", hr);
        return false;
    }
    return true;
}

QStringList CoreEngine::symbolPaths() const
{
    WCHAR wszBuf[MAX_PATH];
    if (SUCCEEDED(m_cif.debugSymbols->GetSymbolPathWide(wszBuf, MAX_PATH, 0)))
        return QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf)).split(QLatin1Char(';'));
    return QStringList();
}

bool CoreEngine::setSymbolPaths(const QStringList &s, QString *errorMessage)
{
    const HRESULT hr = m_cif.debugSymbols->SetSymbolPathWide(reinterpret_cast<PCWSTR>(pathString(s).utf16()));
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage = msgComFailed("SetSymbolPathWide", hr);
        return false;
    }
    return true;
}

bool CoreEngine::isVerboseSymbolLoading() const
{
    ULONG opts;
    const HRESULT hr = m_cif.debugSymbols->GetSymbolOptions(&opts);
    return SUCCEEDED(hr) && (opts & SYMOPT_DEBUG);
}

bool CoreEngine::setVerboseSymbolLoading(bool newValue)
{
    ULONG opts;
    HRESULT hr = m_cif.debugSymbols->GetSymbolOptions(&opts);
    if (FAILED(hr)) {
        qWarning("%s", qPrintable(msgComFailed("GetSymbolOptions", hr)));
        return false;
    }
    const bool isVerbose = (opts & SYMOPT_DEBUG);
    if (isVerbose == newValue)
        return true;
    if (newValue) {
        opts |= SYMOPT_DEBUG;
    } else {
        opts &= ~SYMOPT_DEBUG;
    }
    hr = m_cif.debugSymbols->SetSymbolOptions(opts);
    if (FAILED(hr)) {
        qWarning("%s", qPrintable(msgComFailed("SetSymbolOptions", hr)));
        return false;
    }
    return true;
}

bool CoreEngine::executeDebuggerCommand(const QString &command, QString *errorMessage)
{
        // output to all clients, else we do not see anything
    const HRESULT hr = m_cif.debugControl->ExecuteWide(DEBUG_OUTCTL_ALL_CLIENTS, reinterpret_cast<PCWSTR>(command.utf16()), 0);
    if (debug)
        qDebug() << "executeDebuggerCommand" << command << SUCCEEDED(hr);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to execute '%1': %2").
                        arg(command, msgDebugEngineComResult(hr));
        return false;
    }
    return true;
}

// Those should not fail
CoreEngine::ExpressionSyntax CoreEngine::expressionSyntax() const
{
    ULONG expressionSyntax = DEBUG_EXPR_MASM;
    const HRESULT hr = m_cif.debugControl->GetExpressionSyntax(&expressionSyntax);
    if (FAILED(hr))
        qWarning("Unable to retrieve expression syntax: %s", qPrintable(msgComFailed("GetExpressionSyntax", hr)));
    return expressionSyntax == DEBUG_EXPR_MASM ? AssemblerExpressionSyntax : CppExpressionSyntax;
}

CoreEngine::ExpressionSyntax CoreEngine::setExpressionSyntax(ExpressionSyntax es)
{
    const ExpressionSyntax old = expressionSyntax();
    if (old != es) {
        if (debug)
            qDebug() << "Setting expression syntax" << es;
        const HRESULT hr = m_cif.debugControl->SetExpressionSyntax(es == AssemblerExpressionSyntax ? DEBUG_EXPR_MASM : DEBUG_EXPR_CPLUSPLUS);
        if (FAILED(hr))
            qWarning("Unable to set expression syntax: %s", qPrintable(msgComFailed("SetExpressionSyntax", hr)));
    }
    return old;
}

CoreEngine::CodeLevel CoreEngine::codeLevel() const
{
    ULONG currentCodeLevel = DEBUG_LEVEL_ASSEMBLY;
    HRESULT hr = m_cif.debugControl->GetCodeLevel(&currentCodeLevel);
    if (FAILED(hr)) {
        qWarning("Cannot determine code level: %s", qPrintable(msgComFailed("GetCodeLevel", hr)));
    }
    return currentCodeLevel == DEBUG_LEVEL_ASSEMBLY ? CodeLevelAssembly : CodeLevelSource;
}

CoreEngine::CodeLevel CoreEngine::setCodeLevel(CodeLevel cl)
{
    const CodeLevel old = codeLevel();
    if (old != cl) {
        if (debug)
            qDebug() << "Setting code level" << cl;
        const HRESULT hr = m_cif.debugControl->SetCodeLevel(cl == CodeLevelAssembly ? DEBUG_LEVEL_ASSEMBLY : DEBUG_LEVEL_SOURCE);
        if (FAILED(hr))
            qWarning("Unable to set code level: %s", qPrintable(msgComFailed("SetCodeLevel", hr)));
    }
    return old;
}

bool CoreEngine::evaluateExpression(const QString &expression,
                                        QString *value,
                                        QString *type,
                                        QString *errorMessage)
{
    DEBUG_VALUE debugValue;
    if (!evaluateExpression(expression, &debugValue, errorMessage))
        return false;
    *value = debugValueToString(debugValue, type, 10, m_cif.debugControl);
    return true;
}

bool CoreEngine::evaluateExpression(const QString &expression,
                                    DEBUG_VALUE *debugValue,
                                    QString *errorMessage)
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << expression;

    memset(debugValue, 0, sizeof(DEBUG_VALUE));
    // Use CPP syntax, original syntax must be restored, else setting breakpoints will fail.
    SyntaxSetter syntaxSetter(this, CppExpressionSyntax);
    Q_UNUSED(syntaxSetter)
    ULONG errorPosition = 0;
    const HRESULT hr = m_cif.debugControl->EvaluateWide(reinterpret_cast<PCWSTR>(expression.utf16()),
                                                        DEBUG_VALUE_INVALID, debugValue,
                                                        &errorPosition);
    if (FAILED(hr)) {
        if (HRESULT_CODE(hr) == 517) {
            *errorMessage = QString::fromLatin1("Unable to evaluate '%1': Expression out of scope.").
                            arg(expression);
        } else {
            *errorMessage = QString::fromLatin1("Unable to evaluate '%1': Error at %2: %3").
                            arg(expression).arg(errorPosition).arg(msgDebugEngineComResult(hr));
        }
        return false;
    }
    return true;
}

ULONG CoreEngine::executionStatus() const
{
    ULONG ex = DEBUG_STATUS_NO_CHANGE;
    const HRESULT hr = m_cif.debugControl->GetExecutionStatus(&ex);
    if (FAILED(hr))
        qWarning("Cannot determine execution status: %s", qPrintable(msgComFailed("GetExecutionStatus", hr)));
    return ex;
}

bool CoreEngine::setExecutionStatus(ULONG ex, QString *errorMessage)
{
    const HRESULT hr = m_cif.debugControl->SetExecutionStatus(ex);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Cannot set execution status to %1: %2")
                        .arg(QLatin1String(msgExecutionStatusString(ex)), msgComFailed("SetExecutionStatus", hr));
        return false;
    }
    return true;
}

static inline void appendError(const QString &what, QString *appendableErrorMessage)
{
    if (!appendableErrorMessage->isEmpty())
        appendableErrorMessage->append(QLatin1Char('\n'));
    appendableErrorMessage->append(what);
}

bool CoreEngine::detachCurrentProcess(QString *appendableErrorMessage)
{
    const HRESULT hr = m_cif.debugClient->DetachCurrentProcess();
    if (FAILED(hr)) {
        appendError(msgComFailed("DetachCurrentProcess", hr), appendableErrorMessage);
        return false;
    }
    return true;
}

bool CoreEngine::terminateCurrentProcess(QString *appendableErrorMessage)
{
    const HRESULT hr = m_cif.debugClient->TerminateCurrentProcess();
    if (FAILED(hr)) {
        appendError(msgComFailed("TerminateCurrentProcess", hr), appendableErrorMessage);
        return false;
    }
    return true;
}

bool CoreEngine::terminateProcesses(QString *appendableErrorMessage)
{
    const HRESULT hr = m_cif.debugClient->TerminateProcesses();
    if (FAILED(hr)) {
        appendError(msgComFailed("TerminateProcesses", hr), appendableErrorMessage);
        return false;
    }
    return true;
}

bool CoreEngine::endSession(QString *appendableErrorMessage)
{
    const HRESULT hr = m_cif.debugClient->EndSession(DEBUG_END_PASSIVE);
    if (FAILED(hr)) {
        appendError(msgComFailed("EndSession", hr), appendableErrorMessage);
        return false;
    }
    return true;
}

bool CoreEngine::allocDebuggeeMemory(int size, ULONG64 *address, QString *errorMessage)
{
    *address = 0;
    const QString allocCmd = QLatin1String(".dvalloc ") + QString::number(size);

    QSharedPointer<StringOutputHandler> outputHandler(new StringOutputHandler);
    OutputRedirector redir(this, outputHandler);
    Q_UNUSED(redir)
    if (!executeDebuggerCommand(allocCmd, errorMessage))
        return false;
   // "Allocated 1000 bytes starting at 003a0000" .. hopefully never localized
    bool ok = false;
    const QString output = outputHandler->result();
    const int lastBlank = output.lastIndexOf(QLatin1Char(' '));
    if (lastBlank != -1) {
        const qint64 addri = output.mid(lastBlank + 1).toLongLong(&ok, 16);
        if (ok)
            *address = addri;
    }
     if (debug > 1)
        qDebug() << Q_FUNC_INFO << '\n' << output << *address << ok;
    if (!ok) {
        *errorMessage = QString::fromLatin1("Failed to parse output '%1'").arg(output);
        return false;
    }
    return true;
}

// Alloc an AscII string in debuggee
bool CoreEngine::createDebuggeeAscIIString(const QString &s,
                                           ULONG64 *address,
                                           QString *errorMessage)
{
    QByteArray sAsciiData = s.toLocal8Bit();
    sAsciiData += '\0';
    if (!allocDebuggeeMemory(sAsciiData.size(), address, errorMessage))
        return false;
    const HRESULT hr = m_cif.debugDataSpaces->WriteVirtual(*address, sAsciiData.data(), sAsciiData.size(), 0);
    if (FAILED(hr)) {
        *errorMessage= msgComFailed("WriteVirtual", hr);
        return false;
    }
    return true;
}

// Write to debuggee memory in chunks
bool CoreEngine::writeToDebuggee(const QByteArray &buffer, quint64 address, QString *errorMessage)
{
    char *ptr = const_cast<char*>(buffer.data());
    ULONG bytesToWrite = buffer.size();
    while (bytesToWrite > 0) {
        ULONG bytesWritten = 0;
        const HRESULT hr = m_cif.debugDataSpaces->WriteVirtual(address, ptr, bytesToWrite, &bytesWritten);
        if (FAILED(hr)) {
            *errorMessage = msgComFailed("WriteVirtual", hr);
            return false;
        }
        bytesToWrite -= bytesWritten;
        ptr += bytesWritten;
    }
    return true;
}

bool CoreEngine::dissassemble(ULONG64 offset,
                              unsigned long beforeLines,
                              unsigned long afterLines,
                              QString *target,
                              QString *errorMessage)
{
    const ULONG flags = DEBUG_DISASM_MATCHING_SYMBOLS|DEBUG_DISASM_SOURCE_LINE_NUMBER|DEBUG_DISASM_SOURCE_FILE_NAME;
    // Catch the output by temporarily setting another handler.
    // We use the method that outputs to the output handler as it
    // conveniently provides the 'beforeLines' context (stepping back
    // in assembler code). We build a complete string first as line breaks
    // may occur in-between messages.
    QSharedPointer<StringOutputHandler> outputHandler(new StringOutputHandler);
    OutputRedirector redir(this, outputHandler);
    // For some reason, we need to output to "all clients"
    const HRESULT hr =  m_cif.debugControl->OutputDisassemblyLines(DEBUG_OUTCTL_ALL_CLIENTS,
                                                                   beforeLines, beforeLines + afterLines,
                                                                   offset, flags, 0, 0, 0, 0);
    if (FAILED(hr)) {
        *errorMessage= QString::fromLatin1("Unable to dissamble at 0x%1: %2").
                       arg(offset, 0, 16).arg(msgComFailed("OutputDisassemblyLines", hr));
        return false;
    }
    *target = outputHandler->result();
    return true;
}

quint64 CoreEngine::getSourceLineAddress(const QString &file,
                                         int line,
                                         QString *errorMessage) const
{
    ULONG64 rc = 0;
    const HRESULT hr = m_cif.debugSymbols->GetOffsetByLineWide(line,
                                                               const_cast<ushort*>(file.utf16()),
                                                               &rc);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to determine address of %1:%2 : %3").
                        arg(file).arg(line).arg(msgComFailed("GetOffsetByLine", hr));
        return 0;
    }
    return rc;
}

bool CoreEngine::autoDetectPath(QString *outPath,
                                QStringList *checkedDirectories /* = 0 */)
{
    // Look for $ProgramFiles/"Debugging Tools For Windows <bit-idy>" and its
    // " (x86)", " (x64)" variations. Qt Creator needs 64/32 bit depending
    // on how it was built.
#ifdef Q_OS_WIN64
    static const char *postFixes[] = {" (x64)", " 64-bit" };
#else
    static const char *postFixes[] = { " (x86)", " (x32)" };
#endif

    outPath->clear();
    const QByteArray programDirB = qgetenv("ProgramFiles");
    if (programDirB.isEmpty())
        return false;

    const QString programDir = QString::fromLocal8Bit(programDirB) + QDir::separator();
    const QString installDir = QLatin1String("Debugging Tools For Windows");

    QString path = programDir + installDir;
    if (checkedDirectories)
        checkedDirectories->push_back(path);
    if (QFileInfo(path).isDir()) {
        *outPath = QDir::toNativeSeparators(path);
        return true;
    }
    const int rootLength = path.size();
    for (int i = 0; i < sizeof(postFixes)/sizeof(const char*); i++) {
        path.truncate(rootLength);
        path += QLatin1String(postFixes[i]);
        if (checkedDirectories)
            checkedDirectories->push_back(path);
        if (QFileInfo(path).isDir()) {
            *outPath = QDir::toNativeSeparators(path);
            return true;
        }
    }
    return false;
}

bool CoreEngine::debugBreakProcess(HANDLE hProcess, QString *errorMessage)
{
    const bool rc = DebugBreakProcess(hProcess);
    if (!rc)
        *errorMessage = QString::fromLatin1("DebugBreakProcess failed: %1").arg(Utils::winErrorMessage(GetLastError()));
    return rc;
}

bool CoreEngine::setInterrupt(QString *errorMessage)
{
    const HRESULT hr = interfaces().debugControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE|DEBUG_INTERRUPT_EXIT);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to interrupt debuggee after %1s: %2").
                        arg(getInterruptTimeOutSecs(interfaces().debugControl)).arg(msgComFailed("SetInterrupt", hr));
        return false;
    }
    return true;
}

// ------------- DEBUG_VALUE formatting helpers

// format an array of integers as "0x323, 0x2322, ..."
template <class Integer>
static QString formatArrayHelper(const Integer *array, int size, int base = 10)
{
    QString rc;
    const QString hexPrefix = QLatin1String("0x");
    const QString separator = QLatin1String(", ");
    const bool hex = base == 16;
    for (int i = 0; i < size; i++) {
        if (i)
            rc += separator;
        if (hex)
            rc += hexPrefix;
        rc += QString::number(array[i], base);
    }
    return rc;
}

QString hexFormatArray(const unsigned short *array, int size)
{
    return formatArrayHelper(array, size, 16);
}

// Helper to format an integer with
// a hex prefix in case base = 16
template <class Integer>
        inline QString formatInteger(Integer value, int base)
{
    QString rc;
    if (base == 16)
        rc = QLatin1String("0x");
    rc += QString::number(value, base);
    return rc;
}

QString debugValueToString(const DEBUG_VALUE &dv,
                           QString *qType /* =0 */,
                           int integerBase,
                           CIDebugControl *ctl /* =0 */)
{
    switch (dv.Type) {
    case DEBUG_VALUE_INT8:
        if (qType)
            *qType = QLatin1String("char");
        return formatInteger(dv.I8, integerBase);
    case DEBUG_VALUE_INT16:
        if (qType)
            *qType = QLatin1String("short");
        return formatInteger(static_cast<short>(dv.I16), integerBase);
    case DEBUG_VALUE_INT32:
        if (qType)
            *qType = QLatin1String("long");
        return formatInteger(static_cast<long>(dv.I32), integerBase);
    case DEBUG_VALUE_INT64:
        if (qType)
            *qType = QLatin1String("long long");
        return formatInteger(static_cast<long long>(dv.I64), integerBase);
    case DEBUG_VALUE_FLOAT32:
        if (qType)
            *qType = QLatin1String("float");
        return QString::number(dv.F32);
    case DEBUG_VALUE_FLOAT64:
        if (qType)
            *qType = QLatin1String("double");
        return QString::number(dv.F64);
    case DEBUG_VALUE_FLOAT80:
    case DEBUG_VALUE_FLOAT128: { // Convert to double
            DEBUG_VALUE doubleValue;
            double d = 0.0;
            if (ctl && SUCCEEDED(ctl->CoerceValue(const_cast<DEBUG_VALUE*>(&dv), DEBUG_VALUE_FLOAT64, &doubleValue))) {
                d = dv.F64;
            } else {
                qWarning("Unable to convert DEBUG_VALUE_FLOAT80/DEBUG_VALUE_FLOAT128 values");
            }
            if (qType)
                *qType = QLatin1String(dv.Type == DEBUG_VALUE_FLOAT80 ? "80bit-float" : "128bit-float");
            return QString::number(d);
        }
    case DEBUG_VALUE_VECTOR64: {
            if (qType)
                *qType = QLatin1String("64bit-vector");
            QString rc = QLatin1String("bytes: ");
            rc += formatArrayHelper(dv.VI8, 8, integerBase);
            rc += QLatin1String(" long: ");
            rc += formatArrayHelper(dv.VI32, 2, integerBase);
            return rc;
        }
    case DEBUG_VALUE_VECTOR128: {
            if (qType)
                *qType = QLatin1String("128bit-vector");
            QString rc = QLatin1String("bytes: ");
            rc += formatArrayHelper(dv.VI8, 16, integerBase);
            rc += QLatin1String(" long long: ");
            rc += formatArrayHelper(dv.VI64, 2, integerBase);
            return rc;
        }
    }
    if (qType)
        *qType = QString::fromLatin1("Unknown type #%1:").arg(dv.Type);
    return formatArrayHelper(dv.RawBytes, 24, integerBase);
}

bool debugValueToInteger(const DEBUG_VALUE &dv, qint64 *value)
{
    *value = 0;
    switch (dv.Type) {
    case DEBUG_VALUE_INT8:
        *value = dv.I8;
        return true;
    case DEBUG_VALUE_INT16:
        *value = static_cast<short>(dv.I16);
        return true;
    case DEBUG_VALUE_INT32:
        *value = static_cast<long>(dv.I32);
        return true;
    case DEBUG_VALUE_INT64:
        *value = static_cast<long long>(dv.I64);
        return true;
    default:
        break;
    }
    return false;
}

} // namespace CdbCore
