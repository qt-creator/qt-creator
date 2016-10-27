/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "extensioncontext.h"
#include "symbolgroup.h"
#include "symbolgroupvalue.h"
#include "eventcallback.h"
#include "outputcallback.h"
#include "stringutils.h"
#include "gdbmihelpers.h"

#include <algorithm>

#ifdef WITH_PYTHON
#include <Python.h>
#include "pycdbextmodule.h"
#endif

// wdbgexts.h declares 'extern WINDBG_EXTENSION_APIS ExtensionApis;'
// and it's inline functions rely on its existence.
WINDBG_EXTENSION_APIS   ExtensionApis = {sizeof(WINDBG_EXTENSION_APIS), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const char *ExtensionContext::stopReasonKeyC = "reason";
const char *ExtensionContext::breakPointStopReasonC = "breakpoint";

/*!  \struct Parameters

    Externally configureable parameters.
    \ingroup qtcreatorcdbext
*/

/*!  \class StateNotificationBlocker

    Blocks state (stopped) notification of ExtensionContext while instantiated

    \ingroup qtcreatorcdbext
*/

class StateNotificationBlocker {
    StateNotificationBlocker(const StateNotificationBlocker &);
    StateNotificationBlocker &operator=(const StateNotificationBlocker &);

public:
    StateNotificationBlocker(ExtensionContext *ec)
        : m_oldValue(ec->stateNotification())
        , m_extensionContext(ec)
        { m_extensionContext->setStateNotification(false); }
    ~StateNotificationBlocker() { m_extensionContext->setStateNotification(m_oldValue); }

private:
    const bool m_oldValue;
    ExtensionContext *m_extensionContext;
};

/*!  \class ExtensionContext

    Global singleton with context.
    Caches a symbolgroup per frame and thread as long as the session is accessible.
    \ingroup qtcreatorcdbext
*/

ExtensionContext::~ExtensionContext()
{
    unhookCallbacks();
}

ExtensionContext &ExtensionContext::instance()
{
    static ExtensionContext extContext;
    return extContext;
}

// Redirect the event/output callbacks
void ExtensionContext::hookCallbacks(CIDebugClient *client)
{
    if (!client || m_hookedClient || m_creatorEventCallback)
        return;
    // Store the hooked client. Any other client obtained
    // is invalid for unhooking
    m_hookedClient = client;
    if (client->GetEventCallbacks(&m_oldEventCallback) == S_OK) {
        m_creatorEventCallback = new EventCallback(m_oldEventCallback);
        client->SetEventCallbacks(m_creatorEventCallback);
    }
    if (client->GetOutputCallbacksWide(&m_oldOutputCallback) == S_OK) {
        m_creatorOutputCallback = new OutputCallback(m_oldOutputCallback);
        client->SetOutputCallbacksWide(m_creatorOutputCallback);
    }
}

void ExtensionContext::startRecordingOutput()
{
    if (m_creatorOutputCallback)
        m_creatorOutputCallback->startRecording();
    else
        report('X', 0, 0, "Error", "ExtensionContext::startRecordingOutput() called with no output hooked.\n");
}

std::wstring ExtensionContext::stopRecordingOutput()
{
    return m_creatorOutputCallback ? m_creatorOutputCallback->stopRecording() : std::wstring();
}

void ExtensionContext::setStopReason(const StopReasonMap &r, const std::string &reason)
{
    m_stopReason = r;
    if (!reason.empty())
        m_stopReason.insert(StopReasonMap::value_type(stopReasonKeyC, reason));
}

// Restore the callbacks.
void ExtensionContext::unhookCallbacks()
{
    if (!m_hookedClient || (!m_creatorEventCallback && !m_creatorOutputCallback))
        return;

    if (m_creatorEventCallback) {
        m_hookedClient->SetEventCallbacks(m_oldEventCallback);
        delete m_creatorEventCallback;
        m_creatorEventCallback = 0;
        m_oldEventCallback = 0;
    }

    if (m_creatorOutputCallback) {
        m_hookedClient->SetOutputCallbacksWide(m_oldOutputCallback);
        delete m_creatorOutputCallback;
        m_creatorOutputCallback = 0;
        m_oldOutputCallback  = 0;
    }
    m_hookedClient = 0;
}

HRESULT ExtensionContext::initialize(PULONG Version, PULONG Flags)
{
    if (isInitialized())
        return S_OK;

    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;

#ifdef WITH_PYTHON
    initCdbextPythonModule();
    Py_Initialize();
    PyRun_SimpleString("import cdbext");
    PyRun_SimpleString("import sys");
#endif

    IInterfacePointer<CIDebugClient> client;
    if (!client.create())
        return client.hr();
    m_control.create(client.data());
    if (!m_control)
        return m_control.hr();
    return m_control->GetWindbgExtensionApis64(&ExtensionApis);
}

bool ExtensionContext::isInitialized() const
{
    return ExtensionApis.lpOutputRoutine != 0;
}

ULONG ExtensionContext::executionStatus() const
{
    ULONG ex = 0;
    return (m_control && SUCCEEDED(m_control->GetExecutionStatus(&ex))) ? ex : ULONG(0);
}

// Helpers for finding the address of the JS execution context in
// case of a QML crash: Find module
static std::string findModule(CIDebugSymbols *syms,
                              const std::string &name,
                              std::string *errorMessage)
{
    const Modules mods = getModules(syms, errorMessage);
    const size_t count = mods.size();
    for (size_t m = 0; m < count; ++m)
        if (!mods.at(m).name.compare(0, name.size(), name))
            return mods.at(m).name;
    return std::string();
}

// Try to find a JS execution context passed as parameter in a complete stack dump (kp)
static ULONG64 jsExecutionEngineFromStackTrace(const std::wstring &stack)
{
    // Search for "QV4::ExecutionEngine * - varying variable names - 0x...[,)]"
    const wchar_t needle[] = L"struct QV4::ExecutionEngine * "; // Qt 5.7 onwards
    std::string::size_type varEnd = std::string::npos;
    std::string::size_type varPos = stack.find(needle);
    if (varPos != std::string::npos) {
        varEnd = varPos + sizeof(needle) / sizeof(wchar_t) - 1;
    } else {
        const wchar_t needle56[] = L"struct QV4::ExecutionContext * "; // up to Qt 5.6
        varPos = stack.find(needle56);
        if (varPos != std::string::npos)
            varEnd = varPos + sizeof(needle56) / sizeof(wchar_t) - 1;
    }
    if (varEnd == std::string::npos)
        return 0;
    std::string::size_type numPos = stack.find(L"0x", varEnd);
    if (numPos == std::string::npos || numPos > (varEnd + 20))
        return 0;
    numPos += 2;
    const std::string::size_type endPos = stack.find_first_of(L",)", numPos);
    if (endPos == std::string::npos)
        return 0;
    // Fix hex values: (0x)000000f5`cecae5b0 -> (0x)000000f5cecae5b0
    std::wstring address = stack.substr(numPos, endPos - numPos);
    if (address.size() > 8 && address.at(8) == L'`')
        address.erase(8, 1);
    std::wistringstream str(address);
    ULONG64 result;
    str >> std::hex >> result;
    return str.fail() ? 0 : result;
}

// Try to find address of jsExecutionEngine by looking at the
// stack trace in case QML is loaded.
ULONG64 ExtensionContext::jsExecutionEngine(ExtensionCommandContext &exc,
                                            std::string *errorMessage)
{

    const QtInfo &qtInfo = QtInfo::get(SymbolGroupValueContext(exc.dataSpaces(), exc.symbols()));
    static const std::string qmlModule =
        findModule(exc.symbols(), qtInfo.moduleName(QtInfo::Qml), errorMessage);
    if (qmlModule.empty()) {
        if (errorMessage->empty())
            *errorMessage = "QML not loaded";
        return 0;
    }
    // Retrieve top frames of stack and try to find a JS execution engine passed as parameter
    startRecordingOutput();
    StateNotificationBlocker blocker(this);
    const HRESULT hr = m_control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "kp 15", DEBUG_EXECUTE_ECHO);
    if (FAILED(hr)) {
        stopRecordingOutput();
        *errorMessage = msgDebugEngineComFailed("Execute", hr);
        return 0;
    }
    const std::wstring fullStackTrace = stopRecordingOutput();
    if (fullStackTrace.empty()) {
        *errorMessage = "Unable to obtain stack (output redirection in place?)";
        return 0;
    }
    const ULONG64 result = jsExecutionEngineFromStackTrace(fullStackTrace);
    if (!result)
        *errorMessage = "JS ExecutionEngine address not found in stack";
    return result;
}

