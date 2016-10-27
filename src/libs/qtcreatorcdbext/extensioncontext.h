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

#pragma once

#include "common.h"
#include "iinterfacepointer.h"

#include <memory>
#include <map>

class LocalsSymbolGroup;
class WatchesSymbolGroup;
class OutputCallback;
class ExtensionCommandContext;

// Global parameters
struct Parameters
{
    unsigned maxStringLength = 10000;
    unsigned maxArraySize = 100;
    unsigned maxStackDepth = 1000;
};

// Global singleton with context.
// Caches a symbolgroup per frame and thread as long as the session is accessible.
class ExtensionContext {
    ExtensionContext(const ExtensionContext&);
    ExtensionContext& operator=(const ExtensionContext&);

    ExtensionContext() = default;
public:
    enum CallFlags {
        CallWithExceptionsHandled = 0x1,
        CallWithExceptionsNotHandled = 0x2
    };

    // Key used to report stop reason in StopReasonMap
    static const char *stopReasonKeyC;
    static const char *breakPointStopReasonC;  // pre-defined stop reasons
    // Map of parameters reported with the next stop as GDBMI
    typedef std::map<std::string, std::string> StopReasonMap;

    ~ExtensionContext();

    static ExtensionContext &instance();
    // Call from DLL initialization.
    HRESULT initialize(PULONG Version, PULONG Flags);

    // Hook up our Event and Output callbacks that report exceptions and events.
    // Call this from the first extension command that gets a client.
    // Does not work when called from initialization.
    void hookCallbacks(CIDebugClient *client);
    // Undo hooking.
    void unhookCallbacks();

     // CDB has a limitation on output, so, long messages need to be split (empirically ca 10KB)
    static const size_t outputChunkSize = 10240;
    /* Report output in standardized format understood by Qt Creator.
     * '<qtcreatorcdbext>|R|<token>|remainingChunks|<serviceName>|<one-line-output>'.
     * Char code is 'R' command reply, 'N' command fail, 'E' event notification,
     * 'X' exception, error. If the message is larger than outputChunkSize,
     * it needs to be split up in chunks, remainingChunks needs to indicate the number
     * of the following chunks (0 for just one chunk). */
    bool report(char code, int remainingChunks, int token, const char *serviceName, PCSTR Format, ...);
    // Convenience for reporting potentially long messages in chunks
    bool reportLong(char code, int token, const char *serviceName, const std::string &message);
    ULONG executionStatus() const;
    // Call from notify handler, tell engine about state.
    void notifyState(ULONG Notify);
    // register as '.idle_cmd' to notify creator about stop
    void notifyIdleCommand(CIDebugClient *client);

    // Return symbol group for frame (cached as long as frame/thread do not change).
    LocalsSymbolGroup *symbolGroup(CIDebugSymbols *symbols, ULONG threadId, int frame, std::string *errorMessage);
    int symbolGroupFrame() const;
    void discardSymbolGroup();

    WatchesSymbolGroup *watchesSymbolGroup(CIDebugSymbols *symbols, std::string *errorMessage);
    WatchesSymbolGroup *watchesSymbolGroup() const; // Do not create.
    void discardWatchesSymbolGroup();

    // Set a stop reason to be reported with the next idle notification (exception).
    void setStopReason(const StopReasonMap &, const std::string &reason = std::string());

    void startRecordingOutput();
    std::wstring stopRecordingOutput();
    // Execute a function call and record the output.
    bool call(const std::string &functionCall, unsigned callFlags, std::wstring *output, std::string *errorMessage);

    CIDebugClient *hookedClient() const { return m_hookedClient; }

    const Parameters &parameters() const { return m_parameters; }
    Parameters &parameters() { return m_parameters; }

    ULONG64 jsExecutionEngine(ExtensionCommandContext &exc, std::string *errorMessage);

    bool stateNotification() const { return m_stateNotification; }
    void setStateNotification(bool s) { m_stateNotification = s; }

    struct CdbVersion
    {
        int major = 0;
        int minor = 0;
        int patch = 0;
        void clear () { major = minor = patch = 0; }
    };

    CdbVersion cdbVersion();

private:
    bool isInitialized() const;

    IInterfacePointer<CIDebugControl> m_control;
    std::auto_ptr<LocalsSymbolGroup> m_symbolGroup;
    std::auto_ptr<WatchesSymbolGroup> m_watchesSymbolGroup;

    CIDebugClient *m_hookedClient = nullptr;
    IDebugEventCallbacks *m_oldEventCallback = nullptr;
    IDebugOutputCallbacksWide *m_oldOutputCallback = nullptr;
    IDebugEventCallbacks *m_creatorEventCallback = nullptr;
    OutputCallback *m_creatorOutputCallback = nullptr;

    StopReasonMap m_stopReason;
    bool m_stateNotification = true;
    Parameters m_parameters;
};

// Context for extension commands to be instantiated on stack in a command handler.
// Provides the IDebug objects on demand.
class ExtensionCommandContext
{
    ExtensionCommandContext(const ExtensionCommandContext&);
    ExtensionCommandContext &operator=(const ExtensionCommandContext&);
public:
    explicit ExtensionCommandContext(CIDebugClient *Client);
    ~ExtensionCommandContext();

    // For accessing outside commands. Might return 0, based on the
    // assumption that there is only once instance active.
    static ExtensionCommandContext *instance();

    CIDebugControl *control();
    CIDebugSymbols *symbols();
    CIDebugSystemObjects *systemObjects();
    CIDebugAdvanced *advanced();
    CIDebugRegisters *registers();
    CIDebugDataSpaces *dataSpaces();

    ULONG threadId();

private:
    static ExtensionCommandContext *m_instance;

    CIDebugClient *m_client;
    IInterfacePointer<CIDebugControl> m_control;
    IInterfacePointer<CIDebugSymbols> m_symbols;
    IInterfacePointer<CIDebugAdvanced> m_advanced;
    IInterfacePointer<CIDebugSystemObjects> m_systemObjects;
    IInterfacePointer<CIDebugRegisters> m_registers;
    IInterfacePointer<CIDebugDataSpaces> m_dataSpaces;
};
