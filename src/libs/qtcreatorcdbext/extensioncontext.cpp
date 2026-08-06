// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensioncontext.h"
#include "symbolgroup.h"
#include "symbolgroupvalue.h"
#include "eventcallback.h"
#include "outputcallback.h"
#include "stringutils.h"
#include "gdbmihelpers.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

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

class ExceptionReportBlocker
{
    ExceptionReportBlocker(const ExceptionReportBlocker &);
    ExceptionReportBlocker &operator=(const ExceptionReportBlocker &);

public:
    ExceptionReportBlocker(ExtensionContext *ec)
        : m_oldValue(ec->exceptionReporting())
        , m_extensionContext(ec)
        { m_extensionContext->setExceptionReporting(false); }
    ~ExceptionReportBlocker() { m_extensionContext->setExceptionReporting(m_oldValue); }

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
    PyStatus status;
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }
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
        *errorMessage = "A crash occurred while calling: " + functionCall;
        hr = m_control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, goCommandForCall(CallWithExceptionsNotHandled), DEBUG_EXECUTE_ECHO);
        startRecordingOutput();
        m_control->WaitForEvent(0, INFINITE);
        // if we encounter a second chance exception while calling reset the call stack
        if (stopRecordingOutput().find(L"!!! second chance !!!") != std::string::npos)
            m_control->Execute(DEBUG_OUTCTL_IGNORE, ".call /C", DEBUG_EXECUTE_ECHO);
        return false;
    }
    return true;
}

bool ExtensionContext::allocateMemory(unsigned long size, ULONG64 *address,
                                      std::string *errorMessage)
{
    // DbgEng has no direct allocator; '.dvalloc' reserves inferior memory.
    if (!m_creatorOutputCallback) {
        *errorMessage = "Attempt to allocate with no output hooked.";
        return false;
    }
    const std::string cmd = ".dvalloc " + std::to_string(size);
    startRecordingOutput();
    const HRESULT hr = m_control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, cmd.c_str(),
                                          DEBUG_EXECUTE_ECHO);
    const std::wstring output = stopRecordingOutput();
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("Execute", hr);
        return false;
    }
    // Expected: "Allocated <n> bytes starting at <addr>", where <addr> may
    // carry a backtick separator, e.g. 00000250`6f3a0000.
    const std::wstring marker = L"starting at ";
    const std::wstring::size_type pos = output.find(marker);
    if (pos == std::wstring::npos) {
        *errorMessage = "Could not parse '.dvalloc' output";
        return false;
    }
    ULONG64 value = 0;
    bool any = false;
    for (std::wstring::size_type i = pos + marker.size(); i < output.size(); ++i) {
        const wchar_t c = output[i];
        if (c == L'`')
            continue;
        int digit = -1;
        if (c >= L'0' && c <= L'9')
            digit = c - L'0';
        else if (c >= L'a' && c <= L'f')
            digit = c - L'a' + 10;
        else if (c >= L'A' && c <= L'F')
            digit = c - L'A' + 10;
        if (digit < 0) {
            if (any)
                break;
            continue;
        }
        value = (value << 4) | ULONG64(digit);
        any = true;
    }
    if (!any) {
        *errorMessage = "No address in '.dvalloc' output";
        return false;
    }
    *address = value;
    return true;
}

// Reads a register by name into a plain integer.
static bool registerValue(CIDebugRegisters *registers, const char *name, ULONG64 *value,
                          std::string *errorMessage)
{
    ULONG index = 0;
    if (FAILED(registers->GetIndexByName(name, &index))) {
        *errorMessage = std::string("No register named ") + name;
        return false;
    }
    DEBUG_VALUE debugValue;
    if (FAILED(registers->GetValue(index, &debugValue))) {
        *errorMessage = std::string("Could not read register ") + name;
        return false;
    }
    *value = debugValue.I64;
    return true;
}

static bool setRegisterValue(CIDebugRegisters *registers, const char *name, ULONG64 value,
                             std::string *errorMessage)
{
    ULONG index = 0;
    if (FAILED(registers->GetIndexByName(name, &index))) {
        *errorMessage = std::string("No register named ") + name;
        return false;
    }
    DEBUG_VALUE debugValue;
    memset(&debugValue, 0, sizeof(debugValue));
    debugValue.Type = DEBUG_VALUE_INT64;
    debugValue.I64 = value;
    if (FAILED(registers->SetValue(index, &debugValue))) {
        *errorMessage = std::string("Could not write register ") + name;
        return false;
    }
    return true;
}

