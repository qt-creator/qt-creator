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

#include "cdbdebugengine.h"
#include "cdbdebugengine_p.h"
#include "cdbstacktracecontext.h"
#include "cdbstackframecontext.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbbreakpoint.h"
#include "cdbmodules.h"
#include "cdbassembler.h"
#include "cdboptionspage.h"
#include "cdboptions.h"
#include "cdbexceptionutils.h"
#include "debuggeragents.h"

#include "debuggeractions.h"
#include "debuggermanager.h"
#include "breakhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "registerhandler.h"
#include "moduleshandler.h"
#include "watchutils.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/winutils.h>
#include <utils/consoleprocess.h>
#include <utils/fancymainwindow.h>
#include <texteditor/itexteditor.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QTimerEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtCore/QCoreApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QApplication>
#include <QtGui/QToolTip>

#define DBGHELP_TRANSLATE_TCHAR
#include <inc/Dbghelp.h>

static const char *dbgHelpDllC = "dbghelp";
static const char *dbgEngineDllC = "dbgeng";
static const char *debugCreateFuncC = "DebugCreate";

static const char *localSymbolRootC = "local";

namespace Debugger {
namespace Internal {

typedef QList<WatchData> WatchList;

// ----- Message helpers

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

static QString msgStackIndexOutOfRange(int idx, int size)
{
    return QString::fromLatin1("Frame index %1 out of range (%2).").arg(idx).arg(size);
}

QString msgComFailed(const char *func, HRESULT hr)
{
    return QString::fromLatin1("%1 failed: %2").arg(QLatin1String(func), msgDebugEngineComResult(hr));
}

QString msgDebuggerCommandFailed(const QString &command, HRESULT hr)
{
    return QString::fromLatin1("Unable to execute '%1': %2").arg(command, msgDebugEngineComResult(hr));
}

static const char *msgNoStackTraceC = "Internal error: no stack trace present.";

static inline QString msgLibLoadFailed(const QString &lib, const QString &why)
{
    return CdbDebugEngine::tr("Unable to load the debugger engine library '%1': %2").
            arg(lib, why);
}

// Format function failure message. Pass in Q_FUNC_INFO
static QString msgFunctionFailed(const char *func, const QString &why)
{
    // Strip a "cdecl_ int namespace1::class::foo(int bar)" as
    // returned by Q_FUNC_INFO down to "foo"
    QString function = QLatin1String(func);
    const int firstParentPos = function.indexOf(QLatin1Char('('));
    if (firstParentPos != -1)
        function.truncate(firstParentPos);
    const int classSepPos = function.lastIndexOf(QLatin1String("::"));
    if (classSepPos != -1)
        function.remove(0, classSepPos + 2);
   //: Function call failed
   return CdbDebugEngine::tr("The function \"%1()\" failed: %2").arg(function, why);
}

// ----- Engine helpers

static inline ULONG getInterruptTimeOutSecs(CIDebugControl *ctl)
{
    ULONG rc = 0;
    ctl->GetInterruptTimeout(&rc);
    return rc;
}

bool getExecutionStatus(CIDebugControl *ctl,
                        ULONG *executionStatus,
                        QString *errorMessage /* = 0 */)
{
    const HRESULT hr = ctl->GetExecutionStatus(executionStatus);
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage = msgComFailed("GetExecutionStatus", hr);
        return false;
    }
    return true;
}

const char *executionStatusString(ULONG executionStatus)
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

// Debug convenience
const char *executionStatusString(CIDebugControl *ctl)
{
    ULONG executionStatus;
    if (getExecutionStatus(ctl, &executionStatus))
        return executionStatusString(executionStatus);
    return "<failed>";
}

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
        *errorMessage = CdbDebugEngine::tr("Unable to resolve '%1' in the debugger engine library '%2'").
                        arg(QLatin1String(debugCreateFuncC), QLatin1String(dbgEngineDllC));
        return false;
    }
    m_debugCreate = static_cast<DebugCreateFunction>(createFunc);
    return true;
}

// ----- SyntaxSetter
SyntaxSetter::SyntaxSetter(CIDebugControl *ctl, ULONG desiredSyntax) :
    m_desiredSyntax(desiredSyntax),
    m_ctl(ctl)
{
    m_ctl->GetExpressionSyntax(&m_oldSyntax);
    if (m_oldSyntax != m_desiredSyntax)
        m_ctl->SetExpressionSyntax(m_desiredSyntax);
}

SyntaxSetter::~SyntaxSetter()
{
    if (m_oldSyntax != m_desiredSyntax)
        m_ctl->SetExpressionSyntax(m_oldSyntax);
}

// CdbComInterfaces
CdbComInterfaces::CdbComInterfaces() :
    debugClient(0),
    debugControl(0),
    debugSystemObjects(0),
    debugSymbols(0),
    debugRegisters(0),
    debugDataSpaces(0)
{
}

// --- CdbDebugEnginePrivate

CdbDebugEnginePrivate::CdbDebugEnginePrivate(DebuggerManager *manager,
                                             const QSharedPointer<CdbOptions> &options,
                                             CdbDebugEngine* engine) :
    m_options(options),
    m_hDebuggeeProcess(0),
    m_hDebuggeeThread(0),
    m_breakEventMode(BreakEventHandle),
    m_dumper(new CdbDumperHelper(manager, &m_cif)),
    m_currentThreadId(-1),
    m_eventThreadId(-1),
    m_interruptArticifialThreadId(-1),
    m_ignoreInitialBreakPoint(false),
    m_interrupted(false),    
    m_watchTimer(-1),
    m_debugEventCallBack(engine),
    m_engine(engine),
    m_currentStackTrace(0),
    m_firstActivatedFrame(true),
    m_inferiorStartupComplete(false),
    m_mode(AttachCore)
{
}

bool CdbDebugEnginePrivate::init(QString *errorMessage)
{
    enum {  bufLen = 10240 };
    // Load the DLL
    DebuggerEngineLibrary lib;
    if (!lib.init(m_options->path, &m_dbengDLL, errorMessage))
        return false;
    // Initialize the COM interfaces
    HRESULT hr;
    hr = lib.debugCreate( __uuidof(IDebugClient5), reinterpret_cast<void**>(&m_cif.debugClient));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugClient5 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    m_cif.debugClient->SetOutputCallbacksWide(&m_debugOutputCallBack);
    m_cif.debugClient->SetEventCallbacksWide(&m_debugEventCallBack);

    hr = lib.debugCreate( __uuidof(IDebugControl4), reinterpret_cast<void**>(&m_cif.debugControl));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugControl4 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    setCodeLevel();

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

    if (debugCDB)
        qDebug() << QString::fromLatin1("CDB Initialization succeeded, interrupt time out %1s.").arg(getInterruptTimeOutSecs(m_cif.debugControl));
    return true;
}

IDebuggerEngine *CdbDebugEngine::create(Debugger::DebuggerManager *manager,
                                        const QSharedPointer<CdbOptions> &options,
                                        QString *errorMessage)
{
    CdbDebugEngine *rc = new CdbDebugEngine(manager, options);
    if (rc->m_d->init(errorMessage)) {
        rc->syncDebuggerPaths();
        return rc;
    }
    delete rc;
    return 0;
}

// Adapt code level setting to the setting of the action.
static inline const char *codeLevelName(ULONG level)
{
    return level == DEBUG_LEVEL_ASSEMBLY ? "assembly" : "source";
}

