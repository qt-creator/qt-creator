/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "outputcallback.h"
#include "eventcallback.h"
#include "symbolgroup.h"
#include "symbolgroupvalue.h"
#include "stringutils.h"
#include "gdbmihelpers.h"

#include <cstdio>
#include <sstream>
#include <list>
#include <iterator>

/* QtCreatorCDB ext is an extension loaded into CDB.exe (see cdbengine.cpp)
 * providing:
 * - Notification about the state of the debugging session:
 *   + idle: (hooked with .idle_cmd) debuggee stopped
 *   + accessible: Debuggee stopped, cdb.exe accepts commands
 *   + inaccessible: Debuggee runs, no way to post commands
 *   + session active/inactive: Lost debuggee, terminating.
 * - Hook up with output/event callbacks and produce formatted output
 * - Provide some extension commands that produce output in a standardized (GDBMI)
 *   format that ends up in handleExtensionMessage().
 */

// Data struct and helpers for formatting help
struct CommandDescription {
    const char *name;
    const char *description;
    const char *usage;
};

// Single line of usage: For reporting usage errors back as a single line
static std::string singleLineUsage(const CommandDescription &d)
{
    std::string rc = "Usage: ";
    const char *endOfLine = strchr(d.usage, '\n');
    rc += endOfLine ? std::string(d.usage, endOfLine -  d.usage) : std::string(d.usage);
    return rc;
}

// Format description of a command
std::ostream &operator<<(std::ostream &str, const CommandDescription &d)
{
    str << "Command '" << d.name << "': " << d.description << '\n';
    if (d.usage[0])
        str << "Usage: " << d.name << ' ' << d.usage << '\n';
    str << '\n';
    return str;
}

enum Command {
    CmdPid,
    CmdExpandlocals,
    CmdLocals,
    CmdDumplocal,
    CmdTypecast,
    CmdAddsymbol,
    CmdAssign,
    CmdThreads,
    CmdRegisters,
    CmdModules,
    CmdIdle,
    CmdHelp,
    CmdMemory,
    CmdStack,
    CmdShutdownex,
    CmdTest
};

static const CommandDescription commandDescriptions[] = {
{"pid",
 "Prints inferior process id and hooks up output callbacks.",
 "[-t token]"},
{"expandlocals", "Expands local variables by iname in symbol group.",
 "[-t token] [-c] <frame-number> <iname1-list>\n"
 "iname1-list: Comma-separated list of inames\n"
 "-c complex dumpers"},
{"locals",
 "Prints local variables of symbol group in GDBMI or debug format",
 "[-t token] [-v] [T formats] [-I formats] [-f debugfilter] [-c] [-h] [-d] [-e expand-list] [-u uninitialized-list]\n<frame-number> [iname]\n"
 "-h human-readable ouput\n"
 "-v increase verboseness of dumping\n"
 "-d debug output\n"
 "-f debug_filter\n"
 "-c complex dumpers\n"
 "-e expand-list        Comma-separated list of inames to be expanded beforehand\n"
 "-u uninitialized-list Comma-separated list of uninitialized inames\n"
 "-I formatmap          map of 'hex-encoded-iname=typecode'\n"
 "-T formatmap          map of 'hex-encoded-type-name=typecode'"},
{"dumplocal", "Dumps local variable using simple dumpers (testing command).",
 "[-t token] <frame-number> <iname>"},
{"typecast","Performs a type cast on an unexpanded iname of symbol group.",
 "[-t token] <frame-number> <iname> <desired-type>"},
{"addsymbol","Adds a symbol to symbol group (testing command).",
 "[-t token] <frame-number> <name-expression> [optional-iname]"},
{"assign","Assigns a value to a variable in current symbol group.",
 "[-t token] <iname=value>"},
{"threads","Lists threads in GDBMI format.","[-t token]"},
{"registers","Lists registers in GDBMI format","[-t token]"},
{"modules","Lists modules in GDBMI format.","[-t token]"},
{"idle",
 "Reports stop reason in GDBMI format.\n"
 "Intended to be used with .idle_cmd to obtain proper stop notification.",""},
{"help","Prints help.",""},
{"memory","Prints memory contents in Base64 encoding.","[-t token] <address> <length>"},
{"stack","Prints stack in GDBMI format.","[-t token] [max-frames]"},
{"shutdownex","Unhooks output callbacks.\nNeeds to be called explicitly only in case of remote debugging.",""},
{"test","Testing command","-T type"}
};

typedef std::vector<std::string> StringVector;
typedef std::list<std::string> StringList;

