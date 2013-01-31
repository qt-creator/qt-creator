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

/*!
    \group qtcreatorcdbext
    \title Qt Creator CDB extension

    \brief  QtCreatorCDB ext is an extension loaded into CDB.exe (see cdbengine.cpp).


    It provides

    \list
    \o Notification about the state of the debugging session:
    \list
        \o idle: (hooked with .idle_cmd) debuggee stopped
        \o accessible: Debuggee stopped, cdb.exe accepts commands
        \o inaccessible: Debuggee runs, no way to post commands
        \o session active/inactive: Lost debuggee, terminating.
    \endlist
    \o Hook up with output/event callbacks and produce formatted output
    \o Provide some extension commands that produce output in a standardized (GDBMI)
       format that ends up in handleExtensionMessage().
    \endlist
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
    CmdWatches,
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
    CmdExpression,
    CmdStack,
    CmdShutdownex,
    CmdAddWatch,
    CmdWidgetAt,
    CmdBreakPoints,
    CmdTest,
    CmdSetParameter
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
 "[-t token] [-v] [T formats] [-I formats] [-f debugfilter] [-c] [-h] [-d]\n[-e expand-list] [-u uninitialized-list]\n"
 "[-W] [-w watch-iname watch-expression] <frame-number> [iname]\n"
 "-h human-readable output\n"
 "-v increase verboseness of dumping\n"
 "-d debug output\n"
 "-f debug_filter\n"
 "-c complex dumpers\n"
 "-e expand-list        Comma-separated list of inames to be expanded beforehand\n"
 "-u uninitialized-list Comma-separated list of uninitialized inames\n"
 "-I formatmap          map of 'hex-encoded-iname=typecode'\n"
 "-T formatmap          map of 'hex-encoded-type-name=typecode'\n"
 "-D                    Discard existing symbol group\n"
 "-W                    Synchronize watch items (-w)\n"
 "-w iname expression   Watch item"},
{"watches",
 "Prints watches variables of symbol group in GDBMI or debug format",
 "[-t token] [-v] [T formats] [-I formats] [-f debugfilter] [-c] [-h] [-d] <iname>\n"
 "-h human-readable output\n"
 "-v increase verboseness of dumping\n"
 "-d debug output\n"
 "-f debug_filter\n"
 "-c complex dumpers\n"
 "-I formatmap          map of 'hex-encoded-iname=typecode'\n"
 "-T formatmap          map of 'hex-encoded-type-name=typecode'"},
{"dumplocal", "Dumps local variable using simple dumpers (testing command).",
 "[-t token] <frame-number> <iname>"},
{"typecast","Performs a type cast on an unexpanded iname of symbol group.",
 "[-t token] <frame-number> <iname> <desired-type>"},
{"addsymbol","Adds a symbol to symbol group (testing command).",
 "[-t token] <frame-number> <name-expression> [optional-iname]"},
{"assign","Assigns a value to a variable in current symbol group.",
 "[-t token] [-h] <iname=value>\n"
 "-h    Data are hex-encoded, binary data\n"
 "-u    Data are hex-encoded, UTF16 data"
},
{"threads","Lists threads in GDBMI format.","[-t token]"},
{"registers","Lists registers in GDBMI format","[-t token]"},
{"modules","Lists modules in GDBMI format.","[-t token]"},
{"idle",
 "Reports stop reason in GDBMI format.\n"
 "Intended to be used with .idle_cmd to obtain proper stop notification.",""},
{"help","Prints help.",""},
{"memory","Prints memory contents in Base64 encoding.","[-t token] <address> <length>"},
{"expression","Prints expression value.","[-t token] <expression>"},
{"stack","Prints stack in GDBMI format.","[-t token] [<max-frames>|unlimited]"},
{"shutdownex","Unhooks output callbacks.\nNeeds to be called explicitly only in case of remote debugging.",""},
{"addwatch","Add watch expression","<iname> <expression>"},
{"widgetat","Return address of widget at position","<x> <y>"},
{"breakpoints","List breakpoints with modules","[-h] [-v]"},
{"test","Testing command","-T type | -w watch-expression"},
{"setparameter","Set parameter","maxStringLength=value maxStackDepth=value"}
};

typedef std::vector<std::string> StringVector;
typedef std::list<std::string> StringList;

static inline bool isOption(const std::string &opt)
{
    return opt.size() == 2 && opt.at(0) == '-' && opt != "--";
}

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
    dprintf("Qt Creator CDB extension version 2.7 (Qt 5 support) %d bit built %s.\n",
            sizeof(void *) * 8, __DATE__);
    if (const ULONG pid = currentProcessId(client))
        ExtensionContext::instance().report('R', token, 0, "pid", "%u", pid);
    else
        ExtensionContext::instance().report('N', token, 0, "pid", "0");
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

// Parameters to be shared between watch/locals dump commands
struct DumpCommandParameters
{
    DumpCommandParameters() : debugOutput(0), verbose(0) {}

    // Check option off front and remove if parsed
    enum ParseOptionResult { Ok, Error, OtherOption, EndOfOptions };
    ParseOptionResult parseOption(StringList *options);

    unsigned debugOutput;
    std::string debugFilter;
    DumpParameters dumpParameters;
    unsigned verbose;
};

DumpCommandParameters::ParseOptionResult DumpCommandParameters::parseOption(StringList *options)
{
    // Parse all options and erase valid ones from the list
    if (options->empty())
        return DumpCommandParameters::EndOfOptions;

    const std::string &opt = options->front();
    if (!isOption(opt))
        return DumpCommandParameters::EndOfOptions;
    const char option = opt.at(1);
    bool knownOption = true;
    switch (option) {
    case 'd':
        debugOutput++;
        break;
    case 'h':
        dumpParameters.dumpFlags |= DumpParameters::DumpHumanReadable;
        break;
    case 'c':
        dumpParameters.dumpFlags |= DumpParameters::DumpComplexDumpers;
        break;
    case 'f':
        if (options->size() < 2)
            return Error;
        options->pop_front();
        debugFilter = options->front();
        break;
    case 'v':
        verbose++;
        break;
    case 'T': // typeformats: 'hex'ed name = formatnumber,...'
        if (options->size() < 2)
            return Error;
        options->pop_front();
        if (options->front().empty())
            return Error;
        dumpParameters.typeFormats =
            DumpParameters::decodeFormatArgument(options->front(), true);
        break;
    case 'I': // individual formats: iname= formatnumber,...'
        if (options->size() < 2)
            return Error;
        options->pop_front();
        dumpParameters.individualFormats =
            DumpParameters::decodeFormatArgument(options->front(), false);
        break;
    default:
        knownOption = false;
        break;
    } // case option
    if (knownOption)
        options->pop_front();
    return knownOption ? Ok : OtherOption;
}

// Extension command 'locals':
// Display local variables of symbol group in GDBMI or debug output form.
// Takes an optional iname which is expanded before displaying (for updateWatchData)

static std::string commmandLocals(ExtensionCommandContext &commandExtCtx,PCSTR args, int *token, std::string *errorMessage)
{
    typedef WatchesSymbolGroup::InameExpressionMap InameExpressionMap;
    typedef InameExpressionMap::value_type InameExpressionMapEntry;

    // Parse the command
    ExtensionContext &extCtx = ExtensionContext::instance();
    DumpCommandParameters parameters;
    std::string iname;
    StringList tokens = commandTokens<StringList>(args, token);
    StringVector expandedInames;
    StringVector uninitializedInames;
    InameExpressionMap watcherInameExpressionMap;
    bool watchSynchronization = false;
    bool discardSymbolGroup = false;
    // Parse away options
    for (bool optionLeft = true;  optionLeft && !tokens.empty(); ) {
        switch (parameters.parseOption(&tokens)) {
        case DumpCommandParameters::Ok:
            break;
        case DumpCommandParameters::Error:
            *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
            return std::string();
        case DumpCommandParameters::OtherOption: {
            const char option = tokens.front().at(1);
            tokens.pop_front();
            switch (option) {
            case 'u':
                if (tokens.empty()) {
                    *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
                    return std::string();
                }
                split(tokens.front(), ',', std::back_inserter(uninitializedInames));
                tokens.pop_front();
                break;
            case 'e':
                if (tokens.empty()) {
                    *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
                    return std::string();
                }
                split(tokens.front(), ',', std::back_inserter(expandedInames));
                tokens.pop_front();
                break;
            case 'w':  { // Watcher iname exp
                if (tokens.size() < 2) {
                    *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
                    return std::string();
                }
                const std::string iname = tokens.front();
                tokens.pop_front();
                const std::string expression = tokens.front();
                tokens.pop_front();
                watcherInameExpressionMap.insert(InameExpressionMapEntry(iname, expression));
            }
            break;
            case 'W':
                watchSynchronization = true;
                break;
            case 'D':
                discardSymbolGroup = true;
                break;
            } // case option
        }
        break;
        case DumpCommandParameters::EndOfOptions:
            optionLeft = false;
            break;
        }
    }
    // Frame and iname
    unsigned frame;
    if (tokens.empty() || !integerFromString(tokens.front(), &frame)) {
        *errorMessage = singleLineUsage(commandDescriptions[CmdLocals]);
        return std::string();
    }

    tokens.pop_front();
    if (!tokens.empty())
        iname = tokens.front();

    const SymbolGroupValueContext dumpContext(commandExtCtx.dataSpaces(), commandExtCtx.symbols());
    if (discardSymbolGroup)
        extCtx.discardSymbolGroup();
    SymbolGroup * const symGroup = extCtx.symbolGroup(commandExtCtx.symbols(), commandExtCtx.threadId(), frame, errorMessage);
    if (!symGroup)
        return std::string();

    if (!uninitializedInames.empty())
        symGroup->markUninitialized(uninitializedInames);

    SymbolGroupValue::verbose = parameters.verbose;
    if (SymbolGroupValue::verbose)
        DebugPrint() << parameters.dumpParameters;

    // Synchronize watches if desired.
    WatchesSymbolGroup *watchesSymbolGroup = extCtx.watchesSymbolGroup();
    if (watchSynchronization) {
        if (watcherInameExpressionMap.empty()) { // No watches..kill group.
            watchesSymbolGroup = 0;
            extCtx.discardWatchesSymbolGroup();
            if (SymbolGroupValue::verbose)
                DebugPrint() << "Discarding watchers";
        } else {
            // Force group into existence
            watchesSymbolGroup = extCtx.watchesSymbolGroup(commandExtCtx.symbols(), errorMessage);
            if (!watchesSymbolGroup || !watchesSymbolGroup->synchronize(commandExtCtx.symbols(), watcherInameExpressionMap, errorMessage))
                return std::string();
        }
    }

    // Pre-expand.
    if (!expandedInames.empty()) {
        if (parameters.dumpParameters.dumpFlags & DumpParameters::DumpComplexDumpers) {
            symGroup->expandListRunComplexDumpers(expandedInames, dumpContext, errorMessage);
            if (watchesSymbolGroup)
                watchesSymbolGroup->expandListRunComplexDumpers(expandedInames, dumpContext, errorMessage);
        } else {
            symGroup->expandList(expandedInames, errorMessage);
            if (watchesSymbolGroup)
                watchesSymbolGroup->expandList(expandedInames, errorMessage);
        }
    }

    // Debug output: Join the 2 symbol groups if no iname is given.
    if (parameters.debugOutput) {
        std::string debugRc = symGroup->debug(iname, parameters.debugFilter, parameters.debugOutput - 1);
        if (iname.empty() && watchesSymbolGroup) {
            debugRc.push_back('\n');
            debugRc += watchesSymbolGroup->debug(iname, parameters.debugFilter, parameters.debugOutput - 1);
        }
        return debugRc;
    }

    // Dump all: Join the 2 symbol groups '[local.x][watch.0]'->'[local.x,watch.0]'
    if (iname.empty()) {
        std::string dumpRc = symGroup->dump(dumpContext, parameters.dumpParameters);
        if (!dumpRc.empty() && watchesSymbolGroup) {
            std::string watchesDump = watchesSymbolGroup->dump(dumpContext, parameters.dumpParameters);
            if (!watchesDump.empty()) {
                dumpRc.erase(dumpRc.size() - 1, 1); // Strip ']'
                watchesDump[0] = ','; // '[' -> '.'
                dumpRc += watchesDump;
            }
        }
        return dumpRc;
    }

    return symGroup->dump(iname, dumpContext, parameters.dumpParameters, errorMessage);
}

extern "C" HRESULT CALLBACK locals(CIDebugClient *client, PCSTR args)
{
    ExtensionCommandContext exc(client);
    std::string errorMessage;
    int token;
    const std::string output = commmandLocals(exc, args, &token, &errorMessage);
    SymbolGroupValue::verbose = 0;
    if (output.empty())
        ExtensionContext::instance().report('N', token, 0, "locals", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "locals", output);
    return S_OK;
}

// Extension command 'locals':
// Display local variables of symbol group in GDBMI or debug output form.
// Takes an optional iname which is expanded before displaying (for updateWatchData)

static std::string commmandWatches(ExtensionCommandContext &exc, PCSTR args, int *token, std::string *errorMessage)
{
    // Parse the command
    DumpCommandParameters parameters;
    StringList tokens = commandTokens<StringList>(args, token);
    // Parse away options
    for (bool optionLeft = true;  optionLeft && !tokens.empty(); ) {
        switch (parameters.parseOption(&tokens)) {
        case DumpCommandParameters::Ok:
            break;
        case DumpCommandParameters::Error:
        case DumpCommandParameters::OtherOption:
            *errorMessage = singleLineUsage(commandDescriptions[CmdWatches]);
            return std::string();
        case DumpCommandParameters::EndOfOptions:
            optionLeft = false;
            break;
        }
    }
    const std::string iname = tokens.empty() ? std::string() : tokens.front();
    if (iname.empty() && !parameters.debugOutput) {
        *errorMessage = singleLineUsage(commandDescriptions[CmdWatches]);
        return std::string();
    }

    const SymbolGroupValueContext dumpContext(exc.dataSpaces(), exc.symbols());
    SymbolGroup * const symGroup = ExtensionContext::instance().watchesSymbolGroup(exc.symbols(), errorMessage);
    if (!symGroup)
        return std::string();
    SymbolGroupValue::verbose = parameters.verbose;

    if (parameters.debugOutput)
        return symGroup->debug(iname, parameters.debugFilter, parameters.debugOutput - 1);
    return symGroup->dump(iname, dumpContext, parameters.dumpParameters, errorMessage);
}

extern "C" HRESULT CALLBACK watches(CIDebugClient *client, PCSTR args)
{
    ExtensionCommandContext exc(client);
    std::string errorMessage = "e";
    int token = 0;
    const std::string output = commmandWatches(exc, args, &token, &errorMessage);
    SymbolGroupValue::verbose = 0;
    if (output.empty())
        ExtensionContext::instance().report('N', token, 0, "locals", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "locals", output);
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
    if (value.empty())
        ExtensionContext::instance().report('N', token, 0, "dumplocal", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "dumplocal", value);
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
    if (symGroup != 0 && symGroup->typeCast(iname, desiredType, &errorMessage))
        ExtensionContext::instance().report('R', token, 0, "typecast", "OK");
    else
        ExtensionContext::instance().report('N', token, 0, "typecast", errorMessage.c_str());
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
    if (symGroup != 0 && symGroup->addSymbol(std::string(), name, iname, &errorMessage))
        ExtensionContext::instance().report('R', token, 0, "addsymbol", "OK");
    else
        ExtensionContext::instance().report('N', token, 0, "addsymbol", errorMessage.c_str());
    return S_OK;
}

extern "C" HRESULT CALLBACK addwatch(CIDebugClient *client, PCSTR argsIn)
{
    ExtensionCommandContext exc(client);

    std::string errorMessage;
    std::string watchExpression;
    std::string iname;

    int token = 0;
    bool success = false;
    do {
        StringList tokens = commandTokens<StringList>(argsIn, &token);
        if (tokens.size() != 2) {
            errorMessage = singleLineUsage(commandDescriptions[CmdAddWatch]);
            break;
        }
        iname = tokens.front();
        tokens.pop_front();
        watchExpression = tokens.front();

        WatchesSymbolGroup *watchesSymGroup = ExtensionContext::instance().watchesSymbolGroup(exc.symbols(), &errorMessage);
        if (!watchesSymGroup)
            break;
        success = watchesSymGroup->addWatch(exc.symbols(), iname, watchExpression, &errorMessage);
    } while (false);

    if (success)
        ExtensionContext::instance().report('R', token, 0, "addwatch", "Ok");
    else
        ExtensionContext::instance().report('N', token, 0, "addwatch", errorMessage.c_str());
    return S_OK;
}

// Extension command 'assign':
// Assign locals by iname: 'assign locals.x=5'

extern "C" HRESULT CALLBACK assign(CIDebugClient *client, PCSTR argsIn)
{
    ExtensionCommandContext exc(client);

    std::string errorMessage;
    bool success = false;
    AssignEncoding enc = AssignPlainValue;
    int token = 0;
    do {
        StringList tokens = commandTokens<StringList>(argsIn, &token);
        if (tokens.empty()) {
            errorMessage = singleLineUsage(commandDescriptions[CmdAssign]);
            break;
        }

        if (tokens.front() == "-h") {
            enc = AssignHexEncoded;
            tokens.pop_front();
        } else if (tokens.front() == "-u") {
            enc = AssignHexEncodedUtf16;
            tokens.pop_front();
        }

        if (tokens.empty()) {
            errorMessage = singleLineUsage(commandDescriptions[CmdAssign]);
            break;
        }

        // Parse 'assign locals.x=5'
        const std::string::size_type equalsPos = tokens.front().find('=');
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
        success = symGroup->assign(iname, enc, value,
                                   SymbolGroupValueContext(exc.dataSpaces(), exc.symbols()),
                                   &errorMessage);
    } while (false);

    if (success)
        ExtensionContext::instance().report('R', token, 0, "assign", "Ok");
    else
        ExtensionContext::instance().report('N', token, 0, "assign", errorMessage.c_str());
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
    if (gdbmi.empty())
        ExtensionContext::instance().report('N', token, 0, "threads", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "threads", gdbmi);
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
    if (regs.empty())
        ExtensionContext::instance().report('N', token, 0, "registers", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "registers", regs);
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
    if (modules.empty())
        ExtensionContext::instance().report('N', token, 0, "modules", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "modules", modules);
    return S_OK;
}

// Report stop of debuggee to Creator. This is hooked up as .idle_cmd command
// by the Creator engine to reliably get notified about stops.
extern "C" HRESULT CALLBACK idle(CIDebugClient *client, PCSTR)
{
    ExtensionContext::instance().notifyIdleCommand(client);
    return S_OK;
}

// Extension command 'setparameter':
// Parse a list of parameters: 'key=value'

extern "C" HRESULT CALLBACK setparameter(CIDebugClient *, PCSTR args)
{
    int token;
    StringVector tokens = commandTokens<StringVector>(args, &token);
    const size_t count = tokens.size();
    size_t success = 0;
    for (size_t i = 0; i < count; ++i) {
        const std::string &token = tokens.at(i);
        const std::string::size_type equalsPos = token.find('=');
        if (equalsPos != std::string::npos) {
            const std::string value = token.substr(equalsPos + 1, token.size() - 1 - equalsPos);
            if (!token.compare(0, equalsPos, "maxStringLength")) {
                if (integerFromString(value, &ExtensionContext::instance().parameters().maxStringLength))
                    ++success;
            } else if (!token.compare(0, equalsPos, "maxStackDepth")) {
                if (integerFromString(value, &ExtensionContext::instance().parameters().maxStackDepth))
                    ++success;
            }
        }
    }
    if (success != count)
        DebugPrint() << "Errors parsing setparameters command '" << args << '\'';
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

extern "C" HRESULT CALLBACK expression(CIDebugClient *Client, PCSTR argsIn)
{
    ExtensionCommandContext exc(Client);
    std::string errorMessage;
    LONG64 value = 0;
    int token = 0;

    do {
        const StringVector tokens = commandTokens<StringVector>(argsIn, &token);
        if (tokens.size()  != 1) {

            errorMessage = singleLineUsage(commandDescriptions[CmdExpression]);
            break;
        }
        if (!evaluateInt64Expression(exc.control(), tokens.front(), &value, &errorMessage))
            break;
    } while (false);

    if (errorMessage.empty())
        ExtensionContext::instance().reportLong('R', token, "expression", toString(value));
    else
        ExtensionContext::instance().report('N', token, 0, "expression", errorMessage.c_str());
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
    unsigned maxFrames = ExtensionContext::instance().parameters().maxStackDepth;

    StringList tokens = commandTokens<StringList>(argsIn, &token);
    if (!tokens.empty() && tokens.front() == "-h") {
         humanReadable = true;
         tokens.pop_front();
    }
    if (!tokens.empty()) {
        if (tokens.front() == "unlimited") {
            maxFrames = 1000;
        } else {
            integerFromString(tokens.front(), &maxFrames);
        }
    }

    const std::string stack = gdbmiStack(exc.control(), exc.symbols(),
                                         maxFrames, humanReadable, &errorMessage);

    if (stack.empty())
        ExtensionContext::instance().report('N', token, 0, "stack", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "stack", stack);
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

extern "C" HRESULT CALLBACK widgetat(CIDebugClient *client, PCSTR argsIn)
{
    ExtensionCommandContext exc(client);
    int token = 0;
    std::string widgetAddress;
    std::string errorMessage;

    do {
        int x = -1;
        int y = -1;

        const StringVector tokens = commandTokens<StringVector>(argsIn, &token);
        if (tokens.size() != 2) {
            errorMessage = singleLineUsage(commandDescriptions[CmdWidgetAt]);
            break;
        }
        if (!integerFromString(tokens.front(), &x) || !integerFromString(tokens.at(1), &y)) {
            errorMessage = singleLineUsage(commandDescriptions[CmdWidgetAt]);
            break;
        }
        widgetAddress = widgetAt(SymbolGroupValueContext(exc.dataSpaces(), exc.symbols()),
                                 x, y, &errorMessage);
    } while (false);

    if (widgetAddress.empty())
        ExtensionContext::instance().report('N', token, 0, "widgetat", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "widgetat", widgetAddress);
    return S_OK;
}

extern "C" HRESULT CALLBACK breakpoints(CIDebugClient *client, PCSTR argsIn)
{
    ExtensionCommandContext exc(client);
    int token;
    std::string errorMessage;
    bool humanReadable = false;
    unsigned verbose = 0;
    StringList tokens = commandTokens<StringList>(argsIn, &token);
    while (!tokens.empty() && tokens.front().size() == 2 && tokens.front().at(0) == '-') {
        switch (tokens.front().at(1)) {
        case 'h':
            humanReadable = true;
            break;
        case 'v':
            verbose++;
            break;
        }
        tokens.pop_front();
    }
    const std::string bp = gdbmiBreakpoints(exc.control(), exc.symbols(), exc.dataSpaces(),
                                            humanReadable, verbose, &errorMessage);
    if (bp.empty())
        ExtensionContext::instance().report('N', token, 0, "breakpoints", errorMessage.c_str());
    else
        ExtensionContext::instance().reportLong('R', token, "breakpoints", bp);
    return S_OK;
}

extern "C" HRESULT CALLBACK test(CIDebugClient *client, PCSTR argsIn)
{
    enum Mode { Invalid, TestType, TestFixWatchExpression };
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
        case 'w':
            mode = TestFixWatchExpression;
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
        std::ostringstream str;
        switch (mode) {
        case Invalid:
            break;
        case TestType: {
            const KnownType kt = knownType(testType, 0);
            const std::string fixed = SymbolGroupValue::stripConst(testType);
            const unsigned size = SymbolGroupValue::sizeOf(fixed.c_str());
            str << '"' << testType <<  "\" (" << fixed << ") " << kt << " [";
            formatKnownTypeFlags(str, kt);
            str << "] size=" << size;
        }
            break;
        case TestFixWatchExpression:
            str << testType << " -> " << WatchesSymbolGroup::fixWatchExpression(exc.symbols(), testType);
            break;
        }
        ExtensionContext::instance().reportLong('R', token, "test", str.str());
    }
    return S_OK;
}

// Hook for dumping Known Structs.
// Shows up in 'dv' (appended) as well as IDebugSymbolGroup::GetValueText.

extern "C"  HRESULT CALLBACK KnownStructOutput(ULONG Flag, ULONG64 Address, PSTR StructName, PSTR Buffer, PULONG BufferSize)
{
    static const char knownTypesC[] = "HWND__\0"; // implicitly adds terminating 0
    if (Flag == DEBUG_KNOWN_STRUCT_GET_NAMES) {
        memcpy(Buffer, knownTypesC, sizeof(knownTypesC));
        return S_OK;
    }
    // Usually 260 chars buf
    if (!strcmp(StructName, "HWND__")) {
        // Dump a HWND. This is usually passed around as 'HWND*' with an (opaque) pointer value.
        // CDB dereferences it and passes it on here, which is why we get it as address.
        RECT rectangle;
        enum { BufSize = 1000 };
        char buf[BufSize];
        std::ostringstream str;

        HWND hwnd;
        memset(&rectangle, 0, sizeof(RECT));
        memset(&hwnd, 0, sizeof(HWND));
        // Coerce the address into a HWND (little endian)
        memcpy(&hwnd, &Address, SymbolGroupValue::pointerSize());
        // Dump geometry and class.
        if (GetWindowRect(hwnd, &rectangle) && GetClassNameA(hwnd, buf, BufSize))
            str << ' ' << rectangle.left << '+' << rectangle.top << '+'
                << (rectangle.right - rectangle.left) << 'x' << (rectangle.bottom - rectangle.top)
                << " '" << buf << '\'';
        // Check size for string + 1;
        const std::string rc = str.str();
        const ULONG requiredBufferLength = static_cast<ULONG>(rc.size()) + 1;
        if (requiredBufferLength >= *BufferSize) {
            *BufferSize = requiredBufferLength;
            return  S_FALSE;
        }
        strcpy(Buffer, rc.c_str());
    }

    return S_OK;
}