bool CdbDebugEnginePrivate::setCodeLevel()
{
    const ULONG codeLevel = theDebuggerBoolSetting(OperateByInstruction) ?
                            DEBUG_LEVEL_ASSEMBLY : DEBUG_LEVEL_SOURCE;
    ULONG currentCodeLevel = DEBUG_LEVEL_ASSEMBLY;
    HRESULT hr = m_cif.debugControl->GetCodeLevel(&currentCodeLevel);
    if (FAILED(hr)) {
        m_engine->warning(QString::fromLatin1("Cannot determine code level: %1").arg(msgComFailed("GetCodeLevel", hr)));
        return true;
    }
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\nSetting code level to " << codeLevelName(codeLevel) << " (was" << codeLevelName(currentCodeLevel) << ')';
    if (currentCodeLevel == codeLevel)
        return false;
    hr = m_cif.debugControl->SetCodeLevel(codeLevel);
    if (FAILED(hr)) {
        m_engine->warning(QString::fromLatin1("Cannot set code level: %1").arg(msgComFailed("SetCodeLevel", hr)));
        return false;
    }
    return true;
}

CdbDebugEnginePrivate::~CdbDebugEnginePrivate()
{
    cleanStackTrace();
    if (m_cif.debugClient)
        m_cif.debugClient->Release();
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

DebuggerManager *CdbDebugEnginePrivate::manager() const
{
    return m_engine->manager();
}

void CdbDebugEnginePrivate::clearForRun()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    m_breakEventMode = BreakEventHandle;
    m_eventThreadId = m_interruptArticifialThreadId = -1;
    m_interrupted = false;
    cleanStackTrace();
}

void CdbDebugEnginePrivate::cleanStackTrace()
{
    if (m_currentStackTrace) {
        delete m_currentStackTrace;
        m_currentStackTrace = 0;
    }
    m_firstActivatedFrame = false;
    m_editorToolTipCache.clear();
}

CdbDebugEngine::CdbDebugEngine(DebuggerManager *manager, const QSharedPointer<CdbOptions> &options) :
    IDebuggerEngine(manager),
    m_d(new CdbDebugEnginePrivate(manager, options, this))
{
    m_d->m_consoleStubProc.setMode(Utils::ConsoleProcess::Suspend);
    connect(&m_d->m_consoleStubProc, SIGNAL(processError(QString)),
            this, SLOT(slotConsoleStubError(QString)));
    connect(&m_d->m_consoleStubProc, SIGNAL(processStarted()),
            this, SLOT(slotConsoleStubStarted()));
    connect(&m_d->m_consoleStubProc, SIGNAL(wrapperStopped()),
            this, SLOT(slotConsoleStubTerminated()));
    connect(&m_d->m_debugOutputCallBack, SIGNAL(debuggerOutput(int,QString)),
            manager, SLOT(showDebuggerOutput(int,QString)));
    connect(&m_d->m_debugOutputCallBack, SIGNAL(debuggerInputPrompt(int,QString)),
            manager, SLOT(showDebuggerInput(int,QString)));
    connect(&m_d->m_debugOutputCallBack, SIGNAL(debuggeeOutput(QString)),
            manager, SLOT(showApplicationOutput(QString)));
    connect(&m_d->m_debugOutputCallBack, SIGNAL(debuggeeInputPrompt(QString)),
            manager, SLOT(showApplicationOutput(QString)));
}

CdbDebugEngine::~CdbDebugEngine()
{
    delete m_d;
}

void CdbDebugEngine::setState(DebuggerState state, const char *func, int line)
{
    if (debugCDB)
        qDebug() << "setState(" << state << ") at " << func << ':' << line;
    IDebuggerEngine::setState(state);
}

void CdbDebugEngine::startWatchTimer()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    if (m_d->m_watchTimer == -1)
        m_d->m_watchTimer = startTimer(0);
}

void CdbDebugEngine::killWatchTimer()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    if (m_d->m_watchTimer != -1) {
        killTimer(m_d->m_watchTimer);
        m_d->m_watchTimer = -1;
    }
}

void CdbDebugEngine::shutdown()
{
    exitDebugger();
}

QString CdbDebugEngine::editorToolTip(const QString &exp, const QString &function)
{
    // Figure the editor tooltip. Ask the frame context of the
    // function if it is a local variable it knows. If that is not
    // the case, try to evaluate via debugger
    QString errorMessage;
    QString rc;
    // Find the frame of the function if there is any
    CdbStackFrameContext *frame = 0;
    if (m_d->m_currentStackTrace &&  !function.isEmpty()) {
        const int frameIndex = m_d->m_currentStackTrace->indexOf(function);
        if (frameIndex != -1)
            frame = m_d->m_currentStackTrace->frameContextAt(frameIndex, &errorMessage);
    }
    if (frame && frame->editorToolTip(QLatin1String("local.") + exp, &rc, &errorMessage))
        return rc;
    // No function/symbol context found, try to evaluate in current context.
    // Do not append type as this will mostly be 'long long' for integers, etc.
    QString type;
    if (!evaluateExpression(exp, &rc, &type, &errorMessage))
        return QString();
    return rc;
}

void CdbDebugEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    typedef CdbDebugEnginePrivate::EditorToolTipCache EditorToolTipCache;
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n' << cursorPos;
    // Need a stopped debuggee and a cpp file
    if (!m_d->m_hDebuggeeProcess || m_d->isDebuggeeRunning())
        return;
    if (!isCppEditor(editor))
        return;
    // Determine expression and function
    QString toolTip;
    do {
        int line;
        int column;
        QString function;
        const QString exp = cppExpressionAt(editor, cursorPos, &line, &column, &function);
        if (function.isEmpty() || exp.isEmpty())
            break;
        // Check cache (key containing function) or try to figure out expression
        QString cacheKey = function;
        cacheKey += QLatin1Char('@');
        cacheKey += exp;
        const EditorToolTipCache::const_iterator cit = m_d->m_editorToolTipCache.constFind(cacheKey);
        if (cit != m_d->m_editorToolTipCache.constEnd()) {
            toolTip = cit.value();
        } else {
            toolTip = editorToolTip(exp, function);
            if (!toolTip.isEmpty())
                m_d->m_editorToolTipCache.insert(cacheKey, toolTip);
        }
    } while (false);
    // Display
    QToolTip::hideText();
    if (!toolTip.isEmpty())
        QToolTip::showText(mousePos, toolTip);
}

void CdbDebugEnginePrivate::clearDisplay()
{
    manager()->threadsHandler()->removeAll();
    manager()->modulesHandler()->removeAll();
    manager()->registerHandler()->removeAll();
}

void CdbDebugEnginePrivate::checkVersion()
{
    static bool versionNotChecked = true;
    // Check for version 6.11 (extended expression syntax)
    if (versionNotChecked) {
        versionNotChecked = false;
        // Get engine DLL version
        QString errorMessage;
        const QString version = Utils::winGetDLLVersion(Utils::WinDLLProductVersion, m_dbengDLL, &errorMessage);
        if (version.isEmpty()) {
            qWarning("%s\n", qPrintable(errorMessage));
            return;
        }
        // Compare
        const double minVersion = 6.11;
        manager()->showDebuggerOutput(LogMisc, CdbDebugEngine::tr("Version: %1").arg(version));
        if (version.toDouble() <  minVersion) {
            const QString msg = CdbDebugEngine::tr(
                    "<html>The installed version of the <i>Debugging Tools for Windows</i> (%1) "
                    "is rather old. Upgrading to version %2 is recommended "
                    "for the proper display of Qt's data types.</html>").arg(version).arg(minVersion);
            Core::ICore::instance()->showWarningWithOptions(CdbDebugEngine::tr("Debugger"), msg, QString(),
                                                            QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY),
                                                            CdbOptionsPage::settingsId());
        }
    }
}

void CdbDebugEngine::startDebugger(const QSharedPointer<DebuggerStartParameters> &sp)
{
    if (debugCDBExecution)
        qDebug() << "startDebugger" << *sp;
    setState(AdapterStarting, Q_FUNC_INFO, __LINE__);
    m_d->checkVersion();
    if (m_d->m_hDebuggeeProcess) {
        warning(QLatin1String("Internal error: Attempt to start debugger while another process is being debugged."));
        setState(AdapterStartFailed, Q_FUNC_INFO, __LINE__);
        emit startFailed();
    }
    m_d->clearDisplay();
    m_d->m_inferiorStartupComplete = false;
    setState(AdapterStarted, Q_FUNC_INFO, __LINE__);

    const DebuggerStartMode mode = sp->startMode;
    // Figure out dumper. @TODO: same in gdb...
    const QString dumperLibName = QDir::toNativeSeparators(manager()->qtDumperLibraryName());
    bool dumperEnabled = mode != AttachCore
                         && mode != AttachCrashedExternal
                         && manager()->qtDumperLibraryEnabled();
    if (dumperEnabled) {
        const QFileInfo fi(dumperLibName);
        if (!fi.isFile()) {
            const QStringList &locations = manager()->qtDumperLibraryLocations();
            const QString loc = locations.join(QLatin1String(", "));
            const QString msg = tr("The dumper library was not found at %1.").arg(loc);
            manager()->showQtDumperLibraryWarning(msg);
            dumperEnabled = false;
        }
    }
    m_d->m_dumper->reset(dumperLibName, dumperEnabled);

    setState(InferiorStarting, Q_FUNC_INFO, __LINE__);
    manager()->showStatusMessage("Starting Debugger", -1);

    QString errorMessage;
    bool rc = false;
    bool needWatchTimer = false;
    m_d->clearForRun();
    m_d->setCodeLevel();
    m_d->m_ignoreInitialBreakPoint = false;
    switch (mode) {
    case AttachExternal:
    case AttachCrashedExternal:
        rc = startAttachDebugger(sp->attachPID, mode, &errorMessage);
        needWatchTimer = true; // Fetch away module load, etc. even if crashed
        break;
    case StartInternal:
    case StartExternal:
        if (sp->useTerminal) {
            // Attaching to console processes triggers an initial breakpoint, which we do not want
            m_d->m_ignoreInitialBreakPoint = true;
            // Launch console stub and wait for its startup
            m_d->m_consoleStubProc.stop(); // We leave the console open, so recycle it now.
            m_d->m_consoleStubProc.setWorkingDirectory(sp->workingDir);
            m_d->m_consoleStubProc.setEnvironment(sp->environment);
            rc = m_d->m_consoleStubProc.start(sp->executable, sp->processArgs);
            if (!rc)
                errorMessage = tr("The console stub process was unable to start '%1'.").arg(sp->executable);
            // continues in slotConsoleStubStarted()...
        } else {
            needWatchTimer = true;
            rc = startDebuggerWithExecutable(mode, &errorMessage);
        }
        break;
    case AttachCore:
        errorMessage = tr("Attaching to core files is not supported!");
        break;
    }
    if (rc) {
        manager()->showStatusMessage(tr("Debugger running"), -1);
        if (needWatchTimer)
            startWatchTimer();
            emit startSuccessful();
    } else {
        warning(errorMessage);
        setState(InferiorStartFailed, Q_FUNC_INFO, __LINE__);
        emit startFailed();
    }
}

