/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "extensioncontext.h"
#include "symbolgroup.h"
#include "eventcallback.h"
#include "outputcallback.h"
#include "stringutils.h"

// wdbgexts.h declares 'extern WINDBG_EXTENSION_APIS ExtensionApis;'
// and it's inline functions rely on its existence.
WINDBG_EXTENSION_APIS   ExtensionApis = {sizeof(WINDBG_EXTENSION_APIS), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const char *ExtensionContext::stopReasonKeyC = "reason";

ExtensionContext::ExtensionContext() :
    m_hookedClient(0),
    m_oldEventCallback(0), m_oldOutputCallback(0),
    m_creatorEventCallback(0), m_creatorOutputCallback(0)
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
    completeStopReasons(ExtensionContext::StopReasonMap stopReasons, ULONG ex)
{
    typedef ExtensionContext::StopReasonMap::value_type StopReasonMapValue;

    stopReasons.insert(StopReasonMapValue(std::string("executionStatus"), toString(ex)));

    IInterfacePointer<CIDebugClient> client;
    if (client.create()) {
        if (const ULONG processId = currentProcessId(client.data()))
            stopReasons.insert(StopReasonMapValue(std::string("processId"), toString(processId)));
        const ULONG threadId = currentThreadId(client.data());
        stopReasons.insert(StopReasonMapValue(std::string("threadId"), toString(threadId)));
    }
    // Any reason?
    const std::string reasonKey = std::string(ExtensionContext::stopReasonKeyC);
    if (stopReasons.find(reasonKey) == stopReasons.end())
        stopReasons.insert(StopReasonMapValue(reasonKey, "unknown"));
    return stopReasons;
}

void ExtensionContext::notifyIdle()
{
    discardSymbolGroup();

    const StopReasonMap stopReasons = completeStopReasons(m_stopReason, executionStatus());
    m_stopReason.clear();
    // Format
    std::ostringstream str;
    formatGdbmiHash(str, stopReasons);
    report('E', 0, "session_idle", "%s", str.str().c_str());
    m_stopReason.clear();
}

void ExtensionContext::notifyState(ULONG Notify)
{
    const ULONG ex = executionStatus();
    switch (Notify) {
    case DEBUG_NOTIFY_SESSION_ACTIVE:
        report('E', 0, "session_active", "%u", ex);
        break;
    case DEBUG_NOTIFY_SESSION_ACCESSIBLE: // Meaning, commands accepted
        report('E', 0, "session_accessible", "%u", ex);
        break;
    case DEBUG_NOTIFY_SESSION_INACCESSIBLE:
        report('E', 0, "session_inaccessible", "%u", ex);
        break;
    case DEBUG_NOTIFY_SESSION_INACTIVE:
        report('E', 0, "session_inactive", "%u", ex);
        discardSymbolGroup();
        // We lost the debuggee, at this point restore output.
        if (ex & DEBUG_STATUS_NO_DEBUGGEE)
            unhookCallbacks();
        break;
    }
}

SymbolGroup *ExtensionContext::symbolGroup(CIDebugSymbols *symbols, ULONG threadId, int frame, std::string *errorMessage)
{
    if (m_symbolGroup.get() && m_symbolGroup->frame() == frame && m_symbolGroup->threadId() == threadId)
        return m_symbolGroup.get();
    SymbolGroup *newSg = SymbolGroup::create(m_control.data(), symbols, threadId, frame, errorMessage);
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

void ExtensionContext::discardSymbolGroup()
{
    if (m_symbolGroup.get())
        m_symbolGroup.reset();
}

bool ExtensionContext::report(char code, int token, const char *serviceName, PCSTR Format, ...)
{
    if (!isInitialized())
        return false;
    // '<qtcreatorcdbext>|R|<token>|<serviceName>|<one-line-output>'.
    m_control->Output(DEBUG_OUTPUT_NORMAL, "<qtcreatorcdbext>|%c|%d|%s|", code, token, serviceName);
    va_list Args;
    va_start(Args, Format);
    m_control->OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);
    m_control->Output(DEBUG_OUTPUT_NORMAL, "\n");
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

// -------- ExtensionCommandContext

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
