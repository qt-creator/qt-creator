/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlv8debuggerclient.h"
#include "qmlv8debuggerclientconstants.h"
#include "debuggerstringutils.h"

#include "watchhandler.h"
#include "breakpoint.h"
#include "breakhandler.h"
#include "qmlengine.h"
#include "stackhandler.h"

#include <utils/qtcassert.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QTextBlock>
#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QtGui/QTextDocument>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

#define DEBUG_QML 0
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif

using namespace Core;

namespace Debugger {
namespace Internal {

typedef QPair<QByteArray, QByteArray> WatchDataPair;

struct QmlV8ObjectData {
    QByteArray type;
    QVariant value;
    QVariant properties;
};

class QmlV8DebuggerClientPrivate
{
public:
    explicit QmlV8DebuggerClientPrivate(QmlV8DebuggerClient *q) :
        q(q),
        sequence(-1),
        engine(0)
    {
        q->resetState();
        parser = m_scriptEngine.evaluate(_("JSON.parse"));
        stringifier = m_scriptEngine.evaluate(_("JSON.stringify"));
    }

    void connect();
    void disconnect();

    void interrupt();
    void continueDebugging(QmlV8DebuggerClient::StepAction stepAction, int stepCount = 1);

    void evaluate(const QString expr, bool global = false, bool disableBreak = false,
                  int frame = -1, bool addContext = false);
    void lookup(const QList<int> handles, bool includeSource = false);
    void backtrace(int fromFrame = -1, int toFrame = -1, bool bottom = false);
    void frame(int number = -1);
    void scope(int number = -1, int frameNumber = -1);
    void scopes(int frameNumber = -1);
    void scripts(int types = 4, const QList<int> ids = QList<int>(),
                 bool includeSource = false, const QVariant filter = QVariant());
    void source(int frame = -1, int fromLine = -1, int toLine = -1);

    void setBreakpoint(const QString type, const QString target, int line = -1,
                       int column = -1, bool enabled = true,
                       const QString condition = QString(), int ignoreCount = -1);
    void changeBreakpoint(int breakpoint, bool enabled = true,
                          const QString condition = QString(), int ignoreCount = -1);
    void clearBreakpoint(int breakpoint);
    void setExceptionBreak(QmlV8DebuggerClient::Exceptions type, bool enabled = false);
    void listBreakpoints();

    void v8flags(const QString flags);
    void version();
    //void profile(ProfileCommand command); //NOT SUPPORTED
    void gc();

    QmlV8ObjectData extractData(const QVariant &data);

private:
    QByteArray packMessage(const QByteArray &message);
    QScriptValue initObject();

public:
    QmlV8DebuggerClient *q;

    int sequence;
    QmlEngine *engine;
    QHash<BreakpointModelId, int> breakpoints;
    QHash<int, BreakpointModelId> breakpointsSync;
    QHash<int, QByteArray> locals;
    QHash<int, WatchDataPair> watches;

    QScriptValue parser;
    QScriptValue stringifier;

