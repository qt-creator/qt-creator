// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Debugger {
namespace Internal {

inline constexpr char V8REQUEST[] = "v8request";
inline constexpr char V8MESSAGE[] = "v8message";
inline constexpr char BREAKONSIGNAL[] = "breakonsignal";
inline constexpr char CONNECT[] = "connect";
inline constexpr char INTERRUPT[] = "interrupt";
inline constexpr char V8DEBUG[] = "V8DEBUG";
inline constexpr char SEQ[] = "seq";
inline constexpr char TYPE[] = "type";
inline constexpr char COMMAND[] = "command";
inline constexpr char ARGUMENTS[] = "arguments";
inline constexpr char STEPACTION[] = "stepaction";
inline constexpr char EXPRESSION[] = "expression";
inline constexpr char FRAME[] = "frame";
inline constexpr char GLOBAL[] = "global";
inline constexpr char DISABLE_BREAK[] = "disable_break";
inline constexpr char CONTEXT[] = "context";
inline constexpr char HANDLES[] = "handles";
inline constexpr char INCLUDESOURCE[] = "includeSource";
inline constexpr char FROMFRAME[] = "fromFrame";
inline constexpr char TOFRAME[] = "toFrame";
inline constexpr char BOTTOM[] = "bottom";
inline constexpr char NUMBER[] = "number";
inline constexpr char FRAMENUMBER[] = "frameNumber";
inline constexpr char TYPES[] = "types";
inline constexpr char IDS[] = "ids";
inline constexpr char FILTER[] = "filter";
inline constexpr char FROMLINE[] = "fromLine";
inline constexpr char TOLINE[] = "toLine";
inline constexpr char TARGET[] = "target";
inline constexpr char LINE[] = "line";
inline constexpr char COLUMN[] = "column";
inline constexpr char ENABLED[] = "enabled";
inline constexpr char CONDITION[] = "condition";
inline constexpr char IGNORECOUNT[] = "ignoreCount";
inline constexpr char BREAKPOINT[] = "breakpoint";
inline constexpr char FLAGS[] = "flags";

inline constexpr char CONTINEDEBUGGING[] = "continue";
inline constexpr char EVALUATE[] = "evaluate";
inline constexpr char LOOKUP[] = "lookup";
inline constexpr char BACKTRACE[] = "backtrace";
inline constexpr char SCOPE[] = "scope";
inline constexpr char SCRIPTS[] = "scripts";
inline constexpr char SETBREAKPOINT[] = "setbreakpoint";
inline constexpr char CLEARBREAKPOINT[] = "clearbreakpoint";
inline constexpr char CHANGEBREAKPOINT[] = "changebreakpoint";
inline constexpr char SETEXCEPTIONBREAK[] = "setexceptionbreak";
inline constexpr char VERSION[] = "version";
inline constexpr char DISCONNECT[] = "disconnect";
//const char PROFILE[] = "profile";

inline constexpr char REQUEST[] = "request";
inline constexpr char IN[] = "in";
inline constexpr char NEXT[] = "next";
inline constexpr char OUT[] = "out";

inline constexpr char FUNCTION[] = "function";
inline constexpr char SCRIPTREGEXP[] = "scriptRegExp";
inline constexpr char EVENT[] = "event";

inline constexpr char ALL[] = "all";
inline constexpr char UNCAUGHT[] = "uncaught";

//const char PAUSE[] = "pause";
//const char RESUME[] = "resume";

inline constexpr char HANDLE[] = "handle";
inline constexpr char REF[] = "ref";
inline constexpr char REFS[] = "refs";
inline constexpr char BODY[] = "body";
inline constexpr char NAME[] = "name";
inline constexpr char VALUE[] = "value";
inline constexpr char SUCCESS[] = "success";
inline constexpr char MESSAGE[] = "message";

inline constexpr char OBJECT[] = "{}";
inline constexpr char ARRAY[] = "[]";

inline constexpr char INTERNAL_FUNCTION[] = "(function(method) { "\
        "return (function(object, data, qmlglobal) { "\
            "return (function() { "\
                "return method(object, data, qmlglobal, arguments.length, arguments); "\
            "});"\
        "});"\
    "})";
} //Internal
} //Debugger