ExtensionContext::CdbVersion ExtensionContext::cdbVersion()
{
    static CdbVersion version;
    static bool first = true;
    if (!first)
        return version;
    first = false;
    startRecordingOutput();
    const HRESULT hr = m_control->OutputVersionInformation(DEBUG_OUTCTL_ALL_CLIENTS);
    if (FAILED(hr)) {
        stopRecordingOutput();
        return version;
    }
    const std::wstring &output = stopRecordingOutput();
    const std::wstring &versionOutput = L"Microsoft (R) Windows Debugger Version";
    auto majorPos = output.find(versionOutput);
    if (majorPos == std::wstring::npos)
        return version;
    majorPos += versionOutput.length();
    std::wstring::size_type minorPos;
    std::wstring::size_type patchPos;
    try {
        version.major = std::stoi(output.substr(majorPos), &minorPos);
        minorPos += majorPos + 1;
        version.minor = std::stoi(output.substr(minorPos), &patchPos);
        patchPos += minorPos + 1;
        version.patch = std::stoi(output.substr(patchPos));
    }
    catch (...)
    {
        version.clear();
    }
    return version;
}

// Complete stop parameters with common parameters and report
static inline ExtensionContext::StopReasonMap
    completeStopReasons(CIDebugClient *client, ExtensionContext::StopReasonMap stopReasons, ULONG ex)
{
    typedef ExtensionContext::StopReasonMap::value_type StopReasonMapValue;

    stopReasons.insert(StopReasonMapValue(std::string("executionStatus"), toString(ex)));

    if (const ULONG processId = currentProcessId(client))
        stopReasons.insert(StopReasonMapValue(std::string("processId"), toString(processId)));
    const ULONG threadId = currentThreadId(client);
    stopReasons.insert(StopReasonMapValue(std::string("threadId"), toString(threadId)));
    // Any reason?
    const std::string reasonKey = std::string(ExtensionContext::stopReasonKeyC);
    if (stopReasons.find(reasonKey) == stopReasons.end())
        stopReasons.insert(StopReasonMapValue(reasonKey, "unknown"));
    return stopReasons;
}