    QmlV8DebuggerClient::V8DebuggerStates state;
    int currentFrameIndex;
    bool updateCurrentStackFrameIndex;
private:
    QScriptEngine m_scriptEngine;
};

///////////////////////////////////////////////////////////////////////
//
// QmlV8DebuggerClientPrivate
//
///////////////////////////////////////////////////////////////////////

void QmlV8DebuggerClientPrivate::connect()
{
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "connect",
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(CONNECT)));

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::disconnect()
{
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "disconnect",
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(DISCONNECT)));

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::interrupt()
{
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "interrupt",
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(INTERRUPT)));

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::continueDebugging(QmlV8DebuggerClient::StepAction action,
                                                   int count)
{
    //First reset
    q->resetState();

    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "continue",
    //      "arguments" : { "stepaction" : <"in", "next" or "out">,
    //                      "stepcount"  : <number of steps (default 1)>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(CONTINEDEBUGGING)));

    if (action != QmlV8DebuggerClient::Continue) {
        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
        switch (action) {
        case QmlV8DebuggerClient::In:
            args.setProperty(_(STEPACTION), QScriptValue(_(IN)));
            break;
        case QmlV8DebuggerClient::Out:
            args.setProperty(_(STEPACTION), QScriptValue(_(OUT)));
            break;
        case QmlV8DebuggerClient::Next:
            args.setProperty(_(STEPACTION), QScriptValue(_(NEXT)));
            break;
        default:break;
        }
        if (count != 1)
            args.setProperty(_(STEPCOUNT), QScriptValue(count));
        jsonVal.setProperty(_(ARGUMENTS), args);

    }
    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::evaluate(const QString expr, bool global,
                                          bool disableBreak, int frame,
                                          bool addContext)
{
    updateCurrentStackFrameIndex = false;

    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "evaluate",
    //      "arguments" : { "expression"    : <expression to evaluate>,
    //                      "frame"         : <number>,
    //                      "global"        : <boolean>,
    //                      "disable_break" : <boolean>,
    //                      "additional_context" : [
    //                           { "name" : <name1>, "handle" : <handle1> },
    //                           { "name" : <name2>, "handle" : <handle2> },
    //                           ...
    //                      ]
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(EVALUATE)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
    args.setProperty(_(EXPRESSION), QScriptValue(expr));

    if (frame != -1)
        args.setProperty(_(FRAME), QScriptValue(frame));

    if (global)
        args.setProperty(_(GLOBAL), QScriptValue(global));

    if (disableBreak)
        args.setProperty(_(DISABLE_BREAK), QScriptValue(disableBreak));

    if (addContext) {
        QAbstractItemModel *localsModel = engine->localsModel();
        int rowCount = localsModel->rowCount();

        QScriptValue ctxtList = parser.call(QScriptValue(), QScriptValueList() << _(ARRAY  ));
        while (rowCount) {
            QModelIndex index = localsModel->index(--rowCount, 0);
            const WatchData *data = engine->watchHandler()->watchData(LocalsWatch, index);
            QScriptValue ctxt = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
            ctxt.setProperty(_(NAME), QScriptValue(data->name));
            ctxt.setProperty(_(HANDLE), QScriptValue(int(data->id)));

            ctxtList.setProperty(rowCount, ctxt);
        }

        args.setProperty(_(ADDITIONAL_CONTEXT), QScriptValue(ctxtList));
    }

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::lookup(QList<int> handles, bool includeSource)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "lookup",
    //      "arguments" : { "handles"       : <array of handles>,
    //                      "includeSource" : <boolean indicating whether
    //                                          the source will be included when
    //                                          script objects are returned>,
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(LOOKUP)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    QScriptValue array = parser.call(QScriptValue(), QScriptValueList() << _(ARRAY));
    int index = 0;
    foreach (int handle, handles) {
        array.setProperty(index++, QScriptValue(handle));
    }
    args.setProperty(_(HANDLES), array);

    if (includeSource)
        args.setProperty(_(INCLUDESOURCE), QScriptValue(includeSource));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::backtrace(int fromFrame, int toFrame, bool bottom)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "backtrace",
    //      "arguments" : { "fromFrame" : <number>
    //                      "toFrame" : <number>
    //                      "bottom" : <boolean, set to true if the bottom of the
    //                          stack is requested>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(BACKTRACE)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    if (fromFrame != -1)
        args.setProperty(_(FROMFRAME), QScriptValue(fromFrame));

    if (toFrame != -1)
        args.setProperty(_(TOFRAME), QScriptValue(toFrame));

    if (bottom)
        args.setProperty(_(BOTTOM), QScriptValue(bottom));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::frame(int number)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "frame",
    //      "arguments" : { "number" : <frame number>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(FRAME)));

    if (number != -1) {
        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
        args.setProperty(_(NUMBER), QScriptValue(number));

        jsonVal.setProperty(_(ARGUMENTS), args);
    }

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::scope(int number, int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scope",
    //      "arguments" : { "number" : <scope number>
    //                      "frameNumber" : <frame number, optional uses selected
    //                                      frame if missing>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SCOPE)));

    if (number != -1) {
        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
        args.setProperty(_(NUMBER), QScriptValue(number));

        if (frameNumber != -1)
            args.setProperty(_(FRAMENUMBER), QScriptValue(frameNumber));

        jsonVal.setProperty(_(ARGUMENTS), args);
    }

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::scopes(int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scopes",
    //      "arguments" : { "frameNumber" : <frame number, optional uses selected
    //                                      frame if missing>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SCOPES)));

    if (frameNumber != -1) {
        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
        args.setProperty(_(FRAMENUMBER), QScriptValue(frameNumber));

        jsonVal.setProperty(_(ARGUMENTS), args);
    }

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::scripts(int types, const QList<int> ids, bool includeSource,
                                         const QVariant /*filter*/)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scripts",
    //      "arguments" : { "types"         : <types of scripts to retrieve
    //                                           set bit 0 for native scripts
    //                                           set bit 1 for extension scripts
    //                                           set bit 2 for normal scripts
    //                                         (default is 4 for normal scripts)>
    //                      "ids"           : <array of id's of scripts to return. If this is not specified all scripts are requrned>
    //                      "includeSource" : <boolean indicating whether the source code should be included for the scripts returned>
    //                      "filter"        : <string or number: filter string or script id.
    //                                         If a number is specified, then only the script with the same number as its script id will be retrieved.
    //                                         If a string is specified, then only scripts whose names contain the filter string will be retrieved.>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SCRIPTS)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
    args.setProperty(_(TYPES), QScriptValue(types));

    if (ids.count()) {
        QScriptValue array = parser.call(QScriptValue(), QScriptValueList() << _(ARRAY));
        int index = 0;
        foreach (int id, ids) {
            array.setProperty(index++, QScriptValue(id));
        }
        args.setProperty(_(IDS), array);
    }

    if (includeSource)
        args.setProperty(_(INCLUDESOURCE), QScriptValue(includeSource));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::source(int frame, int fromLine, int toLine)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "source",
    //      "arguments" : { "frame"    : <frame number (default selected frame)>
    //                      "fromLine" : <from line within the source default is line 0>
    //                      "toLine"   : <to line within the source this line is not included in
    //                                    the result default is the number of lines in the script>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SOURCE)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    if (frame != -1)
        args.setProperty(_(FRAME), QScriptValue(frame));

    if (fromLine != -1)
        args.setProperty(_(FROMLINE), QScriptValue(fromLine));

    if (toLine != -1)
        args.setProperty(_(TOLINE), QScriptValue(toLine));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::setBreakpoint(const QString type, const QString target,
                                               int line, int column, bool enabled,
                                               const QString condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setbreakpoint",
    //      "arguments" : { "type"        : <"function" or "script" or "scriptId" or "scriptRegExp">
    //                      "target"      : <function expression or script identification>
    //                      "line"        : <line in script or function>
    //                      "column"      : <character position within the line>
    //                      "enabled"     : <initial enabled state. True or false, default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits to ignore, default value is 0>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(SETBREAKPOINT)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(TYPE), QScriptValue(type));
    args.setProperty(_(TARGET), QScriptValue(target));

    if (line != -1)
        args.setProperty(_(LINE), QScriptValue(line));

    if (column != -1)
        args.setProperty(_(COLUMN), QScriptValue(column));

    args.setProperty(_(ENABLED), QScriptValue(enabled));

    if (!condition.isEmpty())
        args.setProperty(_(CONDITION), QScriptValue(condition));

    if (ignoreCount != -1)
        args.setProperty(_(IGNORECOUNT), QScriptValue(ignoreCount));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::changeBreakpoint(int breakpoint, bool enabled,
                                                  const QString condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "changebreakpoint",
    //      "arguments" : { "breakpoint"  : <number of the break point to clear>
    //                      "enabled"     : <initial enabled state. True or false,
    //                                      default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits            }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(CHANGEBREAKPOINT)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(BREAKPOINT), QScriptValue(breakpoint));

    args.setProperty(_(ENABLED), QScriptValue(enabled));

    if (!condition.isEmpty())
        args.setProperty(_(CONDITION), QScriptValue(condition));

    if (ignoreCount != -1)
        args.setProperty(_(IGNORECOUNT), QScriptValue(ignoreCount));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::clearBreakpoint(int breakpoint)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "clearbreakpoint",
    //      "arguments" : { "breakpoint" : <number of the break point to clear>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(CLEARBREAKPOINT)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(BREAKPOINT), QScriptValue(breakpoint));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::setExceptionBreak(QmlV8DebuggerClient::Exceptions type,
                                                   bool enabled)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setexceptionbreak",
    //      "arguments" : { "type"    : <string: "all", or "uncaught">,
    //                      "enabled" : <optional bool: enables the break type if true>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(SETEXCEPTIONBREAK)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    if (type == QmlV8DebuggerClient::AllExceptions)
        args.setProperty(_(TYPE), QScriptValue(_(ALL)));
    //Not Supported
    //    else if (type == QmlV8DebuggerClient::UncaughtExceptions)
    //        args.setProperty(_(TYPE),QScriptValue(_(UNCAUGHT)));

    if (enabled)
        args.setProperty(_(ENABLED), QScriptValue(enabled));

    jsonVal.setProperty(_(ARGUMENTS), args);


    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::listBreakpoints()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "listbreakpoints",
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(LISTBREAKPOINTS)));

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::v8flags(const QString flags)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "v8flags",
    //      "arguments" : { "flags" : <string: a sequence of v8 flags just like
    //                                  those used on the command line>
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(V8FLAGS)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(FLAGS), QScriptValue(flags));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::version()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "version",
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND), QScriptValue(_(VERSION)));

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