bool CdbDebugEngine::startAttachDebugger(qint64 pid, DebuggerStartMode sm, QString *errorMessage)
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
    if (manager()->startParameters()->startMode != AttachCrashedExternal)
        flags |= DEBUG_ATTACH_INVASIVE_NO_INITIAL_BREAK;
    const HRESULT hr = m_d->m_cif.debugClient->AttachProcess(NULL, pid, flags);
    if (debugCDB)
        qDebug() << "Attaching to " << pid << " using flags" << flags << " returns " << hr << executionStatusString(m_d->m_cif.debugControl);
    if (FAILED(hr)) {
        *errorMessage = tr("Attaching to a process failed for process id %1: %2").arg(pid).arg(msgDebugEngineComResult(hr));
        return false;
    } else {
        m_d->m_mode = sm;
    }
    return true;
}

bool CdbDebugEngine::startDebuggerWithExecutable(DebuggerStartMode sm, QString *errorMessage)
{
    showStatusMessage("Starting Debugger", -1);

    DEBUG_CREATE_PROCESS_OPTIONS dbgopts;
    memset(&dbgopts, 0, sizeof(dbgopts));
    dbgopts.CreateFlags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;

    const QSharedPointer<DebuggerStartParameters> sp = manager()->startParameters();
    const QString filename(sp->executable);
    // Set image path
    const QFileInfo fi(filename);
    QString imagePath = QDir::toNativeSeparators(fi.absolutePath());
    if (!m_d->m_baseImagePath.isEmpty()) {
        imagePath += QLatin1Char(';');
        imagePath += m_d->m_baseImagePath;
    }
    HRESULT hr = m_d->m_cif.debugSymbols->SetImagePathWide(reinterpret_cast<PCWSTR>(imagePath.utf16()));
    if (FAILED(hr)) {
        *errorMessage = tr("Unable to set the image path to %1: %2").arg(imagePath, msgComFailed("SetImagePathWide", hr));
        return false;
    }

    if (debugCDB)
        qDebug() << Q_FUNC_INFO <<'\n' << filename << imagePath;

    ULONG symbolOptions = SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS;
    if (m_d->m_options->verboseSymbolLoading)
        symbolOptions |= SYMOPT_DEBUG;
    m_d->m_cif.debugSymbols->SetSymbolOptions(symbolOptions);

    const QString cmd = Utils::AbstractProcess::createWinCommandline(filename, sp->processArgs);
    if (debugCDB)
        qDebug() << "Starting " << cmd;
    PCWSTR env = 0;
    QByteArray envData;
    if (!sp->environment.empty()) {
        envData = Utils::AbstractProcess::createWinEnvironment(Utils::AbstractProcess::fixWinEnvironment(sp->environment));
        env = reinterpret_cast<PCWSTR>(envData.data());
    }
    // The working directory cannot be empty.
    PCWSTR workingDirC = 0;
    const QString workingDir = sp->workingDir.isEmpty() ? QString() : QDir::toNativeSeparators(sp->workingDir);
    if (!workingDir.isEmpty())
        workingDirC = workingDir.utf16();
    hr = m_d->m_cif.debugClient->CreateProcess2Wide(NULL,
                                                    reinterpret_cast<PWSTR>(const_cast<ushort *>(cmd.utf16())),
                                                    &dbgopts, sizeof(dbgopts),
                                                    workingDirC, env);
    if (FAILED(hr)) {
        *errorMessage = tr("Unable to create a process '%1': %2").arg(cmd, msgDebugEngineComResult(hr));
        return false;
    } else {
        m_d->m_mode = sm;
    }
    return true;
}

void CdbDebugEnginePrivate::processCreatedAttached(ULONG64 processHandle, ULONG64 initialThreadHandle)
{   
    m_engine->setState(InferiorRunningRequested, Q_FUNC_INFO, __LINE__);
    setDebuggeeHandles(reinterpret_cast<HANDLE>(processHandle), reinterpret_cast<HANDLE>(initialThreadHandle));
    ULONG currentThreadId;
    if (SUCCEEDED(m_cif.debugSystemObjects->GetThreadIdByHandle(initialThreadHandle, &currentThreadId))) {
        m_currentThreadId = currentThreadId;
    } else {
        m_currentThreadId = 0;
    }
    // Clear any saved breakpoints and set initial breakpoints
    m_engine->executeDebuggerCommand(QLatin1String("bc"));
    if (manager()->breakHandler()->hasPendingBreakpoints()) {
        if (debugCDBExecution)
            qDebug() << "processCreatedAttached: Syncing breakpoints";
        m_engine->attemptBreakpointSynchronization();
    }
    // Attaching to crashed: This handshake (signalling an event) is required for
    // the exception to be delivered to the debugger
    if (m_mode == AttachCrashedExternal) {
        const QString crashParameter = manager()->startParameters()->crashParameter;
        if (!crashParameter.isEmpty()) {
            ULONG64 evtNr = crashParameter.toULongLong();
            const HRESULT hr = m_cif.debugControl->SetNotifyEventHandle(evtNr);
            if (FAILED(hr))
                m_engine->warning(QString::fromLatin1("Handshake failed on event #%1: %2").arg(evtNr).arg(msgComFailed("SetNotifyEventHandle", hr)));
        }
    }
    m_engine->setState(InferiorRunning, Q_FUNC_INFO, __LINE__);
    if (debugCDBExecution)
        qDebug() << "<processCreatedAttached" << executionStatusString(m_cif.debugControl);
}

void CdbDebugEngine::processTerminated(unsigned long exitCode)
{
    manager()->showDebuggerOutput(LogMisc, tr("The process exited with exit code %1.").arg(exitCode));
    setState(InferiorShuttingDown, Q_FUNC_INFO, __LINE__);
    m_d->setDebuggeeHandles(0, 0);
    m_d->clearForRun();
    setState(InferiorShutDown, Q_FUNC_INFO, __LINE__);
    // Avoid calls from event handler.
    QTimer::singleShot(0, manager(), SLOT(exitDebugger()));
}

