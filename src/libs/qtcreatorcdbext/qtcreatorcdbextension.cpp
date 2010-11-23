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

#include "extensioncontext.h"
#include "outputcallback.h"
#include "eventcallback.h"
#include "symbolgroup.h"
#include "stringutils.h"
#include "gdbmihelpers.h"

#include <cstdio>
#include <sstream>
#include <list>
#include <iterator>

/* QtCreatorCDB ext is an extension loaded into CDB.exe (see cdbengine2.cpp):
 * - Notification about the state of the debugging session:
 *   + idle: (hooked with .idle_cmd) debuggee stopped
 *   + accessible: Debuggee stopped, cdb.exe accepts commands
 *   + inaccessible: Debuggee runs, no way to post commands
 *   + session active/inactive: Lost debuggee, terminating.
 * - Hook up with output/event callbacks and produce formatted output
 * - Provide some extension commands that produce output in a standardized (GDBMI)
 *   format that ends up in handleExtensionMessage().
 *   + pid     Return debuggee pid for interrupting.
 *   + locals  Print locals from SymbolGroup
 *   + expandLocals Expand locals in symbol group
 *   + registers, modules, threads */

typedef std::vector<std::string> StringVector;
typedef std::list<std::string> StringList;

// Split & Parse the arguments of a command and extract the
// optional first integer token argument ('command -t <number> remaining arguments')
// Each command takes the 'token' argument and includes it in its output
// so that the calling CDB engine in Qt Creator can match the callback.

template<class StringContainer>
static inline StringContainer commandTokens(PCSTR args, int *token = 0)
{
    typedef StringContainer::iterator ContainerIterator;

    if (token)
        *token = 0;
    std::string cmd(args);
    simplify(cmd);
    StringContainer tokens;
    split(cmd, ' ', std::back_inserter(tokens));

    // Check for token
    ContainerIterator it = tokens.begin();
    if (it != tokens.end() && *it == "-t" && ++it != tokens.end()) {
        if (token)
            std::istringstream(*it) >> *token;
        tokens.erase(tokens.begin(), ++it);
    }
    return tokens;
}

// Extension command 'pid':
// Report back PID so that Qt Creator engine knows whom to DebugBreakProcess.
extern "C" HRESULT CALLBACK pid(CIDebugClient *client, PCSTR args)
{
    ExtensionContext::instance().hookCallbacks(client);

    int token;
    commandTokens<StringList>(args, &token);

    if (const ULONG pid = currentProcessId(client)) {
        ExtensionContext::instance().report('R', token, "pid", "%u", pid);
    } else {
        ExtensionContext::instance().report('N', token, "pid", "0");
    }
    return S_OK;
}

// Extension command 'expandlocals':
// Expand a comma-separated iname-list of local variables.

extern "C" HRESULT CALLBACK expandlocals(CIDebugClient *client, PCSTR args)
{
    ExtensionCommandContext exc(client);
    unsigned frame = 0;
    SymbolGroup *symGroup = 0;
    std::string errorMessage;

    int token;
    const StringVector tokens = commandTokens<StringVector>(args, &token);
    StringVector inames;
    if (tokens.size() == 2u && sscanf(tokens.front().c_str(), "%u", &frame) ==  1) {
        symGroup = ExtensionContext::instance().symbolGroup(exc.symbols(), exc.threadId(), frame, &errorMessage);
        split(tokens.at(1), ',', std::back_inserter(inames));
    } else {
        std::ostringstream str;
        str << "Invalid parameter: '" << args << "' (usage expand <frame> iname1,iname2..).";
        errorMessage = str.str();
    }
    if (symGroup) {
        const unsigned succeeded = symGroup->expandList(inames, &errorMessage);
        ExtensionContext::instance().report('R', token, "expandlocals", "%u/%u %s",
                                            succeeded, unsigned(inames.size()), errorMessage.c_str());
    } else {
        ExtensionContext::instance().report('N', token, "expandlocals", errorMessage.c_str());
    }
    return S_OK;
}

static inline std::string msgLocalsUsage(PCSTR args)
{
    std::ostringstream str;
    str << "Invalid parameter: '" << args
        << "'\nUsage: locals [-t token] [-h] [-d] [-e expandset] [-u uninitializedset] <frame> [iname]).\n"
           "-h human-readable ouput\n"
           "-d debug output\n-e expandset Comma-separated list of expanded inames\n"
           "-u uninitializedset Comma-separated list of uninitialized inames\n";
    return str.str();
}

// Extension command 'locals':
// Display local variables of symbol group in GDBMI or debug output form.
// Takes an optional iname which is expanded before displaying (for updateWatchData)