//void QmlV8DebuggerClientPrivate::profile(ProfileCommand command)
//{
////    { "seq"       : <number>,
////      "type"      : "request",
////      "command"   : "profile",
////      "arguments" : { "command"  : "resume" or "pause" }
////    }
//    QScriptValue jsonVal = initObject();
//    jsonVal.setProperty(_(COMMAND), QScriptValue(_(PROFILE)));

//    QScriptValue args = m_parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

//    if (command == Resume)
//        args.setProperty(_(COMMAND), QScriptValue(_(RESUME)));
//    else
//        args.setProperty(_(COMMAND), QScriptValue(_(PAUSE)));

//    args.setProperty(_("modules"), QScriptValue(1));
//    jsonVal.setProperty(_(ARGUMENTS), args);

//    const QScriptValue jsonMessage = m_stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
//    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
//}

void QmlV8DebuggerClientPrivate::gc()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "gc",
    //      "arguments" : { "type" : <string: "all">,
    //                    }
    //    }
    QScriptValue jsonVal = initObject();
    jsonVal.setProperty(_(COMMAND),
                        QScriptValue(_(GARBAGECOLLECTOR)));

    QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

    args.setProperty(_(TYPE), QScriptValue(_(ALL)));

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    q->sendMessage(packMessage(jsonMessage.toString().toUtf8()));
}