bool CdbDebugEnginePrivate::endInferior(EndInferiorAction action, QString *errorMessage)
{
    // Process must be stopped in order to terminate
    m_engine->setState(InferiorShuttingDown, Q_FUNC_INFO, __LINE__); // pretend it is shutdown
    const bool wasRunning = isDebuggeeRunning();
    if (wasRunning) {
        interruptInterferiorProcess(errorMessage);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    bool success = false;
    switch (action) {
    case DetachInferior: {            
            const HRESULT hr = m_cif.debugClient->DetachCurrentProcess();
            if (SUCCEEDED(hr)) {
                success = true;
            } else {
                *errorMessage += msgComFailed("DetachCurrentProcess", hr);
            }
        }
            break;
    case TerminateInferior: {
            do {
                // The exit process event handler will not be called.
                HRESULT hr = m_cif.debugClient->TerminateCurrentProcess();
                if (FAILED(hr)) {
                    *errorMessage += msgComFailed("TerminateCurrentProcess", hr);
                    break;
                }
                if (wasRunning) {
                    success = true;
                    break;
                }
                hr = m_cif.debugClient->TerminateProcesses();
                if (SUCCEEDED(hr)) {
                    success = true;
                } else {
                    *errorMessage += msgComFailed("TerminateProcesses", hr);
                }
            } while (false);
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            break;
        }
    }
    // Perform cleanup even when failed..no point clinging to the process
    setDebuggeeHandles(0, 0);
    m_engine->killWatchTimer();
    m_engine->setState(success ? InferiorShutDown : InferiorShutdownFailed, Q_FUNC_INFO, __LINE__);
    return success;
}

// End debugging. Note that this can invoked via user action
// or the processTerminated() event handler, in which case it
// must not kill the process again.
void CdbDebugEnginePrivate::endDebugging(EndDebuggingMode em)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << em;

    const DebuggerState oldState = m_engine->state();
    if (oldState == DebuggerNotReady || m_mode == AttachCore)
        return;
    // Do we need to stop the process?
    QString errorMessage;
    if (oldState != InferiorShutDown && m_hDebuggeeProcess) {
        EndInferiorAction action;
        switch (em) {
        case EndDebuggingAuto:
            action = (m_mode == AttachExternal || m_mode == AttachCrashedExternal) ?
                     DetachInferior : TerminateInferior;
            break;
        case EndDebuggingDetach:
            action = DetachInferior;
            break;
        case EndDebuggingTerminate:
            action = TerminateInferior;
            break;
        }
        if (debugCDB)
            qDebug() << Q_FUNC_INFO << action;
        // Need a stopped debuggee to act
        if (!endInferior(action, &errorMessage)) {
            errorMessage = QString::fromLatin1("Unable to detach from/end the debuggee: %1").arg(errorMessage);
            manager()->showDebuggerOutput(LogError, errorMessage);
        }
        errorMessage.clear();
    }
    // Clean up resources (open files, etc.)
    m_engine->setState(AdapterShuttingDown, Q_FUNC_INFO, __LINE__);
    clearForRun();
    const HRESULT hr = m_cif.debugClient->EndSession(DEBUG_END_PASSIVE);
    if (SUCCEEDED(hr)) {
        m_engine->setState(DebuggerNotReady, Q_FUNC_INFO, __LINE__);
    } else {
        m_engine->setState(AdapterShutdownFailed, Q_FUNC_INFO, __LINE__);
        m_engine->setState(DebuggerNotReady, Q_FUNC_INFO, __LINE__);
        errorMessage = QString::fromLatin1("There were errors trying to end debugging: %1").arg(msgComFailed("EndSession", hr));
        manager()->showDebuggerOutput(LogError, errorMessage);
    }
}

void CdbDebugEngine::exitDebugger()
{
    m_d->endDebugging();
}

void CdbDebugEngine::detachDebugger()
{
    m_d->endDebugging(CdbDebugEnginePrivate::EndDebuggingDetach);
}

CdbStackFrameContext *CdbDebugEnginePrivate::getStackFrameContext(int frameIndex, QString *errorMessage) const
{
    if (!m_currentStackTrace) {
        *errorMessage = QLatin1String(msgNoStackTraceC);
        return 0;
    }
    if (CdbStackFrameContext *sg = m_currentStackTrace->frameContextAt(frameIndex, errorMessage))
        return sg;
    return 0;
}

void CdbDebugEngine::evaluateWatcher(WatchData *wd)
{
    if (debugCDBWatchHandling)
        qDebug() << Q_FUNC_INFO << wd->exp;
    QString errorMessage;
    QString value;
    QString type;
    if (evaluateExpression(wd->exp, &value, &type, &errorMessage)) {
        wd->setValue(value);
        wd->setType(type);
    } else {
        wd->setValue(errorMessage);
        wd->setTypeUnneeded();
    }
    wd->setHasChildren(false);
}

void CdbDebugEngine::updateWatchData(const WatchData &incomplete)
{
    // Watch item was edited while running
    if (m_d->isDebuggeeRunning())
        return;

    if (debugCDBWatchHandling)
        qDebug() << Q_FUNC_INFO << "\n    " << incomplete.toString();

    WatchHandler *watchHandler = manager()->watchHandler();
    if (incomplete.iname.startsWith(QLatin1String("watch."))) {
        WatchData watchData = incomplete;
        evaluateWatcher(&watchData);
        watchHandler->insertData(watchData);
        return;
    }

    const int frameIndex = manager()->stackHandler()->currentIndex();

    bool success = false;
    QString errorMessage;
    do {
        CdbStackFrameContext *sg = m_d->m_currentStackTrace->frameContextAt(frameIndex, &errorMessage);
        if (!sg)
            break;
        if (!sg->completeData(incomplete, watchHandler, &errorMessage))
            break;
        success = true;
    } while (false);
    if (!success)
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    if (debugCDBWatchHandling > 1)
        qDebug() << *manager()->watchHandler()->model(LocalsWatch);
}

// Continue inferior with a debugger command, such as "p", "pt"
// or its thread variations
bool CdbDebugEnginePrivate::executeContinueCommand(const QString &command)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << command;
    clearForRun();
    setCodeLevel(); // Step by instruction
    m_engine->setState(InferiorRunningRequested, Q_FUNC_INFO, __LINE__);
    manager()->showDebuggerOutput(LogMisc, CdbDebugEngine::tr("Continuing with '%1'...").arg(command));
    QString errorMessage;
    const bool success = CdbDebugEnginePrivate::executeDebuggerCommand(m_cif.debugControl, command, &errorMessage);
    if (success) {
        m_engine->setState(InferiorRunning, Q_FUNC_INFO, __LINE__);
        m_engine->startWatchTimer();
    } else {
        m_engine->setState(InferiorStopped, Q_FUNC_INFO, __LINE__);
        m_engine->warning(CdbDebugEngine::tr("Unable to continue: %1").arg(errorMessage));
    }
    return success;
}

static inline QString msgStepFailed(unsigned long executionStatus, int threadId, const QString &why)
{
    if (executionStatus ==  DEBUG_STATUS_STEP_OVER)
        return QString::fromLatin1("Thread %1: Unable to step over: %2").arg(threadId).arg(why);
    return QString::fromLatin1("Thread %1: Unable to step into: %2").arg(threadId).arg(why);
}

// Step with  DEBUG_STATUS_STEP_OVER ('p'-command) or
// DEBUG_STATUS_STEP_INTO ('t'-trace-command) or
// its reverse equivalents in the case of single threads.
bool CdbDebugEngine::step(unsigned long executionStatus)
{
    if (debugCDBExecution)
        qDebug() << ">step" << executionStatus << "curr " << m_d->m_currentThreadId << " evt " << m_d->m_eventThreadId;

    // State of reverse stepping as of 10/2009 (Debugging tools 6.11@404):
    // The constants exist, but invoking the calls leads to E_NOINTERFACE.
    // Also there is no CDB command for it.
    if (executionStatus == DEBUG_STATUS_REVERSE_STEP_OVER || executionStatus == DEBUG_STATUS_REVERSE_STEP_INTO) {
        warning(tr("Reverse stepping is not implemented."));
        return false;
    }

    // Do not step the artifical thread created to interrupt the debuggee.
    if (m_d->m_interrupted && m_d->m_currentThreadId == m_d->m_interruptArticifialThreadId) {
        warning(tr("Thread %1 cannot be stepped.").arg(m_d->m_currentThreadId));
        return false;
    }

    // SetExecutionStatus() continues the thread that triggered the
    // stop event (~# p). This can be confusing if the user is looking
    // at the stack trace of another thread and wants to step that one. If that
    // is the case, explicitly tell it to step the current thread using a command.
    const int triggeringEventThread = m_d->m_eventThreadId;
    const bool sameThread = triggeringEventThread == -1
                            || m_d->m_currentThreadId == triggeringEventThread
                            || manager()->threadsHandler()->threads().size() == 1;
    m_d->clearForRun(); // clears thread ids
    m_d->setCodeLevel(); // Step by instruction or source line
    setState(InferiorRunningRequested, Q_FUNC_INFO, __LINE__);
    bool success = false;
    if (sameThread) { // Step event-triggering thread, use fast API
        const HRESULT hr = m_d->m_cif.debugControl->SetExecutionStatus(executionStatus);
        success = SUCCEEDED(hr);
        if (!success)
            warning(msgStepFailed(executionStatus, m_d->m_currentThreadId, msgComFailed("SetExecutionStatus", hr)));
    } else {
        // Need to use a command to explicitly specify the current thread
        QString command;
        QTextStream str(&command);
        str << '~' << m_d->m_currentThreadId << ' '
                << (executionStatus == DEBUG_STATUS_STEP_OVER ? 'p' : 't');
        manager()->showDebuggerOutput(tr("Stepping %1").arg(command));
        const HRESULT hr = m_d->m_cif.debugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, command.toLatin1().constData(), DEBUG_EXECUTE_ECHO);
        success = SUCCEEDED(hr);
        if (!success)
            warning(msgStepFailed(executionStatus, m_d->m_currentThreadId, msgDebuggerCommandFailed(command, hr)));
    }
    if (success) {
        // Oddity: Step into will first break at the calling function. Ignore
        if (executionStatus == DEBUG_STATUS_STEP_INTO || executionStatus == DEBUG_STATUS_REVERSE_STEP_INTO)
            m_d->m_breakEventMode = CdbDebugEnginePrivate::BreakEventIgnoreOnce;
        startWatchTimer();
        setState(InferiorRunning, Q_FUNC_INFO, __LINE__);
    } else {
        setState(InferiorStopped, Q_FUNC_INFO, __LINE__);
    }
    if (debugCDBExecution)
        qDebug() << "<step samethread" << sameThread << "succeeded" << success;
    return success;
}