static std::string commmandLocals(ExtensionCommandContext &exc,PCSTR args, int *token, std::string *errorMessage)
{
    // Parse the command
    unsigned debugOutput = 0;
    bool humanReadableGdbmi = false;
    std::string iname;

    StringList tokens = commandTokens<StringList>(args, token);
    StringVector expandedInames;
    StringVector uninitializedInames;
    // Parse away options
    while (!tokens.empty() && tokens.front().size() == 2 && tokens.front().at(0) == '-') {
        const char option = tokens.front().at(1);
        tokens.pop_front();
        switch (option) {
        case 'd':
            debugOutput++;
            break;
        case 'h':
            humanReadableGdbmi = true;
            break;
        case 'u':
            if (tokens.empty()) {
                *errorMessage = msgLocalsUsage(args);
                return std::string();
            }
            split(tokens.front(), ',', std::back_inserter(uninitializedInames));
            tokens.pop_front();
            break;
        case 'e':
            if (tokens.empty()) {
                *errorMessage = msgLocalsUsage(args);
                return std::string();
            }
            split(tokens.front(), ',', std::back_inserter(expandedInames));
            tokens.pop_front();
            break;
        }
    }
    // Frame and iname
    unsigned frame;
    if (tokens.empty() || sscanf(tokens.front().c_str(), "%u", &frame) != 1) {
        *errorMessage = msgLocalsUsage(args);
        return std::string();
    }

    tokens.pop_front();
    if (!tokens.empty())
        iname = tokens.front();

    SymbolGroup * const symGroup = ExtensionContext::instance().symbolGroup(exc.symbols(), exc.threadId(), frame, errorMessage);
    if (!symGroup)
        return std::string();
    if (!expandedInames.empty())
        symGroup->expandList(expandedInames, errorMessage);
    if (!uninitializedInames.empty())
        symGroup->markUninitialized(uninitializedInames);
    // Complete dump
    if (iname.empty())
        return debugOutput ? symGroup->debug(debugOutput - 1) : symGroup->dump(humanReadableGdbmi);
    // Look up iname
    return symGroup->dump(iname, humanReadableGdbmi, errorMessage);
}

extern "C" HRESULT CALLBACK locals(CIDebugClient *client, PCSTR args)
{
    ExtensionCommandContext exc(client);
    std::string errorMessage;
    int token;
    const std::string output = commmandLocals(exc, args, &token, &errorMessage);
    if (output.empty()) {
        ExtensionContext::instance().report('N', token, "locals", errorMessage.c_str());
    } else {
        ExtensionContext::instance().report('R', token, "locals", "%s", output.c_str());
    }
    return S_OK;
}

// Extension command 'assign':
// Assign locals by iname: 'assign locals.x=5'

extern "C" HRESULT CALLBACK assign(CIDebugClient *client, PCSTR argsIn)
{
    ExtensionCommandContext exc(client);

    std::string errorMessage;
    bool success = false;

    int token = 0;
    do {
        const StringList tokens = commandTokens<StringList>(argsIn, &token);
        // Parse 'assign locals.x=5'
        const std::string::size_type equalsPos = tokens.size() == 1 ? tokens.front().find('=') : std::string::npos;
        if (equalsPos == std::string::npos) {
            errorMessage = "Syntax error, expecting 'locals.x=5'.";
            break;
        }
        const std::string iname = tokens.front().substr(0, equalsPos);
        const std::string value = tokens.front().substr(equalsPos + 1, tokens.front().size() - equalsPos - 1);
        // get the symbolgroup
        const int currentFrame = ExtensionContext::instance().symbolGroupFrame();
        if (currentFrame < 0) {
            errorMessage = "No current frame.";
            break;
        }
        SymbolGroup *symGroup = ExtensionContext::instance().symbolGroup(exc.symbols(), exc.threadId(), currentFrame, &errorMessage);
        if (!symGroup)
            break;
        success = symGroup->assign(iname, value, &errorMessage);
    } while (false);

    if (success) {
        ExtensionContext::instance().report('R', token, "assign", "Ok");
    } else {
        ExtensionContext::instance().report('N', token, "assign", errorMessage.c_str());
    }
    return S_OK;
}

// Extension command 'threads':
// List all thread info in GDBMI syntax

extern "C" HRESULT CALLBACK threads(CIDebugClient *client, PCSTR  argsIn)
{
    ExtensionCommandContext exc(client);
    std::string errorMessage;

    int token;
    commandTokens<StringList>(argsIn, &token);

    const std::string gdbmi = gdbmiThreadList(exc.systemObjects(),
                                              exc.symbols(),
                                              exc.control(),
                                              exc.advanced(),
                                              &errorMessage);
    if (gdbmi.empty()) {
        ExtensionContext::instance().report('N', token, "threads", errorMessage.c_str());
    } else {
        ExtensionContext::instance().report('R', token, "threads", gdbmi.c_str());
    }
    return S_OK;
}

// Extension command 'registers':
// List all registers in GDBMI syntax