// Runs a call set up by callWithoutPrototype() until it returns to 'trap'. Any other
// stop leaves the call unfinished, so the caller must not read a return value then.
static bool runToReturnTrap(ExtensionCommandContext *exc, CIDebugRegisters *registers,
                            ULONG callThread, ULONG64 trap, const std::string &functionCall,
                            std::string *errorMessage)
{
    // Second round grants a first-chance exception, as call() does. "~. g" runs the
    // calling thread alone, so no other thread can hit a user breakpoint mid-call.
    for (int round = 0; round < 2; ++round) {
        const unsigned flags = round == 0 ? 0u : ExtensionContext::CallWithExceptionsNotHandled;
        if (FAILED(exc->control()->Execute(DEBUG_OUTCTL_IGNORE, goCommandForCall(flags), 0))) {
            *errorMessage = "Cannot resume the debuggee for the call.";
            return false;
        }
        if (FAILED(exc->control()->WaitForEvent(0, INFINITE))) {
            *errorMessage = "The call never came back.";
            return false;
        }

        // The stop may belong to any thread, and the registers and the thread
        // context are read from whichever one dbgeng made current.
        ULONG stoppedThread = 0;
        if (SUCCEEDED(exc->systemObjects()->GetCurrentThreadId(&stoppedThread))
                && stoppedThread != callThread
                && FAILED(exc->systemObjects()->SetCurrentThreadId(callThread))) {
            *errorMessage = "Cannot return to the thread called in.";
            return false;
        }

        ULONG64 rip = 0;
        if (!registerValue(registers, "rip", &rip, errorMessage))
            return false;
        // Back at the trap, or one byte past it - an int3 leaves rip on either
        // side of itself depending on how the stop is reported.
        if (rip == trap || rip == trap + 1)
            return true;

        ULONG eventType = 0;
        ULONG eventProcess = 0;
        ULONG eventThread = 0;
        DEBUG_LAST_EVENT_INFO_EXCEPTION exception;
        memset(&exception, 0, sizeof(exception));
        ULONG used = 0;
        const bool isException = SUCCEEDED(exc->control()->GetLastEventInformation(
                                     &eventType, &eventProcess, &eventThread, &exception,
                                     sizeof(exception), &used, nullptr, 0, nullptr))
                                 && eventType == DEBUG_EVENT_EXCEPTION;
        if (round == 0 && isException && exception.FirstChance && eventThread == callThread)
            continue;
        if (isException) {
            std::ostringstream str;
            str << "The call raised an exception, code 0x" << std::hex
                << exception.ExceptionRecord.ExceptionCode << ": " << functionCall;
            *errorMessage = str.str();
        } else {
            *errorMessage = "The call stopped before returning: " + functionCall;
        }
        return false;
    }
    return false;
}