void CdbDebugEngine::stepExec()
{
    step(manager()->isReverseDebugging() ? DEBUG_STATUS_REVERSE_STEP_INTO : DEBUG_STATUS_STEP_INTO);
}

void CdbDebugEngine::nextExec()
{
    step(manager()->isReverseDebugging() ? DEBUG_STATUS_REVERSE_STEP_OVER : DEBUG_STATUS_STEP_OVER);
}

void CdbDebugEngine::stepIExec()
{      
    stepExec(); // Step into by instruction (figured out by step)
}

void CdbDebugEngine::nextIExec()
{
    nextExec(); // Step over by instruction (figured out by step)
}

void CdbDebugEngine::stepOutExec()
{
    if (debugCDBExecution)
        qDebug() << "stepOutExec";
   // emulate gdb 'exec-finish' (exec until return of current function)
   // by running up to address of the above stack frame (mostly works).
   const StackHandler* sh = manager()->stackHandler();
   const int idx = sh->currentIndex() + 1;
   const QList<StackFrame> stackframes = sh->frames();
   if (idx < 0 || idx >= stackframes.size()) {
       warning(QString::fromLatin1("Cannot step out of stack frame %1.").arg(idx));
       return;
   }
   // Set a temporary breakpoint and continue
   const StackFrame& frame = stackframes.at(idx);
   bool success = false;
   QString errorMessage;
   do {
       const ULONG64 address = frame.address.toULongLong(&success, 16);
       if (!success) {
           errorMessage = QLatin1String("Cannot obtain address from stack frame");
           break;
       }
       manager()->showDebuggerOutput(LogMisc, tr("Running to 0x%1...").arg(address, 0, 16));
       IDebugBreakpoint2* pBP;
       HRESULT hr = m_d->m_cif.debugControl->AddBreakpoint2(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &pBP);
       if (FAILED(hr) || !pBP) {
           errorMessage = QString::fromLatin1("Cannot create temporary breakpoint: %1").arg(msgDebugEngineComResult(hr));
           break;
       }

       pBP->SetOffset(address);
       pBP->AddFlags(DEBUG_BREAKPOINT_ENABLED);
       pBP->AddFlags(DEBUG_BREAKPOINT_ONE_SHOT);
       if (!m_d->continueInferior(&errorMessage))
           break;
       success = true;
   } while (false);
   if (!success)
       warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
}

void CdbDebugEngine::continueInferior()
{
    QString errorMessage;    
    if  (!m_d->continueInferior(&errorMessage))
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
}

// Continue process without notifications
bool CdbDebugEnginePrivate::continueInferiorProcess(QString *errorMessagePtr /* = 0 */)
{
    if (debugCDBExecution)
        qDebug() << "continueInferiorProcess";
    const HRESULT hr = m_cif.debugControl->SetExecutionStatus(DEBUG_STATUS_GO);
    if (FAILED(hr)) {
        const QString errorMessage = msgComFailed("SetExecutionStatus", hr);
        if (errorMessagePtr) {
            *errorMessagePtr = errorMessage;
        } else {
            m_engine->warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
        }
        return false;
    }
    return  true;
}

// Continue process with notifications
bool CdbDebugEnginePrivate::continueInferior(QString *errorMessage)
{
    // Check state: Are we running?
    ULONG executionStatus;
    if (!getExecutionStatus(m_cif.debugControl, &executionStatus, errorMessage))
        return false;

    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\n    ex=" << executionStatus;

    if (executionStatus == DEBUG_STATUS_GO) {
        m_engine->warning(QLatin1String("continueInferior() called while debuggee is running."));
        return true;
    }
    // Request continue
    m_engine->setState(InferiorRunningRequested, Q_FUNC_INFO, __LINE__);
    bool success = false;
    do {
        clearForRun();
        setCodeLevel();
        m_engine->killWatchTimer();
        manager()->resetLocation();
        manager()->showStatusMessage(CdbDebugEngine::tr("Running requested..."), 5000);

        if (!continueInferiorProcess(errorMessage))
            break;

        m_engine->startWatchTimer();
        success = true;
    } while (false);
    if (success) {
        m_engine->setState(InferiorRunning, Q_FUNC_INFO, __LINE__);
    } else {
        m_engine->setState(InferiorStopped, Q_FUNC_INFO, __LINE__); // No RunningRequestFailed?
    }
    return true;
}

bool CdbDebugEnginePrivate::interruptInterferiorProcess(QString *errorMessage)
{

    // Interrupt the interferior process without notifications
    if (debugCDBExecution) {
        ULONG executionStatus;
        getExecutionStatus(m_cif.debugControl, &executionStatus, errorMessage);
        qDebug() << "interruptInterferiorProcess  ex=" << executionStatus;
    }

    if (DebugBreakProcess(m_hDebuggeeProcess)) {
        m_interrupted = true;
    } else {
        *errorMessage = QString::fromLatin1("DebugBreakProcess failed: %1").arg(Utils::winErrorMessage(GetLastError()));
        return false;
    }
#if 0
    const HRESULT hr = m_cif.debugControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE|DEBUG_INTERRUPT_EXIT);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to interrupt debuggee after %1s: %2").
                        arg(getInterruptTimeOutSecs(m_cif.debugControl)).arg(msgComFailed("SetInterrupt", hr));
        return false;
    }
    m_interrupted = true;
#endif
    return true;
}

void CdbDebugEngine::interruptInferior()
{
    if (!m_d->m_hDebuggeeProcess || !m_d->isDebuggeeRunning())
        return;

    QString errorMessage;
    setState(InferiorStopping, Q_FUNC_INFO, __LINE__);
    if (!m_d->interruptInterferiorProcess(&errorMessage)) {
        setState(InferiorStopFailed, Q_FUNC_INFO, __LINE__);
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    }
}

void CdbDebugEngine::runToLineExec(const QString &fileName, int lineNumber)
{
    manager()->showDebuggerOutput(LogMisc, tr("Running up to %1:%2...").arg(fileName).arg(lineNumber));
    QString errorMessage;
    CDBBreakPoint tempBreakPoint;
    tempBreakPoint.fileName = fileName;
    tempBreakPoint.lineNumber = lineNumber;
    tempBreakPoint.oneShot = true;
    const bool ok = tempBreakPoint.add(m_d->m_cif.debugControl, &errorMessage)
                    && m_d->continueInferior(&errorMessage);
    if (!ok)
        warning(errorMessage);
}

void CdbDebugEngine::runToFunctionExec(const QString &functionName)
{
    manager()->showDebuggerOutput(LogMisc, tr("Running up to function '%1()'...").arg(functionName));
    QString errorMessage;
    CDBBreakPoint tempBreakPoint;
    tempBreakPoint.funcName = functionName;
    tempBreakPoint.oneShot = true;
    const bool ok = tempBreakPoint.add(m_d->m_cif.debugControl, &errorMessage)
                    && m_d->continueInferior(&errorMessage);
    if (!ok)
        warning(errorMessage);
}