void ExtensionContext::notifyIdleCommand(CIDebugClient *client)
{
    discardSymbolGroup();
    if (m_stateNotification) {
        // Format full thread and stack info along with completed stop reasons.
        std::string errorMessage;
        ExtensionCommandContext exc(client);
        const StopReasonMap stopReasons = completeStopReasons(client, m_stopReason, executionStatus());
        // Format
        std::ostringstream str;
        formatGdbmiHash(str, stopReasons, false);
        const std::string threadInfo = gdbmiThreadList(exc.systemObjects(), exc.symbols(),
                                                       exc.control(), exc.advanced(), &errorMessage);
        if (threadInfo.empty())
            str << ",threaderror=" << gdbmiStringFormat(errorMessage);
        else
            str << ",threads=" << threadInfo;
        const std::string stackInfo = gdbmiStack(exc.control(), exc.symbols(),
                                                 ExtensionContext::instance().parameters().maxStackDepth,
                                                 false, &errorMessage);
        if (stackInfo.empty())
            str << ",stackerror=" << gdbmiStringFormat(errorMessage);
        else
            str << ",stack=" << stackInfo;
        str << '}';
        reportLong('E', 0, "session_idle", str.str());
    }
    m_stopReason.clear();
}

void ExtensionContext::notifyState(ULONG Notify)
{
    const ULONG ex = executionStatus();
    if (m_stateNotification) {
        switch (Notify) {
        case DEBUG_NOTIFY_SESSION_ACTIVE:
            report('E', 0, 0, "session_active", "%u", ex);
            break;
        case DEBUG_NOTIFY_SESSION_ACCESSIBLE: // Meaning, commands accepted
            report('E', 0, 0, "session_accessible", "%u", ex);
            break;
        case DEBUG_NOTIFY_SESSION_INACCESSIBLE:
            report('E', 0, 0, "session_inaccessible", "%u", ex);
            break;
        case DEBUG_NOTIFY_SESSION_INACTIVE:
            report('E', 0, 0, "session_inactive", "%u", ex);
            break;
        }
    }
    if (Notify == DEBUG_NOTIFY_SESSION_INACTIVE) {
        discardSymbolGroup();
        discardWatchesSymbolGroup();
        // We lost the debuggee, at this point restore output.
        if (ex & DEBUG_STATUS_NO_DEBUGGEE)
            unhookCallbacks();
    }
}