// Helper for commandTokens() below:
// Simple splitting of command lines allowing for '"'-quoted tokens
// 'typecast local.i "class QString *"' -> ('typecast','local.i','class QString *')
template<class Inserter>
static inline void splitCommand(PCSTR args, Inserter it)
{
    enum State { WhiteSpace, WithinToken, WithinQuoted };

    State state = WhiteSpace;
    std::string current;
    for (PCSTR p = args; *p; p++) {
        char c = *p;
        switch (state) {
        case WhiteSpace:
            switch (c) {
            case ' ':
                break;
            case '"':
                state = WithinQuoted;
                current.clear();
                break;
            default:
                state = WithinToken;
                current.clear();
                current.push_back(c);
                break;
            }
            break;
        case WithinToken:
            switch (c) {
            case ' ':
                state = WhiteSpace;
                *it = current;
                ++it;
                break;
            default:
                current.push_back(c);
                break;
            }
            break;
        case WithinQuoted:
            switch (c) {
            case '"':
                state = WhiteSpace;
                *it = current;
                ++it;
                break;
            default:
                current.push_back(c);
                break;
            }
            break;
        }
    }
    if (state == WithinToken) {
        *it = current;
        ++it;
    }
}

// Split & Parse the arguments of a command and extract the
// optional first integer token argument ('command -t <number> remaining arguments')
// Each command takes the 'token' argument and includes it in its output
// so that the calling CDB engine in Qt Creator can match the callback.

template<class StringContainer>
static inline StringContainer commandTokens(PCSTR args, int *token = 0)
{
    typedef StringContainer::iterator ContainerIterator;

    if (token)
        *token = -1; // Handled as 'display' in engine, so that user can type commands
    StringContainer tokens;
    splitCommand(args, std::back_inserter(tokens));
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
        ExtensionContext::instance().report('R', token, 0, "pid", "%u", pid);
    } else {
        ExtensionContext::instance().report('N', token, 0, "pid", "0");
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
    StringList tokens = commandTokens<StringList>(args, &token);
    StringVector inames;
    bool runComplexDumpers = false;
    do {
        if (!tokens.empty() && tokens.front() == "-c") {
            runComplexDumpers = true;
            tokens.pop_front();
        }
        if (tokens.empty() || !integerFromString(tokens.front(), &frame))  {
            errorMessage = singleLineUsage(commandDescriptions[CmdExpandlocals]);
            break;
        }
        tokens.pop_front();
        if (tokens.empty()) {
            errorMessage = singleLineUsage(commandDescriptions[CmdExpandlocals]);
            break;
        }
        split(tokens.front(), ',', std::back_inserter(inames));
    } while (false);

    if (errorMessage.empty())
        symGroup = ExtensionContext::instance().symbolGroup(exc.symbols(), exc.threadId(), frame, &errorMessage);

    if (!symGroup) {
        ExtensionContext::instance().report('N', token, 0, "expandlocals", errorMessage.c_str());
        return S_OK;
    }

    const unsigned succeeded = runComplexDumpers ?
        symGroup->expandListRunComplexDumpers(inames, SymbolGroupValueContext(exc.dataSpaces(), exc.symbols()), &errorMessage) :
        symGroup->expandList(inames, &errorMessage);

    ExtensionContext::instance().report('R', token, 0, "expandlocals", "%u/%u %s",
                                        succeeded, unsigned(inames.size()), errorMessage.c_str());
    return S_OK;
}

// Extension command 'locals':
// Display local variables of symbol group in GDBMI or debug output form.
// Takes an optional iname which is expanded before displaying (for updateWatchData)