void CdbDebugEngine::jumpToLineExec(const QString & /* fileName */, int /*lineNumber*/)
{
    warning(tr("Jump to line is not implemented"));
}

void CdbDebugEngine::assignValueInDebugger(const QString &expr, const QString &value)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << expr << value;
    const int frameIndex = manager()->stackHandler()->currentIndex();
    QString errorMessage;
    bool success = false;
    do {
        QString newValue;
        CdbStackFrameContext *sg = m_d->getStackFrameContext(frameIndex, &errorMessage);
        if (!sg)
            break;
        if (!sg->assignValue(expr, value, &newValue, &errorMessage))
            break;
        // Update view
        WatchHandler *watchHandler = manager()->watchHandler();
        if (WatchData *fwd = watchHandler->findItem(expr)) {
            fwd->setValue(newValue);
            watchHandler->insertData(*fwd);
            watchHandler->updateWatchers();
        }
        success = true;
    } while (false);
    if (!success) {
        const QString msg = tr("Unable to assign the value '%1' to '%2': %3").arg(value, expr, errorMessage);
        warning(msg);
    }
}

void CdbDebugEngine::executeDebuggerCommand(const QString &command)
{
    QString errorMessage;
    if (!CdbDebugEnginePrivate::executeDebuggerCommand(m_d->m_cif.debugControl, command, &errorMessage))
        warning(errorMessage);
}

bool CdbDebugEnginePrivate::executeDebuggerCommand(CIDebugControl *ctrl, const QString &command, QString *errorMessage)
{
    // output to all clients, else we do not see anything
    const HRESULT hr = ctrl->ExecuteWide(DEBUG_OUTCTL_ALL_CLIENTS, reinterpret_cast<PCWSTR>(command.utf16()), 0);
    if (debugCDB)
        qDebug() << "executeDebuggerCommand" << command << SUCCEEDED(hr);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Unable to execute '%1': %2").
                        arg(command, msgDebugEngineComResult(hr));
        return false;
    }
    return true;
}

bool CdbDebugEngine::evaluateExpression(const QString &expression,
                                        QString *value,
                                        QString *type,
                                        QString *errorMessage)
{
    DEBUG_VALUE debugValue;
    if (!m_d->evaluateExpression(m_d->m_cif.debugControl, expression, &debugValue, errorMessage))
        return false;
    *value = CdbSymbolGroupContext::debugValueToString(debugValue, m_d->m_cif.debugControl, type);
    return true;
}

bool CdbDebugEnginePrivate::evaluateExpression(CIDebugControl *ctrl,
                                               const QString &expression,
                                               DEBUG_VALUE *debugValue,
                                               QString *errorMessage)
{
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << expression;

    memset(debugValue, 0, sizeof(DEBUG_VALUE));
    // Original syntax must be restored, else setting breakpoints will fail.
    SyntaxSetter syntaxSetter(ctrl, DEBUG_EXPR_CPLUSPLUS);
    ULONG errorPosition = 0;
    const HRESULT hr = ctrl->EvaluateWide(reinterpret_cast<PCWSTR>(expression.utf16()),
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

void CdbDebugEngine::activateFrame(int frameIndex)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << frameIndex;

    if (state() != InferiorStopped) {
        qWarning("WARNING %s: invoked while debuggee is running\n", Q_FUNC_INFO);
        return;
    }

    QString errorMessage;
    bool success = false;
    StackHandler *stackHandler = manager()->stackHandler();
    do {        
        WatchHandler *watchHandler = manager()->watchHandler();
        const int oldIndex = stackHandler->currentIndex();
        if (frameIndex >= stackHandler->stackSize()) {
            errorMessage = msgStackIndexOutOfRange(frameIndex, stackHandler->stackSize());
            break;
        }

        if (oldIndex != frameIndex)
            stackHandler->setCurrentIndex(frameIndex);

        const StackFrame &frame = stackHandler->currentFrame();
        if (!frame.isUsable()) {
            // Clean out model
            watchHandler->beginCycle();
            watchHandler->endCycle();
            errorMessage = QString::fromLatin1("%1: file %2 unusable.").
                           arg(QLatin1String(Q_FUNC_INFO), frame.file);
            break;
        }

        manager()->gotoLocation(frame, true);

        if (oldIndex != frameIndex || m_d->m_firstActivatedFrame) {
            watchHandler->beginCycle();
            if (CdbStackFrameContext *sgc = m_d->getStackFrameContext(frameIndex, &errorMessage))
                success = sgc->populateModelInitially(watchHandler, &errorMessage);
            watchHandler->endCycle();
        } else {
            success = true;
        }
    } while (false);
    if (!success) {
        const QString msg = QString::fromLatin1("Internal error: activateFrame() failed for frame #%1 of %2, thread %3: %4").
                            arg(frameIndex).arg(stackHandler->stackSize()).arg(m_d->m_currentThreadId).arg(errorMessage);
        warning(msg);
    }
    m_d->m_firstActivatedFrame = false;
}

void CdbDebugEngine::selectThread(int index)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << index;

    //reset location arrow
    manager()->resetLocation();

    ThreadsHandler *threadsHandler = manager()->threadsHandler();
    threadsHandler->setCurrentThread(index);
    const int newThreadId = threadsHandler->threads().at(index).id;
    if (newThreadId != m_d->m_currentThreadId) {
        m_d->m_currentThreadId = threadsHandler->threads().at(index).id;
        m_d->updateStackTrace();
    }
}

void CdbDebugEngine::attemptBreakpointSynchronization()
{
    if (!m_d->m_hDebuggeeProcess) // Sometimes called from the breakpoint Window
        return;
    QString errorMessage;
    if (!m_d->attemptBreakpointSynchronization(&errorMessage))
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
}

bool CdbDebugEnginePrivate::attemptBreakpointSynchronization(QString *errorMessage)
{
    if (!m_hDebuggeeProcess) {
        *errorMessage = QLatin1String("attemptBreakpointSynchronization() called while debugger is not running");
        return false;
    }
    // This is called from
    // 1) CreateProcessEvent with the halted engine
    // 2) from the break handler, potentially while the debuggee is running
    // If the debuggee is running (for which the execution status is
    // no reliable indicator), we temporarily halt and have ourselves
    // called again from the debug event handler.

    ULONG dummy;
    const bool wasRunning = !CDBBreakPoint::getBreakPointCount(m_cif.debugControl, &dummy);
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\n  Running=" << wasRunning;

    if (wasRunning) {
        const HandleBreakEventMode oldMode = m_breakEventMode;
        m_breakEventMode = BreakEventSyncBreakPoints;
        if (!interruptInterferiorProcess(errorMessage)) {
            m_breakEventMode = oldMode;
            return false;
        }
        return true;
    }

    QStringList warnings;
    const bool ok = CDBBreakPoint::synchronizeBreakPoints(m_cif.debugControl,
                                                 m_cif.debugSymbols,
                                                 manager()->breakHandler(),
                                                 errorMessage, &warnings);
    if (const int warningsCount = warnings.size())
        for (int w = 0; w < warningsCount; w++)
            m_engine->warning(warnings.at(w));
    return ok;
}

void CdbDebugEngine::fetchDisassembler(DisassemblerViewAgent *agent,
                                       const StackFrame & frame)
{
    enum { ContextLines = 40 };
    bool ok = false;
    QString errorMessage;
    do {
        // get address       
        QString address;
        if (!frame.file.isEmpty())
            address = frame.address;
        if (address.isEmpty())
            address = agent->address();
        if (debugCDB)
            qDebug() << "fetchDisassembler" << address << " Agent: " << agent->address()
            << " Frame" << frame.file << frame.line << frame.address;
        if (address.isEmpty()) { // Clear window
            agent->setContents(QString());
            ok = true;
            break;
        }
        if (address.startsWith(QLatin1String("0x")))
            address.remove(0, 2);
        const int addressFieldWith = address.size(); // For the Marker

        const ULONG64 offset = address.toULongLong(&ok, 16);
        if (!ok) {
            errorMessage = QString::fromLatin1("Internal error: Invalid address for disassembly: '%1'.").arg(agent->address());
            break;
        }
        QString disassembly;
        QApplication::setOverrideCursor(Qt::WaitCursor);
        ok = dissassemble(m_d->m_cif.debugClient, m_d->m_cif.debugControl, offset,
                          ContextLines, ContextLines, addressFieldWith, QTextStream(&disassembly), &errorMessage);
        QApplication::restoreOverrideCursor();
        if (!ok)
            break;
        agent->setContents(disassembly);

    } while (false);

    if (!ok) {
        agent->setContents(QString());
        warning(errorMessage);
    }
}

