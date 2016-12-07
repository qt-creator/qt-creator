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