static std::string commmandLocals(ExtensionCommandContext &exc,PCSTR args, int *token, std::string *errorMessage)
{
    // Parse the command
    unsigned debugOutput = 0;
    std::string iname;
    std::string debugFilter;
    StringList tokens = commandTokens<StringList>(args, token);
    StringVector expandedInames;
    StringVector uninitializedInames;
    DumpParameters parameters;
    SymbolGroupValue::verbose = 0;
    // Parse away options
    while (!tokens.empty() && tokens.front().size() == 2 && tokens.front().at(0) == '-') {
        const char option = tokens.front().at(1);
        tokens.pop_front();
        switch (option) {
        case 'd':
            debugOutput++;
            break;
        case 'h':
            parameters.dumpFlags |= DumpParameters::DumpHumanReadable;
            break;
        case 'c':
            parameters.dumpFlags |= DumpParameters::DumpComplexDumpers;
            break;
        case 'u':
            if (tokens.empty()) {
                *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
                return std::string();
            }
            split(tokens.front(), ',', std::back_inserter(uninitializedInames));
            tokens.pop_front();
            break;
        case 'f':
            if (tokens.empty()) {
                *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
                return std::string();
            }
            debugFilter = tokens.front();
            tokens.pop_front();
            break;
        case 'v':
            SymbolGroupValue::verbose++;
            break;
        case 'e':
            if (tokens.empty()) {
                *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
                return std::string();
            }
            split(tokens.front(), ',', std::back_inserter(expandedInames));
            tokens.pop_front();
            break;
        case 'T': // typeformats: 'hex'ed name = formatnumber,...'
            if (tokens.empty()) {
                *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
                return std::string();
            }
            parameters.typeFormats = DumpParameters::decodeFormatArgument(tokens.front());
            tokens.pop_front();
            break;
        case 'I': // individual formats: 'hex'ed name = formatnumber,...'
            if (tokens.empty()) {
                *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
                return std::string();
            }
            parameters.individualFormats = DumpParameters::decodeFormatArgument(tokens.front());
            tokens.pop_front();
            break;
        } // case option
    }  // for options

    // Frame and iname
    unsigned frame;
    if (tokens.empty() || !integerFromString(tokens.front(), &frame)) {
        *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
        return std::string();
    }

    tokens.pop_front();
    if (!tokens.empty())
        iname = tokens.front();

    const SymbolGroupValueContext dumpContext(exc.dataSpaces(), exc.symbols());
    SymbolGroup * const symGroup = ExtensionContext::instance().symbolGroup(exc.symbols(), exc.threadId(), frame, errorMessage);
    if (!symGroup)
        return std::string();

    if (!uninitializedInames.empty())
        symGroup->markUninitialized(uninitializedInames);

    if (!expandedInames.empty()) {
        if (parameters.dumpFlags & DumpParameters::DumpComplexDumpers) {
            symGroup->expandListRunComplexDumpers(expandedInames, dumpContext, errorMessage);
        } else {
            symGroup->expandList(expandedInames, errorMessage);
        }
    }

    if (debugOutput)
        return symGroup->debug(iname, debugFilter, debugOutput - 1);

    return iname.empty() ?
           symGroup->dump(dumpContext, parameters) :
           symGroup->dump(iname, dumpContext, parameters, errorMessage);
}

extern "C" HRESULT CALLBACK locals(CIDebugClient *client, PCSTR args)
{
    ExtensionCommandContext exc(client);
    std::string errorMessage;
    int token;
    const std::string output = commmandLocals(exc, args, &token, &errorMessage);
    if (output.empty()) {
        ExtensionContext::instance().report('N', token, 0, "locals", errorMessage.c_str());
    } else {
        ExtensionContext::instance().reportLong('R', token, "locals", output);
    }
    return S_OK;
}

// Extension command 'dumplocal':
// Dump a local variable using dumpers (testing command).

static std::string dumplocalHelper(ExtensionCommandContext &exc,PCSTR args, int *token, std::string *errorMessage)
{
    // Parse the command
    StringList tokens = commandTokens<StringList>(args, token);
    // Frame and iname
    unsigned frame;
    if (tokens.empty() || integerFromString(tokens.front(), &frame)) {
        *errorMessage = singleLineUsage(commandDescriptions[CmdDumplocal]);
        return std::string();
    }
    tokens.pop_front();
    if (tokens.empty()) {
        *errorMessage = singleLineUsage(commandDescriptions[CmdDumplocal]);
        return std::string();
    }
    const std::string iname = tokens.front();

    SymbolGroup * const symGroup = ExtensionContext::instance().symbolGroup(exc.symbols(), exc.threadId(), frame, errorMessage);
    if (!symGroup)
        return std::string();

    AbstractSymbolGroupNode *n = symGroup->find(iname);
    if (!n || !n->asSymbolGroupNode()) {
        *errorMessage = "No such iname " + iname;
        return std::string();
    }
    std::wstring value;
    if (!dumpSimpleType(n->asSymbolGroupNode(), SymbolGroupValueContext(exc.dataSpaces(), exc.symbols()), &value)) {
        *errorMessage = "Cannot dump " + iname;
        return std::string();
    }
    return wStringToString(value);
}

extern "C" HRESULT CALLBACK dumplocal(CIDebugClient *client, PCSTR  argsIn)
{
    ExtensionCommandContext exc(client);
    std::string errorMessage;
    int token = 0;
    const std::string value = dumplocalHelper(exc,argsIn, &token, &errorMessage);
    if (value.empty()) {
        ExtensionContext::instance().report('N', token, 0, "dumplocal", errorMessage.c_str());
    } else {
        ExtensionContext::instance().reportLong('R', token, "dumplocal", value);
    }
    return S_OK;
}

// Extension command 'typecast':
// Change the type of a symbol group entry (testing purposes)