void CdbDebugEngine::fetchMemory(MemoryViewAgent *agent, quint64 addr, quint64 length)
{
    if (!m_d->m_hDebuggeeProcess && !length)
        return;
    ULONG received;
    QByteArray data(length, '\0');
    const HRESULT hr = m_d->m_cif.debugDataSpaces->ReadVirtual(addr, data.data(), length, &received);
    if (FAILED(hr)) {
        warning(tr("Unable to retrieve %1 bytes of memory at 0x%2: %3").
                arg(length).arg(addr, 0, 16).arg(msgComFailed("ReadVirtual", hr)));
        return;
    }
    if (received < length)
        data.truncate(received);
    agent->addLazyData(addr, data);
}

void CdbDebugEngine::reloadModules()
{
}

void CdbDebugEngine::loadSymbols(const QString &moduleName)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << moduleName;
}

void CdbDebugEngine::loadAllSymbols()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
}

QList<Symbol> CdbDebugEngine::moduleSymbols(const QString &moduleName)
{
    QList<Symbol> rc;
    QString errorMessage;
    bool success = false;
    do {
        if (m_d->isDebuggeeRunning()) {
            errorMessage = tr("Cannot retrieve symbols while the debuggee is running.");
            break;
        }
        if (!getModuleSymbols(m_d->m_cif.debugSymbols, moduleName, &rc, &errorMessage))
            break;
        success = true;
    } while (false);
    if (!success)
        warning(errorMessage);
    return rc;
}

void CdbDebugEngine::reloadRegisters()
{
    const int intBase = 10;
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << intBase;
    QList<Register> registers;
    QString errorMessage;
    if (!getRegisters(m_d->m_cif.debugControl, m_d->m_cif.debugRegisters, &registers, &errorMessage, intBase))
        warning(msgFunctionFailed("reloadRegisters" , errorMessage));
    manager()->registerHandler()->setRegisters(registers);
}

void CdbDebugEngine::timerEvent(QTimerEvent* te)
{
    // Fetch away the debug events and notify if debuggee
    // stops. Note that IDebugEventCallback does not
    // cover all cases of a debuggee stopping execution
    // (such as step over,etc).
    if (te->timerId() != m_d->m_watchTimer)
        return;

    const HRESULT hr = m_d->m_cif.debugControl->WaitForEvent(0, 1);
    if (debugCDB)
        if (debugCDB > 1 || hr != S_FALSE)
            qDebug() << Q_FUNC_INFO << "WaitForEvent" << state() << msgDebugEngineComResult(hr);

    switch (hr) {
        case S_OK:
            killWatchTimer();
            m_d->handleDebugEvent();
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

void CdbDebugEngine::slotConsoleStubStarted()
{
    const qint64 appPid = m_d->m_consoleStubProc.applicationPID();
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << appPid;
    // Attach to console process
    QString errorMessage;
    if (startAttachDebugger(appPid, AttachExternal, &errorMessage)) {
        startWatchTimer();
        manager()->notifyInferiorPidChanged(appPid);
    } else {
        QMessageBox::critical(manager()->mainWindow(), tr("Debugger Error"), errorMessage);
    }
}

void CdbDebugEngine::slotConsoleStubError(const QString &msg)
{
    QMessageBox::critical(manager()->mainWindow(), tr("Debugger Error"), msg);
}

void CdbDebugEngine::slotConsoleStubTerminated()
{
    exitDebugger();
}

void CdbDebugEngine::warning(const QString &w)
{
    manager()->showDebuggerOutput(LogWarning, w);
    qWarning("%s\n", qPrintable(w));
}

void CdbDebugEnginePrivate::notifyException(long code, bool fatal)
{
    if (debugCDBExecution)
        qDebug() << "notifyException code" << code << " fatal=" << fatal;
    // Suppress the initial breakpoint that occurs when
    // attaching to a console (If a breakpoint is encountered before startup
    // is complete, see startAttachDebugger()).
    switch (code) {
    case winExceptionStartupCompleteTrap:
        m_inferiorStartupComplete = true;
        break;
    case EXCEPTION_BREAKPOINT:
        if (m_ignoreInitialBreakPoint && !m_inferiorStartupComplete && m_breakEventMode == BreakEventHandle) {
            manager()->showDebuggerOutput(LogMisc, CdbDebugEngine::tr("Ignoring initial breakpoint..."));
            m_breakEventMode = BreakEventIgnoreOnce;
        }
        break;
    }
    // Cannot go over crash point to execute calls.
    if (fatal)
        m_dumper->disable();
}

static int threadIndexById(const ThreadsHandler *threadsHandler, int id)
{
    const QList<ThreadData> threads = threadsHandler->threads();
    const int count = threads.count();
    for (int i = 0; i < count; i++)
        if (threads.at(i).id == id)
            return i;
    return -1;
}

void CdbDebugEnginePrivate::handleDebugEvent()
{
    if (debugCDBExecution)
        qDebug() << "handleDebugEvent mode " << m_breakEventMode
                << executionStatusString(m_cif.debugControl) << " interrupt" << m_interrupted
                << " startupcomplete" << m_inferiorStartupComplete;
    // restore mode and do special handling
    const HandleBreakEventMode mode = m_breakEventMode;
    m_breakEventMode = BreakEventHandle;

    switch (mode) {
    case BreakEventHandle: {
        // If this is triggered by breakpoint/crash: Set state to stopping
        // to avoid warnings as opposed to interrupt inferior
        if (m_engine->state() != InferiorStopping)
            m_engine->setState(InferiorStopping, Q_FUNC_INFO, __LINE__);
        m_engine->setState(InferiorStopped, Q_FUNC_INFO, __LINE__);
        m_eventThreadId = updateThreadList();
        m_interruptArticifialThreadId = m_interrupted ? m_eventThreadId : -1;
        // Get thread to stop and its index. If avoidable, do not use
        // the artifical thread that is created when interrupting,
        // use the oldest thread 0 instead.
        ThreadsHandler *threadsHandler = manager()->threadsHandler();
        m_currentThreadId = m_interrupted ? 0 : m_eventThreadId;
        int currentThreadIndex = -1;
        m_currentThreadId = -1;
        if (m_interrupted) {
            m_currentThreadId = 0;
            currentThreadIndex = threadIndexById(threadsHandler, m_currentThreadId);
        }
        if (!m_interrupted || currentThreadIndex == -1) {
            m_currentThreadId = m_eventThreadId;
            currentThreadIndex = threadIndexById(threadsHandler, m_currentThreadId);
        }
        const QString msg = m_interrupted ?
                            CdbDebugEngine::tr("Interrupted in thread %1, current thread: %2").arg(m_interruptArticifialThreadId).arg(m_currentThreadId) :
                            CdbDebugEngine::tr("Stopped, current thread: %1").arg(m_currentThreadId);
        manager()->showDebuggerOutput(LogMisc, msg);
        const int threadIndex = threadIndexById(threadsHandler, m_currentThreadId);
        if (threadIndex != -1)
            threadsHandler->setCurrentThread(threadIndex);
        updateStackTrace();
    }
        break;
    case BreakEventIgnoreOnce:
        m_engine->startWatchTimer();
        m_interrupted = false;
        break;
    case BreakEventSyncBreakPoints: {
            m_interrupted = false;
            // Temp stop to sync breakpoints
            QString errorMessage;
            attemptBreakpointSynchronization(&errorMessage);
            m_engine->startWatchTimer();
            continueInferiorProcess(&errorMessage);
            if (!errorMessage.isEmpty())
                m_engine->warning(QString::fromLatin1("In handleDebugEvent: %1").arg(errorMessage));
    }
        break;
    }
}

void CdbDebugEnginePrivate::setDebuggeeHandles(HANDLE hDebuggeeProcess,  HANDLE hDebuggeeThread)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << hDebuggeeProcess << hDebuggeeThread;
    m_hDebuggeeProcess = hDebuggeeProcess;
    m_hDebuggeeThread = hDebuggeeThread;
}

// Set thread in CDB engine
bool CdbDebugEnginePrivate::setCDBThreadId(unsigned long threadId, QString *errorMessage)
{
    ULONG currentThreadId;
    HRESULT hr = m_cif.debugSystemObjects->GetCurrentThreadId(&currentThreadId);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetCurrentThreadId", hr);
        return false;
    }
    if (currentThreadId == threadId)
        return true;
    hr = m_cif.debugSystemObjects->SetCurrentThreadId(threadId);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Failed to change to from thread %1 to %2: SetCurrentThreadId() failed: %3").
                        arg(currentThreadId).arg(threadId).arg(msgDebugEngineComResult(hr));
        return false;
    }
    const QString msg = CdbDebugEngine::tr("Changing threads: %1 -> %2").arg(currentThreadId).arg(threadId);
    m_engine->showStatusMessage(msg, 500);    
    return true;
}