LocalsSymbolGroup *ExtensionContext::symbolGroup(CIDebugSymbols *symbols, ULONG threadId, int frame, std::string *errorMessage)
{
    if (m_symbolGroup.get() && m_symbolGroup->frame() == frame && m_symbolGroup->threadId() == threadId)
        return m_symbolGroup.get();
    LocalsSymbolGroup *newSg = LocalsSymbolGroup::create(m_control.data(), symbols, threadId, frame, errorMessage);
    if (!newSg)
        return 0;
    m_symbolGroup.reset(newSg);
    return newSg;
}

int ExtensionContext::symbolGroupFrame() const
{
    if (m_symbolGroup.get())
        return m_symbolGroup->frame();
    return -1;
}

WatchesSymbolGroup *ExtensionContext::watchesSymbolGroup() const
{
    if (m_watchesSymbolGroup.get())
        return m_watchesSymbolGroup.get();
    return 0;
}

WatchesSymbolGroup *ExtensionContext::watchesSymbolGroup(CIDebugSymbols *symbols, std::string *errorMessage)
{
    if (m_watchesSymbolGroup.get())
        return m_watchesSymbolGroup.get();
    WatchesSymbolGroup *newSg = WatchesSymbolGroup::create(symbols, errorMessage);
    if (!newSg)
        return 0;
    m_watchesSymbolGroup.reset(newSg);
    return newSg;
}

void ExtensionContext::discardSymbolGroup()
{
    if (m_symbolGroup.get())
        m_symbolGroup.reset();
}

void ExtensionContext::discardWatchesSymbolGroup()
{
    if (m_watchesSymbolGroup.get())
        m_watchesSymbolGroup.reset();
}

bool ExtensionContext::report(char code, int token, int remainingChunks, const char *serviceName, PCSTR Format, ...)
{
    if (!isInitialized())
        return false;
    // '<qtcreatorcdbext>|R|<token>|<serviceName>|<one-line-output>'.
    m_control->Output(DEBUG_OUTPUT_NORMAL, "<qtcreatorcdbext>|%c|%d|%d|%s|",
                      code, token, remainingChunks, serviceName);
    va_list Args;
    va_start(Args, Format);
    m_control->OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);
    m_control->Output(DEBUG_OUTPUT_NORMAL, "\n");
    return true;
}

bool ExtensionContext::reportLong(char code, int token, const char *serviceName, const std::string &message)
{
    const std::string::size_type size = message.size();
    if (size < outputChunkSize)
        return report(code, token, 0, serviceName, "%s", message.c_str());
    // Split up
    std::string::size_type chunkCount = size / outputChunkSize;
    if (size % outputChunkSize)
        chunkCount++;
    std::string::size_type pos = 0;
    for (int remaining = int(chunkCount) -  1; remaining >= 0 ; remaining--) {
        std::string::size_type nextPos = pos + outputChunkSize; // No consistent std::min/std::max in VS8/10
        if (nextPos > size)
            nextPos = size;
        report(code, token, remaining, serviceName, "%s", message.substr(pos, nextPos - pos).c_str());
        pos = nextPos;
    }
    return true;
}