QmlV8ObjectData QmlV8DebuggerClientPrivate::extractData(const QVariant &data)
{
    //    { "handle" : <handle>,
    //      "type"   : <"undefined", "null", "boolean", "number", "string", "object", "function" or "frame">
    //    }

    //    {"handle":<handle>,"type":"undefined"}

    //    {"handle":<handle>,"type":"null"}

    //    { "handle":<handle>,
    //      "type"  : <"boolean", "number" or "string">
    //      "value" : <JSON encoded value>
    //    }

    //    {"handle":7,"type":"boolean","value":true}

    //    {"handle":8,"type":"number","value":42}

    //    { "handle"              : <handle>,
    //      "type"                : "object",
    //      "className"           : <Class name, ECMA-262 property [[Class]]>,
    //      "constructorFunction" : {"ref":<handle>},
    //      "protoObject"         : {"ref":<handle>},
    //      "prototypeObject"     : {"ref":<handle>},
    //      "properties" : [ {"name" : <name>,
    //                        "ref"  : <handle>
    //                       },
    //                       ...
    //                     ]
    //    }

    //        { "handle" : <handle>,
    //          "type"                : "function",
    //          "className"           : "Function",
    //          "constructorFunction" : {"ref":<handle>},
    //          "protoObject"         : {"ref":<handle>},
    //          "prototypeObject"     : {"ref":<handle>},
    //          "name"                : <function name>,
    //          "inferredName"        : <inferred function name for anonymous functions>
    //          "source"              : <function source>,
    //          "script"              : <reference to function script>,
    //          "scriptId"            : <id of function script>,
    //          "position"            : <function begin position in script>,
    //          "line"                : <function begin source line in script>,
    //          "column"              : <function begin source column in script>,
    //          "properties" : [ {"name" : <name>,
    //                            "ref"  : <handle>
    //                           },
    //                           ...
    //                         ]
    //        }

    QmlV8ObjectData objectData;
    const QVariantMap dataMap = data.toMap();
    QString type = dataMap.value(_(TYPE)).toString();

    if (type == _("undefined")) {
        objectData.type = QByteArray("undefined");
        objectData.value = QVariant(_("undefined"));

    } else if (type == _("null")) {
        objectData.type = QByteArray("null");
        objectData.value= QVariant(_("null"));

    } else if (type == _("boolean")) {
        objectData.type = QByteArray("boolean");
        objectData.value = dataMap.value(_(VALUE));

    } else if (type == _("number")) {
        objectData.type = QByteArray("number");
        objectData.value = dataMap.value(_(VALUE));

    } else if (type == _("string")) {
        objectData.type = QByteArray("string");
        objectData.value = dataMap.value(_(VALUE));

    } else if (type == _("object")) {
        objectData.type = QByteArray("object");
        objectData.value = dataMap.value(_("className"));
        objectData.properties = dataMap.value(_("properties"));

    } else if (type == _("function")) {
        objectData.type = QByteArray("function");
        objectData.value = dataMap.value(_(NAME));
        objectData.properties = dataMap.value(_("properties"));

    } else if (type == _("script")) {
        objectData.type = QByteArray("script");
        objectData.value = dataMap.value(_(NAME));
    }

    return objectData;
}

QByteArray QmlV8DebuggerClientPrivate::packMessage(const QByteArray &message)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = V8DEBUG;
    rs << cmd << message;
    SDEBUG(QString(message));
    return reply;
}

QScriptValue QmlV8DebuggerClientPrivate::initObject()
{
    QScriptValue jsonVal = parser.call(QScriptValue(),
                                       QScriptValueList() << QScriptValue(_(OBJECT)));
    jsonVal.setProperty(_(SEQ), QScriptValue(++sequence));
    jsonVal.setProperty(_(TYPE), _(REQUEST));
    return jsonVal;
}

///////////////////////////////////////////////////////////////////////
//
// QmlV8DebuggerClient
//
///////////////////////////////////////////////////////////////////////

QmlV8DebuggerClient::QmlV8DebuggerClient(QmlJsDebugClient::QDeclarativeDebugConnection *client)
    : QmlDebuggerClient(client, QLatin1String("V8Debugger")),
      d(new QmlV8DebuggerClientPrivate(this))
{
}

QmlV8DebuggerClient::~QmlV8DebuggerClient()
{
    delete d;
}

void QmlV8DebuggerClient::startSession()
{
    d->connect();
}

void QmlV8DebuggerClient::endSession()
{
    resetState();
    d->disconnect();
}

void QmlV8DebuggerClient::executeStep()
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState);
    d->continueDebugging(In);
}

void QmlV8DebuggerClient::executeStepOut()
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState);
    d->continueDebugging(Out);
}

void QmlV8DebuggerClient::executeNext()
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState);
    d->continueDebugging(Next);
}

void QmlV8DebuggerClient::executeStepI()
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState);
    d->continueDebugging(In);
}

void QmlV8DebuggerClient::continueInferior()
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState);
    d->continueDebugging(Continue);
}

void QmlV8DebuggerClient::interruptInferior()
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState
              || d->state == QmlV8DebuggerClient::RunningState);
    d->interrupt();
}

void QmlV8DebuggerClient::activateFrame(int index)
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState);
    d->backtrace(index);
}

bool QmlV8DebuggerClient::acceptsBreakpoint(const BreakpointModelId &id)
{
    BreakpointType type = d->engine->breakHandler()->breakpointData(id).type;
    return (type == BreakpointOnQmlSignalHandler
            || type == BreakpointByFunction
            || type == BreakpointAtJavaScriptThrow);
}

void QmlV8DebuggerClient::insertBreakpoint(const BreakpointModelId &id)
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState
              || d->state == QmlV8DebuggerClient::RunningState);
    BreakHandler *handler = d->engine->breakHandler();
    const BreakpointParameters &params = handler->breakpointData(id);

    if (params.type == BreakpointAtJavaScriptThrow) {
        handler->notifyBreakpointInsertOk(id);
        d->setExceptionBreak(AllExceptions, params.enabled);

    } else if (params.type == BreakpointByFileAndLine) {
        d->setBreakpoint(QString(_(SCRIPT)), QFileInfo(params.fileName).fileName(),
                         params.lineNumber - 1, -1, params.enabled,
                         QString(params.condition), params.ignoreCount);

    } else if (params.type == BreakpointByFunction) {
        d->setBreakpoint(QString(_(FUNCTION)), params.functionName,
                         -1, -1, params.enabled, QString(params.condition),
                         params.ignoreCount);

    } else if (params.type == BreakpointOnQmlSignalHandler) {
        d->setBreakpoint(QString(_(EVENT)), params.functionName,
                         -1, -1, params.enabled);
    }

    d->breakpointsSync.insert(d->sequence, id);
}