ULONG CdbDebugEnginePrivate::updateThreadList()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << m_hDebuggeeProcess;

    QList<ThreadData> threads;
    ULONG currentThreadId;
    QString errorMessage;
    // When interrupting, an artifical thread with a breakpoint is created.
    if (!CdbStackTraceContext::getThreads(m_cif, true, &threads, &currentThreadId, &errorMessage))
        m_engine->warning(errorMessage);
    manager()->threadsHandler()->setThreads(threads);
    return currentThreadId;
}

// Figure out the thread to run the dumpers in (see notes on.
// CdbDumperHelper). Avoid the artifical threads created by interrupt
// and threads that are in waitFor().
// A stricter version could only use the thread if it is the event
// thread of a step or breakpoint hit (see CdbDebugEnginePrivate::m_interrupted).

static inline unsigned long dumperThreadId(const QList<StackFrame> &frames,
                                           unsigned long currentThread)
{
    if (frames.empty())
        return CdbDumperHelper::InvalidDumperCallThread;
    if (frames.at(0).function == QLatin1String(CdbStackTraceContext::winFuncDebugBreakPoint))
        return CdbDumperHelper::InvalidDumperCallThread;
    const int waitCheckDepth = qMin(frames.size(), 5);
    static const QString waitForPrefix = QLatin1String(CdbStackTraceContext::winFuncWaitForPrefix);
    static const QString msgWaitForPrefix = QLatin1String(CdbStackTraceContext::winFuncMsgWaitForPrefix);
    for (int f = 0; f < waitCheckDepth; f++) {
        const QString &function = frames.at(f).function;
        if (function.startsWith(waitForPrefix) || function.startsWith(msgWaitForPrefix))
            return CdbDumperHelper::InvalidDumperCallThread;
    }
    return currentThread;
}

void CdbDebugEnginePrivate::updateStackTrace()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
    // Create a new context
    cleanStackTrace();
    QString errorMessage;
    m_engine->reloadRegisters();
    if (!setCDBThreadId(m_currentThreadId, &errorMessage)) {
        m_engine->warning(errorMessage);
        return;
    }
    m_currentStackTrace =
            CdbStackTraceContext::create(m_dumper, m_currentThreadId, &errorMessage);
    if (!m_currentStackTrace) {
        m_engine->warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
        return;
    }
    // Disassembling slows things down a bit. Assembler is still available via menu.
#if 0
    m_engine->reloadDisassembler(); // requires stack trace
#endif
    const QList<StackFrame> stackFrames = m_currentStackTrace->frames();
    // find the first usable frame and select it
    int current = -1;
    const int count = stackFrames.count();
    for (int i=0; i < count; ++i)
        if (stackFrames.at(i).isUsable()) {
            current = i;
            break;
        }
    // Visibly warn the users about missing top frames/all frames, as they otherwise
    // might think stepping is broken.    
    if (!stackFrames.at(0).isUsable()) {
        const QString topFunction = count ? stackFrames.at(0).function : QString();
        const QString msg = current >= 0 ?
                            CdbDebugEngine::tr("Thread %1: Missing debug information for top stack frame (%2).").arg(m_currentThreadId).arg(topFunction) :
                            CdbDebugEngine::tr("Thread %1: No debug information available (%2).").arg(m_currentThreadId).arg(topFunction);
        m_engine->warning(msg);
    }
    // Set up dumper with a thread (or invalid)
    const unsigned long dumperThread = dumperThreadId(stackFrames, m_currentThreadId);
    if (debugCDBExecution)
        qDebug() << "updateStackTrace() current: " << m_currentThreadId << " dumper=" << dumperThread;
    m_dumper->setDumperCallThread(dumperThread);
    // Display frames
    manager()->stackHandler()->setFrames(stackFrames);
    m_firstActivatedFrame = true;
    if (current >= 0) {
        manager()->stackHandler()->setCurrentIndex(current);
        m_engine->activateFrame(current);
    } else {
        // Clean out variables
        manager()->watchHandler()->beginCycle();
        manager()->watchHandler()->endCycle();
    }
    manager()->watchHandler()->updateWatchers();
}

void CdbDebugEnginePrivate::updateModules()
{
    QList<Module> modules;
    QString errorMessage;
    if (!getModuleList(m_cif.debugSymbols, &modules, &errorMessage))
        m_engine->warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    manager()->modulesHandler()->setModules(modules);
}

static const char *dumperPrefixC = "dumper";

void CdbDebugEnginePrivate::handleModuleLoad(const QString &name)
{
    if (debugCDB>2)
        qDebug() << Q_FUNC_INFO << "\n    " << name;
    m_dumper->moduleLoadHook(name, m_hDebuggeeProcess);
    updateModules();
}

void CdbDebugEnginePrivate::handleBreakpointEvent(PDEBUG_BREAKPOINT2 pBP)
{
    Q_UNUSED(pBP)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
}

void CdbDebugEngine::reloadSourceFiles()
{
}

QStringList CdbDebugEnginePrivate::sourcePaths() const
{
    WCHAR wszBuf[MAX_PATH];
    if (SUCCEEDED(m_cif.debugSymbols->GetSourcePathWide(wszBuf, MAX_PATH, 0)))
        return QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf)).split(QLatin1Char(';'));
    return QStringList();
}

void CdbDebugEngine::syncDebuggerPaths()
{
     if (debugCDB)
        qDebug() << Q_FUNC_INFO << m_d->m_options->symbolPaths << m_d->m_options->sourcePaths;
    QString errorMessage;
    if (!m_d->setSourcePaths(m_d->m_options->sourcePaths, &errorMessage)
        || !m_d->setSymbolPaths(m_d->m_options->symbolPaths, &errorMessage)) {
        errorMessage = QString::fromLatin1("Unable to set the debugger paths: %1").arg(errorMessage);
        warning(errorMessage);
    }
}

static inline QString pathString(const QStringList &s)
{  return s.join(QString(QLatin1Char(';')));  }

bool CdbDebugEnginePrivate::setSourcePaths(const QStringList &s, QString *errorMessage)
{
    const HRESULT hr = m_cif.debugSymbols->SetSourcePathWide(reinterpret_cast<PCWSTR>(pathString(s).utf16()));
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage = msgComFailed("SetSourcePathWide", hr);
        return false;
    }
    return true;
}

QStringList CdbDebugEnginePrivate::symbolPaths() const
{
    WCHAR wszBuf[MAX_PATH];
    if (SUCCEEDED(m_cif.debugSymbols->GetSymbolPathWide(wszBuf, MAX_PATH, 0)))
        return QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf)).split(QLatin1Char(';'));
    return QStringList();
}

bool CdbDebugEnginePrivate::setSymbolPaths(const QStringList &s, QString *errorMessage)
{
    const HRESULT hr = m_cif.debugSymbols->SetSymbolPathWide(reinterpret_cast<PCWSTR>(pathString(s).utf16()));
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage = msgComFailed("SetSymbolPathWide", hr);
        return false;
    }
    return true;
}

// Accessed by DebuggerManager
IDebuggerEngine *createWinEngine(DebuggerManager *parent,
                                 bool cmdLineEnabled,
                                 QList<Core::IOptionsPage*> *opts)
{
    // Create options page
    QSharedPointer<CdbOptions> options(new CdbOptions);
    options->fromSettings(Core::ICore::instance()->settings());
    CdbOptionsPage *optionsPage = new CdbOptionsPage(options);
    opts->push_back(optionsPage);
    if (!cmdLineEnabled || !options->enabled)
        return 0;
    // Create engine
    QString errorMessage;
    IDebuggerEngine *engine = CdbDebugEngine::create(parent, options, &errorMessage);
    if (!engine) {
        optionsPage->setFailureMessage(errorMessage);
        qWarning("%s\n" ,qPrintable(errorMessage));
    }
    QObject::connect(optionsPage, SIGNAL(debuggerPathsChanged()), engine, SLOT(syncDebuggerPaths()));
    return engine;
}

} // namespace Internal
} // namespace Debugger

