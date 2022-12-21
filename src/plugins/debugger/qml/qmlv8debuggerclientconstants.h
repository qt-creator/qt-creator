// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Debugger {
namespace Internal {

const char V8REQUEST[] = "v8request";
const char V8MESSAGE[] = "v8message";
const char BREAKONSIGNAL[] = "breakonsignal";
const char CONNECT[] = "connect";
const char INTERRUPT[] = "interrupt";
const char V8DEBUG[] = "V8DEBUG";
const char SEQ[] = "seq";
const char TYPE[] = "type";
const char COMMAND[] = "command";
const char ARGUMENTS[] = "arguments";
const char STEPACTION[] = "stepaction";
const char EXPRESSION[] = "expression";
const char FRAME[] = "frame";
const char GLOBAL[] = "global";
const char DISABLE_BREAK[] = "disable_break";
const char CONTEXT[] = "context";
const char HANDLES[] = "handles";
const char INCLUDESOURCE[] = "includeSource";
const char FROMFRAME[] = "fromFrame";
const char TOFRAME[] = "toFrame";
const char BOTTOM[] = "bottom";
const char NUMBER[] = "number";
const char FRAMENUMBER[] = "frameNumber";
const char TYPES[] = "types";
const char IDS[] = "ids";
const char FILTER[] = "filter";
const char FROMLINE[] = "fromLine";
const char TOLINE[] = "toLine";
const char TARGET[] = "target";
const char LINE[] = "line";
const char COLUMN[] = "column";
const char ENABLED[] = "enabled";
const char CONDITION[] = "condition";
const char IGNORECOUNT[] = "ignoreCount";
const char BREAKPOINT[] = "breakpoint";
const char FLAGS[] = "flags";

const char CONTINEDEBUGGING[] = "continue";
const char EVALUATE[] = "evaluate";
const char LOOKUP[] = "lookup";
const char BACKTRACE[] = "backtrace";
const char SCOPE[] = "scope";
const char SCRIPTS[] = "scripts";
const char SETBREAKPOINT[] = "setbreakpoint";
const char CLEARBREAKPOINT[] = "clearbreakpoint";
const char CHANGEBREAKPOINT[] = "changebreakpoint";
const char SETEXCEPTIONBREAK[] = "setexceptionbreak";
const char VERSION[] = "version";
const char DISCONNECT[] = "disconnect";
//const char PROFILE[] = "profile";

const char REQUEST[] = "request";
const char IN[] = "in";
const char NEXT[] = "next";
const char OUT[] = "out";

const char FUNCTION[] = "function";
const char SCRIPTREGEXP[] = "scriptRegExp";
const char EVENT[] = "event";

const char ALL[] = "all";
const char UNCAUGHT[] = "uncaught";

//const char PAUSE[] = "pause";
//const char RESUME[] = "resume";

const char HANDLE[] = "handle";
const char REF[] = "ref";
const char REFS[] = "refs";
const char BODY[] = "body";
const char NAME[] = "name";
const char VALUE[] = "value";
const char SUCCESS[] = "success";
const char MESSAGE[] = "message";

const char OBJECT[] = "{}";
const char ARRAY[] = "[]";

const char INTERNAL_FUNCTION[] = "(function(method) { "\
        "return (function(object, data, qmlglobal) { "\
            "return (function() { "\
                "return method(object, data, qmlglobal, arguments.length, arguments); "\
            "});"\
        "});"\
    "})";
} //Internal
} //Debugger