void QmlV8DebuggerClient::removeBreakpoint(const BreakpointModelId &id)
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState
              || d->state == QmlV8DebuggerClient::RunningState);
    BreakHandler *handler = d->engine->breakHandler();

    int breakpoint = d->breakpoints.value(id);
    d->breakpoints.remove(id);

    if (handler->breakpointData(id).type == BreakpointAtJavaScriptThrow) {
        d->setExceptionBreak(AllExceptions);
    } else {
        d->clearBreakpoint(breakpoint);
    }
}

void QmlV8DebuggerClient::changeBreakpoint(const BreakpointModelId &id)
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState
              || d->state == QmlV8DebuggerClient::RunningState);
    BreakHandler *handler = d->engine->breakHandler();
    const BreakpointParameters &params = handler->breakpointData(id);

    if (params.type == BreakpointAtJavaScriptThrow) {
        d->setExceptionBreak(AllExceptions, params.enabled);
    }
}

void QmlV8DebuggerClient::synchronizeBreakpoints()
{
    //NOT USED
}

void QmlV8DebuggerClient::assignValueInDebugger(const QByteArray /*expr*/, const quint64 &/*id*/,
                                                const QString &/*property*/, const QString &/*value*/)
{
    //TODO::
}

void QmlV8DebuggerClient::updateWatchData(const WatchData &data)
{
    if (data.isWatcher()) {
        WatchDataPair pair(data.iname, data.exp);
        if (d->watches.key(pair)) {
            WatchData data1 = data;
            data1.setAllUnneeded();
            //            data1.setValue(_("<unavailable>"));
            //            data1.setHasChildren(false);
            d->engine->watchHandler()->insertData(data1);
        } else {
            StackHandler *stackHandler = d->engine->stackHandler();
            if (stackHandler->isContentsValid())
                d->evaluate(data.exp, false, false, stackHandler->currentIndex());
            else
                d->evaluate(data.exp);
            d->watches.insert(d->sequence, pair);
        }
    }
}

void QmlV8DebuggerClient::executeDebuggerCommand(const QString &command)
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState
              || d->state == QmlV8DebuggerClient::RunningState);
    StackHandler *stackHandler = d->engine->stackHandler();
    if (stackHandler->isContentsValid()) {
        //Set the state
        d->state = QmlV8DebuggerClient::BacktraceRequestedState;
        d->evaluate(command, false, false, stackHandler->currentIndex());
    } else {
        //Currently cannot evaluate if not in a javascript break
        d->engine->showMessage(_("Request Was Unsuccessful"), ScriptConsoleOutput);
        //        d->evaluate(command);
    }
}

void QmlV8DebuggerClient::synchronizeWatchers(const QStringList &/*watchers*/)
{
    //TODO::
}

void QmlV8DebuggerClient::expandObject(const QByteArray &iname, quint64 objectId)
{
    QTC_CHECK(d->state == QmlV8DebuggerClient::WaitingForRequestState);
    d->locals.insertMulti(objectId, iname);
    d->lookup(QList<int>() << objectId);
}

void QmlV8DebuggerClient::setEngine(QmlEngine *engine)
{
    d->engine = engine;
}