bool ExtensionContext::callWithoutPrototype(const std::string &functionCall,
                                           ULONG64 *returnValue, std::string *errorMessage)
{
    // "<module>!<function>(<arg>, ...)", the same text call() takes. The arguments
    // are integers or pointers only - which is what a caller reaching this has,
    // since ".call" rejects string literals in the first place.
    const std::string::size_type parenPos = functionCall.find('(');
    if (parenPos == std::string::npos || functionCall.back() != ')') {
        *errorMessage = "Not a function call: " + functionCall;
        return false;
    }
    const std::string function = functionCall.substr(0, parenPos);
    std::vector<ULONG64> arguments;
    const std::string argumentList = functionCall.substr(parenPos + 1,
                                                         functionCall.size() - parenPos - 2);
    for (std::string::size_type pos = 0; pos < argumentList.size(); ) {
        const std::string::size_type comma = argumentList.find(',', pos);
        const std::string argument = argumentList.substr(
            pos, comma == std::string::npos ? std::string::npos : comma - pos);
        if (argument.find_first_not_of(" \t") != std::string::npos) {
            const std::string::size_type cast = argument.rfind(')');
            const std::string value = cast == std::string::npos ? argument
                                                                : argument.substr(cast + 1);
            char *end = nullptr;
            const ULONG64 parsed = std::strtoull(value.c_str(), &end, 0);
            if (end == value.c_str()) {
                *errorMessage = "Cannot parse the argument \"" + argument + "\" of " + functionCall;
                return false;
            }
            arguments.push_back(parsed);
        }
        if (comma == std::string::npos)
            break;
        pos = comma + 1;
    }
    if (arguments.size() > 4) {
        // Anything beyond the fourth argument goes on the stack, which nothing
        // needs yet - the QML service entry points take two at most.
        *errorMessage = "More than four arguments are not supported: " + functionCall;
        return false;
    }

    ExtensionCommandContext *exc = ExtensionCommandContext::instance();
    if (!exc) {
        *errorMessage = "Attempt to issue a call outside a command context.";
        return false;
    }
    CIDebugRegisters *registers = exc->registers();
    CIDebugDataSpaces *data = exc->dataSpaces();

    ULONG64 address = 0;
    if (FAILED(exc->symbols()->GetOffsetByName(function.c_str(), &address))) {
        *errorMessage = "Cannot resolve " + function;
        return false;
    }

    // The int3 the call returns to. A page of the debuggee's own, so nothing has
    // to be assumed about its code and no breakpoint of the caller's is disturbed.
    // Rechecked each time: the page belongs to a process this context outlives.
    if (m_callReturnTrap) {
        unsigned char trap = 0;
        ULONG read = 0;
        if (FAILED(data->ReadVirtual(m_callReturnTrap, &trap, 1, &read)) || read != 1
                || trap != 0xCC) {
            m_callReturnTrap = 0;
        }
    }
    if (!m_callReturnTrap) {
        if (!allocateMemory(16, &m_callReturnTrap, errorMessage))
            return false;
        unsigned char int3 = 0xCC;
        ULONG written = 0;
        if (FAILED(data->WriteVirtual(m_callReturnTrap, &int3, 1, &written)) || written != 1) {
            m_callReturnTrap = 0;
            *errorMessage = "Cannot write the return trap.";
            return false;
        }
    }

    // The thread this all applies to: the registers set up below, the context put
    // back at the end, and the one the call has to come back on.
    ULONG callThread = 0;
    if (FAILED(exc->systemObjects()->GetCurrentThreadId(&callThread))) {
        *errorMessage = "Cannot determine the thread to call in.";
        return false;
    }

    // Everything the call clobbers, to be put back afterwards. The whole context,
    // since the volatile registers are not all of it - the flags and the SSE
    // registers a callee may use just as well can be live at an arbitrary stop.
    CONTEXT savedContext;
    memset(&savedContext, 0, sizeof(savedContext));
    savedContext.ContextFlags = CONTEXT_FULL;
    if (FAILED(exc->advanced()->GetThreadContext(&savedContext, sizeof(savedContext)))) {
        *errorMessage = "Cannot read the thread context.";
        return false;
    }
    ULONG64 stackPointer = 0;
    if (!registerValue(registers, "rsp", &stackPointer, errorMessage))
        return false;

    // The x64 convention: the arguments in rcx/rdx/r8/r9, and rsp such that it is
    // 16-byte aligned *before* the return address goes on - so the callee sees
    // rsp+8 aligned, as it would after a real "call". Far enough below the current
    // rsp that the frame the callee builds cannot reach anything live.
    const ULONG64 stack = ((stackPointer - 0x200) & ~ULONG64(0xF)) - 8;
    ULONG written = 0;
    if (FAILED(data->WriteVirtual(stack, &m_callReturnTrap, sizeof(m_callReturnTrap), &written))
            || written != sizeof(m_callReturnTrap)) {
        *errorMessage = "Cannot write the return address.";
        return false;
    }
    static const char *argumentRegisters[] = {"rcx", "rdx", "r8", "r9"};
    bool ok = setRegisterValue(registers, "rsp", stack, errorMessage);
    for (std::vector<ULONG64>::size_type i = 0; ok && i < arguments.size(); ++i)
        ok = setRegisterValue(registers, argumentRegisters[i], arguments.at(i), errorMessage);
    if (ok)
        ok = setRegisterValue(registers, "rip", address, errorMessage);

    // Run it. The state notifications are blocked for the same reason call()
    // blocks them: this stop is this extension's own business, and the engine must
    // not see the debuggee running or stopping for it.
    if (ok) {
        StateNotificationBlocker blocker(this);
        ExceptionReportBlocker exceptionBlocker(this);
        ok = runToReturnTrap(exc, registers, callThread, m_callReturnTrap, functionCall,
                             errorMessage);
    }
    if (ok)
        ok = registerValue(registers, "rax", returnValue, errorMessage);

    // Put the thread back as it was, whether the call worked or not: the caller is
    // stopped somewhere it expects to still be. Failing that, it is left wherever
    // the call put it, which is nothing to keep quiet about.
    if (FAILED(exc->advanced()->SetThreadContext(&savedContext, sizeof(savedContext)))) {
        *errorMessage = "Cannot restore the thread context after: " + functionCall;
        ok = false;
    }
    return ok;
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