extern "C" HRESULT CALLBACK registers(CIDebugClient *Client, PCSTR argsIn)
{
    ExtensionCommandContext exc(Client);
    std::string errorMessage;

    int token;
    const StringList tokens = commandTokens<StringList>(argsIn, &token);
    const bool humanReadable = !tokens.empty() && tokens.front() == "-h";
    const std::string regs = gdbmiRegisters(exc.registers(), exc.control(), humanReadable, IncludePseudoRegisters, &errorMessage);
    if (regs.empty()) {
        ExtensionContext::instance().report('N', token, "registers", errorMessage.c_str());
    } else {
        ExtensionContext::instance().report('R', token, "registers", regs.c_str());
    }
    return S_OK;
}

// Extension command 'modules':
// List all modules in GDBMI syntax

extern "C" HRESULT CALLBACK modules(CIDebugClient *Client, PCSTR argsIn)
{
    ExtensionCommandContext exc(Client);
    std::string errorMessage;

    int token;
    const StringList tokens = commandTokens<StringList>(argsIn, &token);
    const bool humanReadable = !tokens.empty() && tokens.front() == "-h";
    const std::string modules = gdbmiModules(exc.symbols(), humanReadable, &errorMessage);
    if (modules.empty()) {
        ExtensionContext::instance().report('N', token, "modules", errorMessage.c_str());
    } else {
        ExtensionContext::instance().report('R', token, "modules", modules.c_str());
    }
    return S_OK;
}

// Report stop of debuggee to Creator. This is hooked up as .idle_cmd command
// by the Creator engine to reliably get notified about stops.
extern "C" HRESULT CALLBACK idle(CIDebugClient *, PCSTR)
{
    ExtensionContext::instance().notifyIdle();
    return S_OK;
}

// Extension command 'help':
// Display version

extern "C" HRESULT CALLBACK help(CIDebugClient *, PCSTR)
{
    dprintf("Qt Creator CDB extension built %s\n", __DATE__);
    return S_OK;
}

// Extension command 'memory':
// Display memory as base64

extern "C" HRESULT CALLBACK memory(CIDebugClient *Client, PCSTR argsIn)
{
    ExtensionCommandContext exc(Client);
    std::string errorMessage;
    std::string memory;

    int token;
    ULONG64 address = 0;
    ULONG length = 0;

    const StringVector tokens = commandTokens<StringVector>(argsIn, &token);
    if (tokens.size()  == 2
            && integerFromString(tokens.front(), &address)
            && integerFromString(tokens.at(1), &length)) {
        memory = memoryToBase64(exc.dataSpaces(), address, length, &errorMessage);
    } else {
        errorMessage = "Invalid parameters to memory command.";
    }

    if (memory.empty()) {
        ExtensionContext::instance().report('N', token, "memory", errorMessage.c_str());
    } else {
        ExtensionContext::instance().report('R', token, "memory", memory.c_str());
        if (!errorMessage.empty())
            ExtensionContext::instance().report('W', token, "memory", errorMessage.c_str());
    }
    return S_OK;
}

// Extension command 'stack'
// Report stack correctly as 'k' does not list instruction pointer
// correctly.
extern "C" HRESULT CALLBACK stack(CIDebugClient *Client, PCSTR argsIn)
{
    ExtensionCommandContext exc(Client);
    std::string errorMessage;

    int token;
    bool humanReadable = false;
    unsigned maxFrames = 1000;

    StringList tokens = commandTokens<StringList>(argsIn, &token);
    if (!tokens.empty() && tokens.front() == "-h") {
         humanReadable = true;
         tokens.pop_front();
    }
    if (!tokens.empty())
        integerFromString(tokens.front(), &maxFrames);

    const std::string stack = gdbmiStack(exc.control(), exc.symbols(),
                                         maxFrames, humanReadable, &errorMessage);

    if (stack.empty()) {
        ExtensionContext::instance().report('N', token, "stack", errorMessage.c_str());
    } else {
        ExtensionContext::instance().report('R', token, "stack", stack.c_str());
    }
    return S_OK;
}

// Extension command 'shutdownex' (shutdown is reserved):
// Unhook the output callbacks. This is normally done by the session
// inaccessible notification, however, this does not work for remote-controlled sessions.
extern "C" HRESULT CALLBACK shutdownex(CIDebugClient *, PCSTR)
{
    ExtensionContext::instance().unhookCallbacks();
    return S_OK;
}

// Hook for dumping Known Structs. Not currently used.
// Shows up in 'dv' as well as IDebugSymbolGroup::GetValueText.

extern "C"  HRESULT CALLBACK KnownStructOutput(ULONG Flag, ULONG64 Address, PSTR StructName, PSTR Buffer, PULONG /* BufferSize */)
{
    if (Flag == DEBUG_KNOWN_STRUCT_GET_NAMES) {
        memcpy(Buffer, "\0\0", 2);
        return S_OK;
    }
    // Usually 260 chars buf
    std::ostringstream str;
    str << " KnownStructOutput 0x" << std::hex << Address << ' '<< StructName;
    strcpy(Buffer, str.str().c_str());
    return S_OK;
}