void QmlV8DebuggerClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == V8DEBUG) {
        QByteArray response;
        ds >> response;
        QString responseString(response);

        SDEBUG(responseString);

        const QVariantMap resp = d->parser.call(QScriptValue(),
                                                QScriptValueList() <<
                                                QScriptValue(responseString)).toVariant().toMap();
        const QString type(resp.value(_(TYPE)).toString());

        if (type == _("response")) {

            bool success = resp.value(_("success")).toBool();
            if (!success) {
                SDEBUG("Request was unsuccessful");
                d->engine->logMessage(QmlEngine::LogReceive,
                                      QString(_("V8 Response Error: %1")).arg(
                                          resp.value(_("message")).toString()));
            }

            const QString debugCommand(resp.value(_(COMMAND)).toString());

            if (debugCommand == _(CONNECT)) {
                //debugging session started

            } else if (debugCommand == _(DISCONNECT)) {
                //debugging session ended

            } else if (debugCommand == _(BACKTRACE)) {
                if (success) {
                    updateStack(resp.value(_(BODY)), resp.value(_(REFS)));
                }

            } else if (debugCommand == _(LOOKUP)) {
                expandLocal(resp.value(_(BODY)), resp.value(_(REFS)));

            } else if (debugCommand == _(EVALUATE)) {
                if (success) {
                    int seq = resp.value(_("request_seq")).toInt();
                    updateEvaluationResult(seq, resp.value(_(BODY)), resp.value(_(REFS)));
                } else {
                    d->engine->showMessage(resp.value(_("message")).toString(), ScriptConsoleOutput);
                }

            } else if (debugCommand == _(LISTBREAKPOINTS)) {
                updateBreakpoints(resp.value(_(BODY)));

            } else if (debugCommand == _(SETBREAKPOINT)) {
                //                { "seq"         : <number>,
                //                  "type"        : "response",
                //                  "request_seq" : <number>,
                //                  "command"     : "setbreakpoint",
                //                  "body"        : { "type"       : <"function" or "script">
                //                                    "breakpoint" : <break point number of the new break point>
                //                                  }
                //                  "running"     : <is the VM running after sending this response>
                //                  "success"     : true
                //                }

                int seq = resp.value(_("request_seq")).toInt();
                const QVariantMap breakpointData = resp.value(_(BODY)).toMap();
                int index = breakpointData.value(_("breakpoint")).toInt();

                BreakpointModelId id = d->breakpointsSync.take(seq);
                d->breakpoints.insert(id, index);

                d->engine->breakHandler()->notifyBreakpointInsertOk(id);


            } else if (debugCommand == _(CHANGEBREAKPOINT)) {
                // DO NOTHING

            } else if (debugCommand == _(CLEARBREAKPOINT)) {
                // DO NOTHING

            } else if (debugCommand == _(SETEXCEPTIONBREAK)) {
                //                { "seq"               : <number>,
                //                  "type"              : "response",
                //                  "request_seq" : <number>,
                //                  "command"     : "setexceptionbreak",
                //                  “body”        : { "type"    : <string: "all" or "uncaught" corresponding to the request.>,
                //                                    "enabled" : <bool: true if the break type is currently enabled as a result of the request>
                //                                  }
                //                  "running"     : true
                //                  "success"     : true
                //                }
                //TODO::

            } else if (debugCommand == _(FRAME)) {
                if (success) {
                    const QVariant body = resp.value(_(BODY));
                    const QVariant refs = resp.value(_(REFS));
                    const QVariant locals = body.toMap().value(_("locals"));
                    StackFrame frame = createStackFrame(body, refs);
                    updateLocals(locals, refs);
                    d->engine->stackHandler()->setCurrentIndex(frame.level);
                }

            } else if (debugCommand == _(SCOPE)) {
            } else if (debugCommand == _(SCOPES)) {
            } else if (debugCommand == _(SOURCE)) {
            } else if (debugCommand == _(SCRIPTS)) {
            } else if (debugCommand == _(VERSION)) {
            } else if (debugCommand == _(V8FLAGS)) {
            } else if (debugCommand == _(GARBAGECOLLECTOR)) {
            } else {
                // DO NOTHING
            }
        } else if (type == _(EVENT)) {
            const QString eventType(resp.value(_(EVENT)).toString());

            if (eventType == _("break")) {
                //DO NOTHING
            } else if (eventType == _("exception")) {
                const QVariantMap body = resp.value(_(BODY)).toMap();
                int lineNumber = body.value(_("sourceLine")).toInt() + 1;

                const QVariantMap script = body.value(_("script")).toMap();
                QUrl fileUrl(script.value(_(NAME)).toString());
                QString filePath = d->engine->toFileInProject(fileUrl);

                const QVariantMap exception = body.value(_("exception")).toMap();
                QString errorMessage = exception.value(_("text")).toString();

                highlightExceptionCode(lineNumber, filePath, errorMessage);
            }
        }

        if (resp.value(_("running")).toBool()) {
            d->state = QmlV8DebuggerClient::RunningState;
            SDEBUG(QString(_("State: %1")).arg(d->state));
        } else {
            if (d->state == QmlV8DebuggerClient::RunningState) {
                d->state = QmlV8DebuggerClient::BreakpointsRequestedState;
                SDEBUG(QString(_("State: %1")).arg(d->state));
            }
            d->engine->inferiorSpontaneousStop();
        }

        if (d->state == QmlV8DebuggerClient::BreakpointsRequestedState) {
            d->state = QmlV8DebuggerClient::BacktraceRequestedState;
            SDEBUG(QString(_("State: %1")).arg(d->state));
            d->listBreakpoints();
        } else if (d->state == QmlV8DebuggerClient::BacktraceRequestedState) {
            d->state = QmlV8DebuggerClient::WaitingForRequestState;
            SDEBUG(QString(_("State: %1")).arg(d->state));
            d->backtrace(d->currentFrameIndex);
        }
    } else {
        //DO NOTHING
    }

}

void QmlV8DebuggerClient::updateStack(const QVariant &bodyVal, const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "backtrace",
    //      "body"        : { "fromFrame" : <number>
    //                        "toFrame" : <number>
    //                        "totalFrames" : <number>
    //                        "frames" : <array of frames - see frame request for details>
    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    StackFrames stackFrames;
    const QVariantMap body = bodyVal.toMap();
    const QVariantList frames = body.value(_("frames")).toList();

    d->engine->watchHandler()->beginCycle();
    foreach (const QVariant &frame, frames) {
        stackFrames << createStackFrame(frame, refsVal);
    }
    d->engine->watchHandler()->endCycle();

    d->currentFrameIndex = body.value(_("fromFrame")).toInt();

    if (!d->currentFrameIndex ) {
        d->engine->stackHandler()->setFrames(stackFrames);
    }

    if (d->updateCurrentStackFrameIndex) {
        d->engine->stackHandler()->setCurrentIndex(d->currentFrameIndex);
        d->engine->gotoLocation(stackFrames.value(d->currentFrameIndex));
    }

    d->updateCurrentStackFrameIndex = true;

}