static const char *goCommandForCall(unsigned callFlags)
{
    if (callFlags & ExtensionContext::CallWithExceptionsHandled)
        return "~. gh";
    else if (callFlags & ExtensionContext::CallWithExceptionsNotHandled)
        return "~. gN";
    return "~. g";
}

bool ExtensionContext::call(const std::string &functionCall,
                            unsigned callFlags,
                            std::wstring *output,
                            std::string *errorMessage)
{
    if (!m_creatorOutputCallback) {
        *errorMessage = "Attempt to issue a call with no output hooked.";
        return false;
    }
    // Set up arguments
    const std::string call = ".call " + functionCall;
    HRESULT hr = m_control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, call.c_str(), DEBUG_EXECUTE_ECHO);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("Execute", hr);
        return false;
    }
    // Execute in current thread. TODO: This must not crash, else we are in an inconsistent state
    // (need to call 'gh', etc.)
    hr = m_control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, goCommandForCall(callFlags), DEBUG_EXECUTE_ECHO);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("Execute", hr);
        return false;
    }
    // Wait until finished
    startRecordingOutput();
    StateNotificationBlocker blocker(this);
    m_control->WaitForEvent(0, INFINITE);
    *output =  stopRecordingOutput();
    // Crude attempt at recovering from a crash: Issue 'gN' (go with exception not handled).
    const bool crashed = output->find(L"This exception may be expected and handled.") != std::string::npos;
    if (crashed && !callFlags) {
        m_stopReason.clear();
        hr = m_control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, goCommandForCall(CallWithExceptionsNotHandled), DEBUG_EXECUTE_ECHO);
        m_control->WaitForEvent(0, INFINITE);
        *errorMessage = "A crash occurred while calling: " + functionCall;
        return false;
    }
    return true;
}

// Exported C-functions
extern "C" {

HRESULT CALLBACK DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    return ExtensionContext::instance().initialize(Version, Flags);
}

void CALLBACK DebugExtensionUninitialize(void)
{
#ifdef WITH_PYTHON
    Py_Finalize();
#endif
}

void CALLBACK DebugExtensionNotify(ULONG Notify, ULONG64)
{
    ExtensionContext::instance().notifyState(Notify);
}

} // extern "C"

/*!  \class ExtensionCommandContext

    Context for extension commands to be instantiated on stack in a command handler.
    Provides the IDebug objects on demand. \ingroup qtcreatorcdbext
*/

ExtensionCommandContext *ExtensionCommandContext::m_instance = 0;

ExtensionCommandContext::ExtensionCommandContext(CIDebugClient *client) : m_client(client)
{
    ExtensionCommandContext::m_instance = this;
}

ExtensionCommandContext::~ExtensionCommandContext()
{
    ExtensionCommandContext::m_instance = 0;
}

CIDebugControl *ExtensionCommandContext::control()
{
    if (!m_control)
        m_control.create(m_client);
    return m_control.data();
}

ExtensionCommandContext *ExtensionCommandContext::instance()
{
    return m_instance;
}

CIDebugSymbols *ExtensionCommandContext::symbols()
{
    if (!m_symbols)
        m_symbols.create(m_client);
    return m_symbols.data();
}

CIDebugSystemObjects *ExtensionCommandContext::systemObjects()
{
    if (!m_systemObjects)
        m_systemObjects.create(m_client);
    return m_systemObjects.data();
}

CIDebugAdvanced *ExtensionCommandContext::advanced()
{
    if (!m_advanced)
        m_advanced.create(m_client);
    return m_advanced.data();
}

CIDebugRegisters *ExtensionCommandContext::registers()
{
    if (!m_registers)
        m_registers.create(m_client);
    return m_registers.data();
}

CIDebugDataSpaces *ExtensionCommandContext::dataSpaces()
{
    if (!m_dataSpaces)
        m_dataSpaces.create(m_client);
    return m_dataSpaces.data();
}

ULONG ExtensionCommandContext::threadId()
{
    if (CIDebugSystemObjects *so = systemObjects()) {
        ULONG threadId = 0;
        if (SUCCEEDED(so->GetCurrentThreadId(&threadId)))
            return threadId;
    }
    return 0;
}
