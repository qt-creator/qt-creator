/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "extensioncontext.h"
#include "symbolgroup.h"
#include "eventcallback.h"
#include "outputcallback.h"
#include "stringutils.h"
#include "gdbmihelpers.h"

#include <algorithm>

// wdbgexts.h declares 'extern WINDBG_EXTENSION_APIS ExtensionApis;'
// and it's inline functions rely on its existence.
WINDBG_EXTENSION_APIS   ExtensionApis = {sizeof(WINDBG_EXTENSION_APIS), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const char *ExtensionContext::stopReasonKeyC = "reason";
const char *ExtensionContext::breakPointStopReasonC = "breakpoint";

/*!  \class Parameters

    Externally configureable parameters.
    \ingroup qtcreatorcdbext
*/

Parameters::Parameters() : maxStringLength(10000), maxStackDepth(1000)
{
}

/*!  \class ExtensionContext

    Global singleton with context.
    Caches a symbolgroup per frame and thread as long as the session is accessible.
    \ingroup qtcreatorcdbext
*/

ExtensionContext::ExtensionContext() :
    m_hookedClient(0),
    m_oldEventCallback(0), m_oldOutputCallback(0),
    m_creatorEventCallback(0), m_creatorOutputCallback(0), m_stateNotification(true)
{
}

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

bool ExtensionContext::call(const std::string &functionCall,
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
        return 0;
    }
    // Execute in current thread. TODO: This must not crash, else we are in an inconsistent state
    // (need to call 'gh', etc.)
    hr = m_control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "~. g", DEBUG_EXECUTE_ECHO);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("Execute", hr);
        return 0;
    }
    // Wait until finished
    startRecordingOutput();
    m_stateNotification = false;
    m_control->WaitForEvent(0, INFINITE);
    *output =  stopRecordingOutput();
    m_stateNotification = true;
    // Crude attempt at recovering from a crash: Issue 'gN' (go with exception not handled).
    const bool crashed = output->find(L"This exception may be expected and handled.") != std::string::npos;
    if (crashed) {
        m_stopReason.clear();
        m_stateNotification = false;
        hr = m_control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "~. gN", DEBUG_EXECUTE_ECHO);
        m_control->WaitForEvent(0, INFINITE);
        m_stateNotification = true;
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