StackFrame QmlV8DebuggerClient::createStackFrame(const QVariant &bodyVal, const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "frame",
    //      "body"        : { "index"          : <frame number>,
    //                        "receiver"       : <frame receiver>,
    //                        "func"           : <function invoked>,
    //                        "script"         : <script for the function>,
    //                        "constructCall"  : <boolean indicating whether the function was called as constructor>,
    //                        "debuggerFrame"  : <boolean indicating whether this is an internal debugger frame>,
    //                        "arguments"      : [ { name: <name of the argument - missing of anonymous argument>,
    //                                               value: <value of the argument>
    //                                             },
    //                                             ... <the array contains all the arguments>
    //                                           ],
    //                        "locals"         : [ { name: <name of the local variable>,
    //                                               value: <value of the local variable>
    //                                             },
    //                                             ... <the array contains all the locals>
    //                                           ],
    //                        "position"       : <source position>,
    //                        "line"           : <source line>,
    //                        "column"         : <source column within the line>,
    //                        "sourceLineText" : <text for current source line>,
    //                        "scopes"         : [ <array of scopes, see scope request below for format> ],

    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = bodyVal.toMap();

    StackFrame stackFrame;
    stackFrame.level = body.value(_("index")).toInt();

    QVariantMap func = body.value(_("func")).toMap();
    if (func.contains(_(REF))) {
        func = valueFromRef(func.value(_(REF)).toInt(), refsVal).toMap();
    }
    stackFrame.function = d->extractData(QVariant(func)).value.toString();

    QVariantMap file = body.value(_("script")).toMap();
    if (file.contains(_(REF))) {
        file = valueFromRef(file.value(_(REF)).toInt(), refsVal).toMap();
    }
    stackFrame.file = d->engine->toFileInProject(d->extractData(QVariant(file)).value.toString());
    stackFrame.usable = QFileInfo(stackFrame.file).isReadable();

    QVariantMap receiver = body.value(_("receiver")).toMap();
    if (receiver.contains(_(REF))) {
        receiver = valueFromRef(receiver.value(_(REF)).toInt(), refsVal).toMap();
    }
    stackFrame.to = d->extractData(QVariant(receiver)).value.toString();

    stackFrame.line = body.value(_("line")).toInt() + 1;

    const QVariant locals = body.value(_("locals"));
    updateLocals(locals, refsVal);

    return stackFrame;
}

void QmlV8DebuggerClient::updateLocals(const QVariant &localsVal, const QVariant &refsVal)
{
    //Add Locals
    const QVariantList locals = localsVal.toList();
    QList<WatchData> localDataList;
    foreach (const QVariant &localValue, locals) {
        QVariantMap localData = localValue.toMap();
        WatchData data;
        data.exp = localData.value(_(NAME)).toByteArray();
        //Check for v8 specific local data
        if (data.exp.startsWith(".") || data.exp.isEmpty())
            continue;

        data.name = data.exp;
        data.iname = "local." + data.exp;

        localData = valueFromRef(localData.value(_(VALUE)).toMap()
                                 .value(_(REF)).toInt(), refsVal).toMap();
        data.id = localData.value(_(HANDLE)).toInt();

        QmlV8ObjectData objectData = d->extractData(QVariant(localData));
        data.type = objectData.type;
        data.value = objectData.value.toString();

        data.setHasChildren(objectData.properties.toList().count());

        localDataList << data;
    }

    d->engine->watchHandler()->insertBulkData(localDataList);

}

void QmlV8DebuggerClient::updateEvaluationResult(int sequence, const QVariant &bodyVal,
                                                 const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "evaluate",
    //      "body"        : ...
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    QVariantMap bodyMap = bodyVal.toMap();
    if (bodyMap.contains(_(REF))) {
        bodyMap = valueFromRef(bodyMap.value(_(REF)).toInt(),
                               refsVal).toMap();
    }

    QmlV8ObjectData body = d->extractData(QVariant(bodyMap));

    if (!d->watches.contains(sequence)) {
        //Console
        d->engine->showMessage(body.value.toString(), ScriptConsoleOutput);

    } else {
        WatchDataPair pair = d->watches.value(sequence);
        QByteArray iname = pair.first;
        QByteArray exp = pair.second;
        WatchData data;
        data.exp = exp;
        data.name = data.exp;
        data.iname = iname;
        data.id = bodyMap.value(_(HANDLE)).toInt();
        data.type = body.type;
        data.value = body.value.toString();

        const QVariantList properties = body.properties.toList();
        data.setHasChildren(properties.count());
        //        data.setAllUnneeded();
        //        data.setValueNeeded();
        d->engine->watchHandler()->insertData(data);

        //        foreach (const QVariant &property, properties) {
        //            QVariantMap propertyData = property.toMap();
        //            WatchData data;
        //            data.exp = propertyData.value(_(NAME)).toByteArray();

        //            //Check for v8 specific local data
        //            if (data.exp.startsWith(".") || data.exp.isEmpty())
        //                continue;

        //            data.name = data.exp;
        //            data.iname = prepend + '.' + data.exp;
        //            propertyData = valueFromRef(propertyData.value(_(REF)).toInt(),
        //                                        refsVal).toMap();
        //            data.id = propertyData.value(_(HANDLE)).toInt();

        //            QmlV8ObjectData objectData = d->extractData(QVariant(propertyData));
        //            data.type = objectData.type;
        //            data.value = objectData.value.toString();

        //            data.setHasChildren(objectData.properties.toList().count());
        //            d->engine->watchHandler()->insertData(data);
        //        }
    }
}