extern "C" HRESULT CALLBACK typecast(CIDebugClient *client, PCSTR args)
{
    ExtensionCommandContext exc(client);
    unsigned frame = 0;
    SymbolGroup *symGroup = 0;
    std::string errorMessage;

    int token;
    const StringVector tokens = commandTokens<StringVector>(args, &token);
    std::string iname;
    std::string desiredType;
    if (tokens.size() == 3u && integerFromString(tokens.front(), &frame)) {
        symGroup = ExtensionContext::instance().symbolGroup(exc.symbols(), exc.threadId(), frame, &errorMessage);
        iname = tokens.at(1);
        desiredType = tokens.at(2);
    } else {
        errorMessage = singleLineUsage(commandDescriptions[CmdTypecast]);
    }
    if (symGroup != 0 && symGroup->typeCast(iname, desiredType, &errorMessage)) {
        ExtensionContext::instance().report('R', token, 0, "typecast", "OK");
    } else {
        ExtensionContext::instance().report('N', token, 0, "typecast", errorMessage.c_str());
    }
    return S_OK;
}

// Extension command 'addsymbol':
// Adds a symbol to a symbol group by name (testing purposes)

extern "C" HRESULT CALLBACK addsymbol(CIDebugClient *client, PCSTR args)
{
    ExtensionCommandContext exc(client);
    unsigned frame = 0;
    SymbolGroup *symGroup = 0;
    std::string errorMessage;

    int token;
    const StringVector tokens = commandTokens<StringVector>(args, &token);
    std::string name;
    std::string iname;
    if (tokens.size() >= 2u && integerFromString(tokens.front(), &frame)) {
        symGroup = ExtensionContext::instance().symbolGroup(exc.symbols(), exc.threadId(), frame, &errorMessage);
        name = tokens.at(1);
        if (tokens.size() >= 3)
            iname = tokens.at(2);
    } else {
        errorMessage = singleLineUsage(commandDescriptions[CmdAddsymbol]);
    }
    if (symGroup != 0 && symGroup->addSymbol(name, iname, &errorMessage)) {
        ExtensionContext::instance().report('R', token, 0, "addsymbol", "OK");
    } else {
        ExtensionContext::instance().report('N', token, 0, "addsymbol", errorMessage.c_str());
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
            errorMessage = singleLineUsage(commandDescriptions[CmdAssign]);
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
        ExtensionContext::instance().report('R', token, 0, "assign", "Ok");
    } else {
        ExtensionContext::instance().report('N', token, 0, "assign", errorMessage.c_str());
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
        ExtensionContext::instance().report('N', token, 0, "threads", errorMessage.c_str());
    } else {
        ExtensionContext::instance().reportLong('R', token, "threads", gdbmi);
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
        ExtensionContext::instance().report('N', token, 0, "registers", errorMessage.c_str());
    } else {
        ExtensionContext::instance().reportLong('R', token, "registers", regs);
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
        ExtensionContext::instance().report('N', token, 0, "modules", errorMessage.c_str());
    } else {
        ExtensionContext::instance().reportLong('R', token, "modules", modules);
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
    std::ostringstream str;
    str << "### Qt Creator CDB extension built " << __DATE__ << "\n\n";

    const size_t commandCount = sizeof(commandDescriptions)/sizeof(CommandDescription);
    std::copy(commandDescriptions, commandDescriptions + commandCount,
              std::ostream_iterator<CommandDescription>(str));
    dprintf("%s\n", str.str().c_str());
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
        errorMessage = singleLineUsage(commandDescriptions[CmdMemory]);
    }

    if (memory.empty()) {
        ExtensionContext::instance().report('N', token, 0, "memory", errorMessage.c_str());
    } else {
        ExtensionContext::instance().reportLong('R', token, "memory", memory);
        if (!errorMessage.empty())
            ExtensionContext::instance().report('W', token, 0, "memory", errorMessage.c_str());
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
        ExtensionContext::instance().report('N', token, 0, "stack", errorMessage.c_str());
    } else {
        ExtensionContext::instance().reportLong('R', token, "stack", stack);
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

extern "C" HRESULT CALLBACK test(CIDebugClient *client, PCSTR argsIn)
{
    enum Mode { Invalid, TestType };
    ExtensionCommandContext exc(client);

    std::string testType;
    Mode mode = Invalid;
    int token = 0;
    StringList tokens = commandTokens<StringList>(argsIn, &token);
    // Parse away options
    while (!tokens.empty() && tokens.front().size() == 2 && tokens.front().at(0) == '-') {
        const char option = tokens.front().at(1);
        tokens.pop_front();
        switch (option) {
        case 'T':
            mode = TestType;
            if (!tokens.empty()) {
                testType = tokens.front();
                tokens.pop_front();
            }
            break;
        } // case option
    }  // for options

    // Frame and iname
    if (mode == Invalid || testType.empty()) {
        ExtensionContext::instance().report('N', token, 0, "test", singleLineUsage(commandDescriptions[CmdTest]).c_str());
    } else {
        const KnownType kt = knownType(testType, 0);
        std::ostringstream str;
        str << testType << ' ' << kt << " [";
        formatKnownTypeFlags(str, kt);
        str << ']';
        ExtensionContext::instance().reportLong('R', token, "test", str.str());
    }
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