void QmlV8DebuggerClient::updateBreakpoints(const QVariant &bodyVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "listbreakpoints",
    //      "body"        : { "breakpoints": [ { "type"             : <string: "scriptId"  or "scriptName".>,
    //                                           "script_id"        : <int: script id.  Only defined if type is scriptId.>,
    //                                           "script_name"      : <string: script name.  Only defined if type is scriptName.>,
    //                                           "number"           : <int: breakpoint number.  Starts from 1.>,
    //                                           "line"             : <int: line number of this breakpoint.  Starts from 0.>,
    //                                           "column"           : <int: column number of this breakpoint.  Starts from 0.>,
    //                                           "groupId"          : <int: group id of this breakpoint.>,
    //                                           "hit_count"        : <int: number of times this breakpoint has been hit.  Starts from 0.>,
    //                                           "active"           : <bool: true if this breakpoint is enabled.>,
    //                                           "ignoreCount"      : <int: remaining number of times to ignore breakpoint.  Starts from 0.>,
    //                                           "actual_locations" : <actual locations of the breakpoint.>,
    //                                         }
    //                                       ],
    //                        "breakOnExceptions"         : <true if break on all exceptions is enabled>,
    //                        "breakOnUncaughtExceptions" : <true if break on uncaught exceptions is enabled>
    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = bodyVal.toMap();
    const QVariantList breakpoints = body.value(_("breakpoints")).toList();
    BreakHandler *handler = d->engine->breakHandler();

    foreach (const QVariant &breakpoint, breakpoints) {
        const QVariantMap breakpointData = breakpoint.toMap();

        int index = breakpointData.value(_("number")).toInt();
        BreakpointModelId id = d->breakpoints.key(index);
        BreakpointResponse br = handler->response(id);

        const QVariantList actualLocations = breakpointData.value(_("actual_locations")).toList();
        foreach (const QVariant &location, actualLocations) {
            const QVariantMap locationData = location.toMap();
            br.lineNumber = locationData.value(_("line")).toInt() + 1;;
            br.enabled = breakpointData.value(_("active")).toBool();
            br.hitCount = breakpointData.value(_("hit_count")).toInt();
            br.ignoreCount = breakpointData.value(_("ignoreCount")).toInt();

            handler->setResponse(id, br);
        }
    }
}

QVariant QmlV8DebuggerClient::valueFromRef(int handle, const QVariant &refsVal)
{
    QVariant variant;
    const QVariantList refs = refsVal.toList();
    foreach (const QVariant &ref, refs) {
        const QVariantMap refData = ref.toMap();
        if (refData.value(_(HANDLE)).toInt() == handle) {
            variant = refData;
            break;
        }
    }
    return variant;
}

void QmlV8DebuggerClient::expandLocal(const QVariant &bodyVal, const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "lookup",
    //      "body"        : <array of serialized objects indexed using their handle>
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    const QVariantMap body = bodyVal.toMap();

    int handle = body.keys().value(0).toInt();
    QByteArray prepend = d->locals.take(handle);
    const WatchData *parent = d->engine->watchHandler()->findItem(prepend);
    QmlV8ObjectData bodyObjectData = d->extractData(
                body.value(body.keys().value(0)));

    const QVariantList properties = bodyObjectData.properties.toList();

    QList<WatchData> children;
    foreach (const QVariant &property, properties) {
        QVariantMap propertyData = property.toMap();
        WatchData data;
        data.name = propertyData.value(_(NAME)).toString();

        //Check for v8 specific local data
        if (data.name.startsWith(".") || data.name.isEmpty())
            continue;
        if (parent->type == "object") {
            if (parent->value == _("Array"))
                data.exp = parent->exp + '[' + data.name.toUtf8() + ']';
            else if (parent->value == _("Object"))
                data.exp = parent->exp + '.' + data.name.toUtf8();
        }
        data.iname = prepend + '.' + data.name.toUtf8();
        propertyData = valueFromRef(propertyData.value(_(REF)).toInt(),
                                    refsVal).toMap();
        data.id = propertyData.value(_(HANDLE)).toInt();

        QmlV8ObjectData objectData = d->extractData(QVariant(propertyData));
        data.type = objectData.type;
        data.value = objectData.value.toString();

        data.setHasChildren(objectData.properties.toList().count());
        children << data;
    }
    d->engine->watchHandler()->insertBulkData(children);
}

void QmlV8DebuggerClient::highlightExceptionCode(int lineNumber,
                                                 const QString &filePath,
                                                 const QString &errorMessage)
{
    EditorManager *editorManager = EditorManager::instance();
    QList<IEditor *> openedEditors = editorManager->openedEditors();

    // set up the format for the errors
    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setUnderlineColor(Qt::red);

    foreach (IEditor *editor, openedEditors) {
        if (editor->file()->fileName() == filePath) {
            TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
            if (!ed)
                continue;

            QList<QTextEdit::ExtraSelection> selections;
            QTextEdit::ExtraSelection sel;
            sel.format = errorFormat;
            QTextCursor c(ed->document()->findBlockByNumber(lineNumber));
            const QString text = c.block().text();
            for (int i = 0; i < text.size(); ++i) {
                if (! text.at(i).isSpace()) {
                    c.setPosition(c.position() + i);
                    break;
                }
            }
            c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            sel.cursor = c;

            sel.format.setToolTip(errorMessage);

            selections.append(sel);
            ed->setExtraSelections(TextEditor::BaseTextEditorWidget::DebuggerExceptionSelection, selections);
        }
    }
}

void QmlV8DebuggerClient::clearExceptionSelection()
{
    EditorManager *editorManager = EditorManager::instance();
    QList<IEditor *> openedEditors = editorManager->openedEditors();
    QList<QTextEdit::ExtraSelection> selections;

    foreach (IEditor *editor, openedEditors) {
        TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
        if (!ed)
            continue;

        ed->setExtraSelections(TextEditor::BaseTextEditorWidget::DebuggerExceptionSelection, selections);
    }

}

void QmlV8DebuggerClient::resetState()
{
    clearExceptionSelection();
    d->currentFrameIndex = 0;
    d->updateCurrentStackFrameIndex = true;
    d->state = QmlV8DebuggerClient::BreakpointsRequestedState;
    SDEBUG(QString(_("State: %1")).arg(d->state));
}

} // Internal
} // Debugger
