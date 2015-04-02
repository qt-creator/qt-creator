/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlv8debuggerclient.h"
#include "qmlv8debuggerclientconstants.h"
#include "qmlengine.h"

#include <debugger/debuggerstringutils.h>
#include <debugger/watchhandler.h>
#include <debugger/breakhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggeractions.h>

#include <utils/qtcassert.h>
#include <texteditor/texteditor.h>

#include <coreplugin/editormanager/documentmodel.h>

#include <qmljs/consolemanagerinterface.h>

#include <QTextBlock>
#include <QFileInfo>
#include <QScriptEngine>

#define DEBUG_QML 0
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s << '\n'
#else
#   define SDEBUG(s)
#endif

using namespace Core;
using QmlDebug::QmlDebugStream;

namespace Debugger {
namespace Internal {

typedef QPair<QByteArray, QByteArray> WatchDataPair;

struct QmlV8ObjectData {
    int handle;
    QByteArray name;
    QByteArray type;
    QVariant value;
    QVariantList properties;
};

class QmlV8DebuggerClientPrivate
{
public:
    explicit QmlV8DebuggerClientPrivate(QmlV8DebuggerClient *q) :
        q(q),
        sequence(-1),
        engine(0),
        previousStepAction(QmlV8DebuggerClient::Continue)
    {
        parser = m_scriptEngine.evaluate(_("JSON.parse"));
        stringifier = m_scriptEngine.evaluate(_("JSON.stringify"));
    }

    void connect();
    void disconnect();

    void interrupt();
    void continueDebugging(QmlV8DebuggerClient::StepAction stepAction);

    void evaluate(const QString expr, bool global = false, bool disableBreak = false,
                  int frame = -1, bool addContext = false);
    void lookup(const QList<int> handles, bool includeSource = false);
    void backtrace(int fromFrame = -1, int toFrame = -1, bool bottom = false);
    void frame(int number = -1);
    void scope(int number = -1, int frameNumber = -1);
    void scripts(int types = 4, const QList<int> ids = QList<int>(),
                 bool includeSource = false, const QVariant filter = QVariant());

    void setBreakpoint(const QString type, const QString target,
                       bool enabled = true,int line = 0, int column = 0,
                       const QString condition = QString(), int ignoreCount = -1);
    void clearBreakpoint(int breakpoint);
    void setExceptionBreak(QmlV8DebuggerClient::Exceptions type, bool enabled = false);

    void version();
    //void profile(ProfileCommand command); //NOT SUPPORTED
    void clearCache();

    void logSendMessage(const QString &msg) const;
    void logReceiveMessage(const QString &msg) const;

private:
    QByteArray packMessage(const QByteArray &type, const QByteArray &message = QByteArray());
    QScriptValue initObject();

public:
    QmlV8DebuggerClient *q;

    int sequence;
    QmlEngine *engine;
    QHash<BreakpointModelId, int> breakpoints;
    QHash<int, BreakpointModelId> breakpointsSync;
    QList<int> breakpointsTemp;

    QScriptValue parser;
    QScriptValue stringifier;

    QHash<int, QString> evaluatingExpression;
    QHash<int, QByteArray> localsAndWatchers;
    QList<int> updateLocalsAndWatchers;
    QList<int> debuggerCommands;

    //Cache
    QList<int> currentFrameScopes;
    QHash<int, int> stackIndexLookup;

    QmlV8DebuggerClient::StepAction previousStepAction;
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
    logSendMessage(QString(_("%1 %2")).arg(_(V8DEBUG), _(CONNECT)));
    q->sendMessage(packMessage(CONNECT));
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
    logSendMessage(QString(_("%1 %2")).arg(_(V8DEBUG), jsonMessage.toString()));
    q->sendMessage(packMessage(DISCONNECT, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::interrupt()
{
    logSendMessage(QString(_("%1 %2")).arg(_(V8DEBUG), _(INTERRUPT)));
    q->sendMessage(packMessage(INTERRUPT));
}

void QmlV8DebuggerClientPrivate::continueDebugging(QmlV8DebuggerClient::StepAction action)
{
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
        jsonVal.setProperty(_(ARGUMENTS), args);

    }
    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
    previousStepAction = action;
}

void QmlV8DebuggerClientPrivate::evaluate(const QString expr, bool global,
                                          bool disableBreak, int frame,
                                          bool addContext)
{
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
        WatchHandler *watchHandler = engine->watchHandler();
        QAbstractItemModel *watchModel = watchHandler->model();
        int rowCount = watchModel->rowCount();

        QScriptValue ctxtList = parser.call(QScriptValue(), QScriptValueList() << _(ARRAY  ));
        while (rowCount) {
            QModelIndex index = watchModel->index(--rowCount, 0);
            const WatchData *data = watchHandler->watchItem(index);
            QScriptValue ctxt = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));
            ctxt.setProperty(_(NAME), QScriptValue(data->name));
            ctxt.setProperty(_(HANDLE), QScriptValue(int(data->id)));

            ctxtList.setProperty(rowCount, ctxt);
        }

        args.setProperty(_(ADDITIONAL_CONTEXT), QScriptValue(ctxtList));
    }

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
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
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
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
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
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
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
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
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::scripts(int types, const QList<int> ids, bool includeSource,
                                         const QVariant filter)
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

    QScriptValue filterValue;
    if (filter.type() == QVariant::String)
        filterValue = QScriptValue(filter.toString());
    else if (filter.type() == QVariant::Int)
        filterValue = QScriptValue(filter.toInt());
    else
        QTC_CHECK(!filter.isValid());

    args.setProperty(_(FILTER), filterValue);

    jsonVal.setProperty(_(ARGUMENTS), args);

    const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
}

void QmlV8DebuggerClientPrivate::setBreakpoint(const QString type, const QString target,
                                               bool enabled, int line, int column,
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
    if (type == _(EVENT)) {
        QByteArray params;
        QmlDebugStream rs(&params, QIODevice::WriteOnly);
        rs <<  target.toUtf8() << enabled;
        logSendMessage(QString(_("%1 %2 %3 %4")).arg(_(V8DEBUG), _(BREAKONSIGNAL), target, enabled?_("enabled"):_("disabled")));
        q->sendMessage(packMessage(BREAKONSIGNAL, params));

    } else {
        QScriptValue jsonVal = initObject();
        jsonVal.setProperty(_(COMMAND), QScriptValue(_(SETBREAKPOINT)));

        QScriptValue args = parser.call(QScriptValue(), QScriptValueList() << QScriptValue(_(OBJECT)));

        args.setProperty(_(TYPE), QScriptValue(type));
        if (type == _(SCRIPTREGEXP))
            args.setProperty(_(TARGET),
                             QScriptValue(Utils::FileName::fromString(target).fileName()));
        else
            args.setProperty(_(TARGET), QScriptValue(target));

        if (line)
            args.setProperty(_(LINE), QScriptValue(line - 1));

        if (column)
            args.setProperty(_(COLUMN), QScriptValue(column - 1));

        args.setProperty(_(ENABLED), QScriptValue(enabled));

        if (!condition.isEmpty())
            args.setProperty(_(CONDITION), QScriptValue(condition));

        if (ignoreCount != -1)
            args.setProperty(_(IGNORECOUNT), QScriptValue(ignoreCount));

        jsonVal.setProperty(_(ARGUMENTS), args);

        const QScriptValue jsonMessage = stringifier.call(QScriptValue(), QScriptValueList() << jsonVal);
        logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
        q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
    }
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
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
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
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
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
    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
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
//    logSendMessage(QString(_("%1 %2 %3")).arg(_(V8DEBUG), _(V8REQUEST), jsonMessage.toString()));
//    q->sendMessage(packMessage(V8REQUEST, jsonMessage.toString().toUtf8()));
//}

QVariant valueFromRef(int handle, const QVariant &refsVal, bool *success)
{
    *success = false;
    QVariant variant;
    const QVariantList refs = refsVal.toList();
    foreach (const QVariant &ref, refs) {
        const QVariantMap refData = ref.toMap();
        if (refData.value(_(HANDLE)).toInt() == handle) {
            variant = refData;
            *success = true;
            break;
        }
    }
    return variant;
}

QmlV8ObjectData extractData(const QVariant &data, const QVariant &refsVal)
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

    objectData.name = dataMap.value(_(NAME)).toByteArray();

    if (dataMap.contains(_(REF))) {
        objectData.handle = dataMap.value(_(REF)).toInt();
        bool success;
        QVariant dataFromRef = valueFromRef(objectData.handle, refsVal, &success);
        if (success) {
            QmlV8ObjectData data = extractData(dataFromRef, refsVal);
            objectData.type = data.type;
            objectData.value = data.value;
            objectData.properties = data.properties;
        }
    } else {
        objectData.handle = dataMap.value(_(HANDLE)).toInt();
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
            objectData.properties = dataMap.value(_("properties")).toList();

        } else if (type == _("function")) {
            objectData.type = QByteArray("function");
            objectData.value = dataMap.value(_(NAME));
            objectData.properties = dataMap.value(_("properties")).toList();

        } else if (type == _("script")) {
            objectData.type = QByteArray("script");
            objectData.value = dataMap.value(_(NAME));
        }
    }

    return objectData;
}

void QmlV8DebuggerClientPrivate::clearCache()
{
    currentFrameScopes.clear();
    updateLocalsAndWatchers.clear();
}

QByteArray QmlV8DebuggerClientPrivate::packMessage(const QByteArray &type, const QByteArray &message)
{
    SDEBUG(message);
    QByteArray request;
    QmlDebugStream rs(&request, QIODevice::WriteOnly);
    QByteArray cmd = V8DEBUG;
    rs << cmd << type << message;
    return request;
}

QScriptValue QmlV8DebuggerClientPrivate::initObject()
{
    QScriptValue jsonVal = parser.call(QScriptValue(),
                                       QScriptValueList() << QScriptValue(_(OBJECT)));
    jsonVal.setProperty(_(SEQ), QScriptValue(++sequence));
    jsonVal.setProperty(_(TYPE), _(REQUEST));
    return jsonVal;
}

void QmlV8DebuggerClientPrivate::logSendMessage(const QString &msg) const
{
    if (engine)
        engine->logMessage(QLatin1String("V8DebuggerClient"), QmlEngine::LogSend, msg);
}

void QmlV8DebuggerClientPrivate::logReceiveMessage(const QString &msg) const
{
    if (engine)
        engine->logMessage(QLatin1String("V8DebuggerClient"), QmlEngine::LogReceive, msg);
}

///////////////////////////////////////////////////////////////////////
//
// QmlV8DebuggerClient
//
///////////////////////////////////////////////////////////////////////

QmlV8DebuggerClient::QmlV8DebuggerClient(QmlDebug::QmlDebugConnection *client)
    : BaseQmlDebuggerClient(client, QLatin1String("V8Debugger")),
      d(new QmlV8DebuggerClientPrivate(this))
{
}

QmlV8DebuggerClient::~QmlV8DebuggerClient()
{
    delete d;
}

void QmlV8DebuggerClient::startSession()
{
    flushSendBuffer();
    d->connect();
    //Query for the V8 version. This is
    //only for logging to the debuggerlog
    d->version();
}

void QmlV8DebuggerClient::endSession()
{
    d->disconnect();
}

void QmlV8DebuggerClient::resetSession()
{
    clearExceptionSelection();
}

void QmlV8DebuggerClient::executeStep()
{
    clearExceptionSelection();
    d->continueDebugging(In);
}

void QmlV8DebuggerClient::executeStepOut()
{
    clearExceptionSelection();
    d->continueDebugging(Out);
}

void QmlV8DebuggerClient::executeNext()
{
    clearExceptionSelection();
    d->continueDebugging(Next);
}

void QmlV8DebuggerClient::executeStepI()
{
    clearExceptionSelection();
    d->continueDebugging(In);
}

void QmlV8DebuggerClient::executeRunToLine(const ContextData &data)
{
    d->setBreakpoint(QString(_(SCRIPTREGEXP)), data.fileName,
                     true, data.lineNumber);
    clearExceptionSelection();
    d->continueDebugging(Continue);
}

void QmlV8DebuggerClient::continueInferior()
{
    clearExceptionSelection();
    d->continueDebugging(Continue);
}

void QmlV8DebuggerClient::interruptInferior()
{
    d->interrupt();
}

void QmlV8DebuggerClient::activateFrame(int index)
{
    if (index != d->engine->stackHandler()->currentIndex())
        d->frame(d->stackIndexLookup.value(index));
    d->engine->stackHandler()->setCurrentIndex(index);
}

bool QmlV8DebuggerClient::acceptsBreakpoint(Breakpoint bp)
{
    BreakpointType type = bp.type();
    return (type == BreakpointOnQmlSignalEmit
            || type == BreakpointByFileAndLine
            || type == BreakpointAtJavaScriptThrow);
}

void QmlV8DebuggerClient::insertBreakpoint(Breakpoint bp,
                                           int adjustedLine,
                                           int adjustedColumn)
{
    const BreakpointParameters &params = bp.parameters();

    if (params.type == BreakpointAtJavaScriptThrow) {
        bp.notifyBreakpointInsertOk();
        d->setExceptionBreak(AllExceptions, params.enabled);

    } else if (params.type == BreakpointByFileAndLine) {
        d->setBreakpoint(QString(_(SCRIPTREGEXP)), params.fileName,
                         params.enabled, adjustedLine, adjustedColumn,
                         QLatin1String(params.condition), params.ignoreCount);

    } else if (params.type == BreakpointOnQmlSignalEmit) {
        d->setBreakpoint(QString(_(EVENT)), params.functionName, params.enabled);
        bp.notifyBreakpointInsertOk();
    }

    d->breakpointsSync.insert(d->sequence, bp.id());
}

void QmlV8DebuggerClient::removeBreakpoint(Breakpoint bp)
{
    const BreakpointParameters &params = bp.parameters();

    int breakpoint = d->breakpoints.value(bp.id());
    d->breakpoints.remove(bp.id());

    if (params.type == BreakpointAtJavaScriptThrow)
        d->setExceptionBreak(AllExceptions);
    else if (params.type == BreakpointOnQmlSignalEmit)
        d->setBreakpoint(QString(_(EVENT)), params.functionName, false);
    else
        d->clearBreakpoint(breakpoint);
}

void QmlV8DebuggerClient::changeBreakpoint(Breakpoint bp)
{
    const BreakpointParameters &params = bp.parameters();

    BreakpointResponse br = bp.response();
    if (params.type == BreakpointAtJavaScriptThrow) {
        d->setExceptionBreak(AllExceptions, params.enabled);
        br.enabled = params.enabled;
        bp.setResponse(br);
    } else if (params.type == BreakpointOnQmlSignalEmit) {
        d->setBreakpoint(QString(_(EVENT)), params.functionName, params.enabled);
        br.enabled = params.enabled;
        bp.setResponse(br);
    } else {
        //V8 supports only minimalistic changes in breakpoint
        //Remove the breakpoint and add again
        bp.notifyBreakpointChangeOk();
        bp.removeBreakpoint();
        BreakHandler *handler = d->engine->breakHandler();
        handler->appendBreakpoint(params);
    }
}

void QmlV8DebuggerClient::synchronizeBreakpoints()
{
    //NOT USED
}

void QmlV8DebuggerClient::assignValueInDebugger(const WatchData * /*data*/,
                                                const QString &expr,
                                                const QVariant &valueV)
{
    StackHandler *stackHandler = d->engine->stackHandler();
    QString expression = QString(_("%1 = %2;")).arg(expr).arg(valueV.toString());
    if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
        d->evaluate(expression, false, false, stackHandler->currentIndex());
        d->updateLocalsAndWatchers.append(d->sequence);
    } else {
        d->engine->showMessage(QString(_("Cannot evaluate %1 in current stack frame")).arg(
                                   expression), ConsoleOutput);
    }
}

void QmlV8DebuggerClient::updateWatchData(const WatchData &/*data*/)
{
    //NOT USED
}

void QmlV8DebuggerClient::executeDebuggerCommand(const QString &command)
{
    StackHandler *stackHandler = d->engine->stackHandler();
    if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
        d->evaluate(command, false, false, stackHandler->currentIndex());
        d->debuggerCommands.append(d->sequence);
    } else {
        //Currently cannot evaluate if not in a javascript break
        d->engine->showMessage(QString(_("Cannot evaluate %1 in current stack frame")).arg(
                                   command), ConsoleOutput);
    }
}

void QmlV8DebuggerClient::synchronizeWatchers(const QStringList &watchers)
{
    SDEBUG(watchers);
    if (d->engine->state() != InferiorStopOk)
        return;

    foreach (const QString &exp, watchers) {
        StackHandler *stackHandler = d->engine->stackHandler();
        if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
            d->evaluate(exp, false, false, stackHandler->currentIndex());
            d->evaluatingExpression.insert(d->sequence, exp);
        }
    }
}

void QmlV8DebuggerClient::expandObject(const QByteArray &iname, quint64 objectId)
{
    if (objectId == 0) {
        //We may have got the global object
        const WatchItem *watch = d->engine->watchHandler()->findItem(iname);
        if (watch->value == QLatin1String("global")) {
            StackHandler *stackHandler = d->engine->stackHandler();
            if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
                d->evaluate(watch->name, false, false, stackHandler->currentIndex());
                d->evaluatingExpression.insert(d->sequence, QLatin1String(iname));
            }
            return;
        }
    }
    d->localsAndWatchers.insertMulti(objectId, iname);
    d->lookup(QList<int>() << objectId);
}

void QmlV8DebuggerClient::setEngine(QmlEngine *engine)
{
    d->engine = engine;
    connect(this, &QmlV8DebuggerClient::stackFrameCompleted,
            engine, &QmlEngine::stackFrameCompleted);
}

void QmlV8DebuggerClient::getSourceFiles()
{
    d->scripts(4, QList<int>(), true, QVariant());
}

void QmlV8DebuggerClient::messageReceived(const QByteArray &data)
{
    QmlDebugStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == V8DEBUG) {
        QByteArray type;
        QByteArray response;
        ds >> type >> response;

        d->logReceiveMessage(_(V8DEBUG) + QLatin1Char(' ') +  QLatin1String(type));
        if (type == CONNECT) {
            //debugging session started

        } else if (type == INTERRUPT) {
            //debug break requested

        } else if (type == BREAKONSIGNAL) {
            //break on signal handler requested

        } else if (type == V8MESSAGE) {
            const QString responseString = QLatin1String(response);
            SDEBUG(responseString);
            d->logReceiveMessage(QLatin1String(V8MESSAGE) + QLatin1Char(' ') + responseString);

            const QVariantMap resp = d->parser.call(QScriptValue(),
                                                    QScriptValueList() <<
                                                    QScriptValue(responseString)).toVariant().toMap();
            const QString type(resp.value(_(TYPE)).toString());

            if (type == _("response")) {

                bool success = resp.value(_("success")).toBool();
                if (!success) {
                    SDEBUG("Request was unsuccessful");
                }

                const QString debugCommand(resp.value(_(COMMAND)).toString());

                if (debugCommand == _(DISCONNECT)) {
                    //debugging session ended

                } else if (debugCommand == _(CONTINEDEBUGGING)) {
                    //do nothing, wait for next break

                } else if (debugCommand == _(BACKTRACE)) {
                    if (success)
                        updateStack(resp.value(_(BODY)), resp.value(_(REFS)));

                } else if (debugCommand == _(LOOKUP)) {
                    if (success)
                        expandLocalsAndWatchers(resp.value(_(BODY)), resp.value(_(REFS)));

                } else if (debugCommand == _(EVALUATE)) {
                    int seq = resp.value(_("request_seq")).toInt();
                    if (success) {
                        updateEvaluationResult(seq, success, resp.value(_(BODY)), resp.value(_(REFS)));
                    } else {
                        QVariantMap map;
                        map.insert(_(TYPE), QVariant(_("string")));
                        map.insert(_(VALUE), resp.value(_("message")));
                        updateEvaluationResult(seq, success, QVariant(map), QVariant());
                    }

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

                    if (d->breakpointsSync.contains(seq)) {
                        BreakpointModelId id = d->breakpointsSync.take(seq);
                        d->breakpoints.insert(id, index);

                        //Is actual position info present? Then breakpoint was
                        //accepted
                        const QVariantList actualLocations =
                                breakpointData.value(
                                    _("actual_locations")).toList();
                        if (actualLocations.count()) {
                            //The breakpoint requested line should be same as
                            //actual line
                            BreakHandler *handler = d->engine->breakHandler();
                            Breakpoint bp = handler->breakpointById(id);
                            if (bp.state() != BreakpointInserted) {
                                BreakpointResponse br = bp.response();
                                br.lineNumber = breakpointData.value(_("line")).toInt() + 1;
                                bp.setResponse(br);
                                bp.notifyBreakpointInsertOk();
                            }
                        }


                    } else {
                        d->breakpointsTemp.append(index);
                    }


                } else if (debugCommand == _(CLEARBREAKPOINT)) {
                    // DO NOTHING

                } else if (debugCommand == _(SETEXCEPTIONBREAK)) {
                    //                { "seq"               : <number>,
                    //                  "type"              : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "setexceptionbreak",
                    //                  "body"        : { "type"    : <string: "all" or "uncaught" corresponding to the request.>,
                    //                                    "enabled" : <bool: true if the break type is currently enabled as a result of the request>
                    //                                  }
                    //                  "running"     : true
                    //                  "success"     : true
                    //                }


                } else if (debugCommand == _(FRAME)) {
                    if (success)
                        setCurrentFrameDetails(resp.value(_(BODY)), resp.value(_(REFS)));

                } else if (debugCommand == _(SCOPE)) {
                    if (success)
                        updateScope(resp.value(_(BODY)), resp.value(_(REFS)));

                } else if (debugCommand == _(SCRIPTS)) {
                    //                { "seq"         : <number>,
                    //                  "type"        : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "scripts",
                    //                  "body"        : [ { "name"             : <name of the script>,
                    //                                      "id"               : <id of the script>
                    //                                      "lineOffset"       : <line offset within the containing resource>
                    //                                      "columnOffset"     : <column offset within the containing resource>
                    //                                      "lineCount"        : <number of lines in the script>
                    //                                      "data"             : <optional data object added through the API>
                    //                                      "source"           : <source of the script if includeSource was specified in the request>
                    //                                      "sourceStart"      : <first 80 characters of the script if includeSource was not specified in the request>
                    //                                      "sourceLength"     : <total length of the script in characters>
                    //                                      "scriptType"       : <script type (see request for values)>
                    //                                      "compilationType"  : < How was this script compiled:
                    //                                                               0 if script was compiled through the API
                    //                                                               1 if script was compiled through eval
                    //                                                            >
                    //                                      "evalFromScript"   : <if "compilationType" is 1 this is the script from where eval was called>
                    //                                      "evalFromLocation" : { line   : < if "compilationType" is 1 this is the line in the script from where eval was called>
                    //                                                             column : < if "compilationType" is 1 this is the column in the script from where eval was called>
                    //                                  ]
                    //                  "running"     : <is the VM running after sending this response>
                    //                  "success"     : true
                    //                }

                    if (success) {
                        const QVariantList body = resp.value(_(BODY)).toList();

                        QStringList sourceFiles;
                        for (int i = 0; i < body.size(); ++i) {
                            const QVariantMap entryMap = body.at(i).toMap();
                            const int lineOffset = entryMap.value(QLatin1String("lineOffset")).toInt();
                            const int columnOffset = entryMap.value(QLatin1String("columnOffset")).toInt();
                            const QString name = entryMap.value(QLatin1String("name")).toString();
                            const QString source = entryMap.value(QLatin1String("source")).toString();

                            if (name.isEmpty())
                                continue;

                            if (!sourceFiles.contains(name))
                                sourceFiles << name;

                            d->engine->updateScriptSource(name, lineOffset, columnOffset, source);
                        }
                        d->engine->setSourceFiles(sourceFiles);
                    }
                } else if (debugCommand == _(VERSION)) {
                    d->logReceiveMessage(QString(_("Using V8 Version: %1")).arg(
                                             resp.value(_(BODY)).toMap().
                                             value(_("V8Version")).toString()));

                } else {
                    // DO NOTHING
                }

            } else if (type == _(EVENT)) {
                const QString eventType(resp.value(_(EVENT)).toString());

                if (eventType == _("break")) {
                    const QVariantMap breakData = resp.value(_(BODY)).toMap();
                    const QString invocationText = breakData.value(_("invocationText")).toString();
                    const QString scriptUrl = breakData.value(_("script")).toMap().value(_("name")).toString();
                    const QString sourceLineText = breakData.value(_("sourceLineText")).toString();

                    bool inferiorStop = true;

                    QList<int> v8BreakpointIds;
                    {
                        const QVariantList v8BreakpointIdList = breakData.value(_("breakpoints")).toList();
                        foreach (const QVariant &breakpointId, v8BreakpointIdList)
                            v8BreakpointIds << breakpointId.toInt();
                    }

                    if (!v8BreakpointIds.isEmpty() && invocationText.startsWith(_("[anonymous]()"))
                            && scriptUrl.endsWith(_(".qml"))
                            && sourceLineText.trimmed().startsWith(QLatin1Char('('))) {

                        // we hit most likely the anonymous wrapper function automatically generated for bindings
                        // -> relocate the breakpoint to column: 1 and continue

                        int newColumn = sourceLineText.indexOf(QLatin1Char('(')) + 1;
                        BreakHandler *handler = d->engine->breakHandler();

                        foreach (int v8Id, v8BreakpointIds) {
                            const BreakpointModelId id = d->breakpoints.key(v8Id);
                            Breakpoint bp = handler->breakpointById(id);
                            if (bp.isValid()) {
                                const BreakpointParameters &params = bp.parameters();

                                d->clearBreakpoint(v8Id);
                                d->setBreakpoint(QString(_(SCRIPTREGEXP)),
                                                 params.fileName,
                                                 params.enabled,
                                                 params.lineNumber,
                                                 newColumn,
                                                 QString(QString::fromLatin1(params.condition)),
                                                 params.ignoreCount);
                                d->breakpointsSync.insert(d->sequence, id);
                            }
                        }
                        d->continueDebugging(Continue);
                        inferiorStop = false;
                    }

                    //Skip debug break if this is an internal function
                    if (sourceLineText == _(INTERNAL_FUNCTION)) {
                        d->continueDebugging(d->previousStepAction);
                        inferiorStop = false;
                    }

                    if (inferiorStop) {
                        //Update breakpoint data
                        BreakHandler *handler = d->engine->breakHandler();
                        foreach (int v8Id, v8BreakpointIds) {
                            const BreakpointModelId id = d->breakpoints.key(v8Id);
                            Breakpoint bp = handler->breakpointById(id);
                            if (bp) {
                                BreakpointResponse br = bp.response();
                                if (br.functionName.isEmpty()) {
                                    br.functionName = invocationText;
                                    bp.setResponse(br);
                                }
                                if (bp.state() != BreakpointInserted) {
                                    br.lineNumber = breakData.value(
                                                _("sourceLine")).toInt() + 1;
                                    bp.setResponse(br);
                                    bp.notifyBreakpointInsertOk();
                                }
                            }
                        }

                        if (d->engine->state() == InferiorRunOk) {
                            foreach (const QVariant &breakpointId, v8BreakpointIds) {
                                if (d->breakpointsTemp.contains(breakpointId.toInt()))
                                    d->clearBreakpoint(breakpointId.toInt());
                            }
                            d->engine->inferiorSpontaneousStop();
                            d->backtrace();
                        } else if (d->engine->state() == InferiorStopOk) {
                            d->backtrace();
                        }
                    }

                } else if (eventType == _("exception")) {
                    const QVariantMap body = resp.value(_(BODY)).toMap();
                    int lineNumber = body.value(_("sourceLine")).toInt() + 1;

                    const QVariantMap script = body.value(_("script")).toMap();
                    QUrl fileUrl(script.value(_(NAME)).toString());
                    QString filePath = d->engine->toFileInProject(fileUrl);

                    const QVariantMap exception = body.value(_("exception")).toMap();
                    QString errorMessage = exception.value(_("text")).toString();

                    highlightExceptionCode(lineNumber, filePath, errorMessage);

                    if (d->engine->state() == InferiorRunOk) {
                        d->engine->inferiorSpontaneousStop();
                        d->backtrace();
                    }

                    if (d->engine->state() == InferiorStopOk)
                        d->backtrace();

                } else if (eventType == _("afterCompile")) {
                    //Currently break point relocation is disabled.
                    //Uncomment the line below when it will be enabled.
//                    d->listBreakpoints();
                }

                //Sometimes we do not get event type!
                //This is most probably due to a wrong eval expression.
                //Redirect output to console.
                if (eventType.isEmpty()) {
                     bool success = resp.value(_("success")).toBool();
                     QVariantMap map;
                     map.insert(_(TYPE), QVariant(_("string")));
                     map.insert(_(VALUE), resp.value(_("message")));
                     //Since there is no sequence value, best estimate is
                     //last sequence value
                     updateEvaluationResult(d->sequence, success, QVariant(map), QVariant());
                }

            } //EVENT
        } //V8MESSAGE

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

    const QVariantMap body = bodyVal.toMap();
    const QVariantList frames = body.value(_("frames")).toList();

    int fromFrameIndex = body.value(_("fromFrame")).toInt();

    QTC_ASSERT(0 == fromFrameIndex, return);

    StackHandler *stackHandler = d->engine->stackHandler();
    StackFrames stackFrames;
    int i = 0;
    d->stackIndexLookup.clear();
    foreach (const QVariant &frame, frames) {
        StackFrame stackFrame = extractStackFrame(frame, refsVal);
        if (stackFrame.level < 0)
            continue;
        d->stackIndexLookup.insert(i, stackFrame.level);
        stackFrame.level = i;
        stackFrames << stackFrame;
        i++;
    }
    stackHandler->setFrames(stackFrames);

    //Populate locals and watchers wrt top frame
    //Update all Locals visible in current scope
    //Traverse the scope chain and store the local properties
    //in a list and show them in the Locals Window.
    setCurrentFrameDetails(frames.value(0), refsVal);
}

StackFrame QmlV8DebuggerClient::extractStackFrame(const QVariant &bodyVal, const QVariant &refsVal)
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
    //Do not insert the frame corresponding to the internal function
    if (body.value(QLatin1String("sourceLineText")) == QLatin1String(INTERNAL_FUNCTION)) {
        stackFrame.level = -1;
        return stackFrame;
    }

    QmlV8ObjectData objectData = extractData(body.value(_("func")), refsVal);
    QString functionName = objectData.value.toString();
    if (functionName.isEmpty())
        functionName = tr("Anonymous Function");
    stackFrame.function = functionName;

    objectData = extractData(body.value(_("script")), refsVal);
    stackFrame.file = d->engine->toFileInProject(objectData.value.toString());
    stackFrame.usable = QFileInfo(stackFrame.file).isReadable();

    objectData = extractData(body.value(_("receiver")), refsVal);
    stackFrame.to = objectData.value.toString();

    stackFrame.line = body.value(_("line")).toInt() + 1;

    return stackFrame;
}

void QmlV8DebuggerClient::setCurrentFrameDetails(const QVariant &bodyVal, const QVariant &refsVal)
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
    QVariantMap currentFrame = bodyVal.toMap();

    StackHandler *stackHandler = d->engine->stackHandler();
    WatchHandler * watchHandler = d->engine->watchHandler();
    d->clearCache();

    const int frameIndex = stackHandler->currentIndex();
    QSet<QByteArray> expandedInames = watchHandler->expandedINames();
    QHash<quint64, QByteArray> handlesToLookup;
    // Store handles of all expanded watch data
    foreach (const QByteArray &iname, expandedInames) {
        const WatchItem *item = watchHandler->findItem(iname);
        if (item && item->isLocal())
            handlesToLookup.insert(item->id, iname);
    }
    watchHandler->removeAllData();
    if (frameIndex < 0)
        return;
    const StackFrame frame = stackHandler->currentFrame();
    if (!frame.isUsable())
        return;

    //Set "this" variable
    {
        auto item = new WatchItem("local.this", QLatin1String("this"));
        QmlV8ObjectData objectData = extractData(currentFrame.value(_("receiver")), refsVal);
        item->id = objectData.handle;
        item->type = objectData.type;
        item->value = objectData.value.toString();
        item->setHasChildren(objectData.properties.count());
        //Incase of global object, we do not get children
        //Set children nevertheless and query later
        if (item->value == QLatin1String("global")) {
            item->setHasChildren(true);
            item->id = 0;
        }
        watchHandler->insertItem(item);
    }

    const QVariantList currentFrameScopes = currentFrame.value(_("scopes")).toList();
    foreach (const QVariant &scope, currentFrameScopes) {
        //Do not query for global types (0)
        //Showing global properties increases clutter.
        if (scope.toMap().value(_("type")).toInt() == 0)
            continue;
        int scopeIndex = scope.toMap().value(_("index")).toInt();
        d->currentFrameScopes.append(scopeIndex);
        d->scope(scopeIndex);
    }
    d->engine->gotoLocation(stackHandler->currentFrame());

    // Expand watch data that were previously expanded
    QHash<quint64, QByteArray>::const_iterator itEnd = handlesToLookup.constEnd();
    for (QHash<quint64, QByteArray>::const_iterator it = handlesToLookup.constBegin(); it != itEnd; ++it)
        expandObject(it.value(), it.key());
    emit stackFrameCompleted();
}

void QmlV8DebuggerClient::updateScope(const QVariant &bodyVal, const QVariant &refsVal)
{
//    { "seq"         : <number>,
//      "type"        : "response",
//      "request_seq" : <number>,
//      "command"     : "scope",
//      "body"        : { "index"      : <index of this scope in the scope chain. Index 0 is the top scope
//                                        and the global scope will always have the highest index for a
//                                        frame>,
//                        "frameIndex" : <index of the frame>,
//                        "type"       : <type of the scope:
//                                         0: Global
//                                         1: Local
//                                         2: With
//                                         3: Closure
//                                         4: Catch >,
//                        "object"     : <the scope object defining the content of the scope.
//                                        For local and closure scopes this is transient objects,
//                                        which has a negative handle value>
//                      }
//      "running"     : <is the VM running after sending this response>
//      "success"     : true
//    }
    QVariantMap bodyMap = bodyVal.toMap();

    //Check if the frameIndex is same as current Stack Index
    StackHandler *stackHandler = d->engine->stackHandler();
    if (bodyMap.value(_("frameIndex")).toInt() != stackHandler->currentIndex())
        return;

    QmlV8ObjectData objectData = extractData(bodyMap.value(_("object")), refsVal);

    QList<int> handlesToLookup;
    foreach (const QVariant &property, objectData.properties) {
        QmlV8ObjectData localData = extractData(property, refsVal);
        auto item = new WatchItem;
        item->exp = localData.name;
        //Check for v8 specific local data
        if (item->exp.startsWith('.') || item->exp.isEmpty())
            continue;

        item->name = QLatin1String(item->exp);
        item->iname = QByteArray("local.") + item->exp;

        int handle = localData.handle;
        if (localData.value.isValid()) {
            item->id = handle;
            item->type = localData.type;
            item->value = localData.value.toString();
            item->setHasChildren(localData.properties.count());
            d->engine->watchHandler()->insertItem(item);
        } else {
            handlesToLookup << handle;
            d->localsAndWatchers.insertMulti(handle, item->exp);
        }
    }

    if (!handlesToLookup.isEmpty())
        d->lookup(handlesToLookup);
}

QmlJS::ConsoleItem *constructLogItemTree(QmlJS::ConsoleItem *parent,
                                                 const QmlV8ObjectData &objectData,
                                                 const QVariant &refsVal)
{
    using namespace QmlJS;
    bool sorted = boolSetting(SortStructMembers);
    if (!objectData.value.isValid())
        return 0;

    QString text;
    if (objectData.name.isEmpty())
        text = objectData.value.toString();
    else
        text = QString(_("%1: %2")).arg(QString::fromLatin1(objectData.name))
                .arg(objectData.value.toString());

    ConsoleItem *item = new ConsoleItem(parent, ConsoleItem::UndefinedType, text);

    QSet<QString> childrenFetched;
    foreach (const QVariant &property, objectData.properties) {
        const QmlV8ObjectData childObjectData = extractData(property, refsVal);
        if (childObjectData.handle == objectData.handle)
            continue;
        ConsoleItem *child = constructLogItemTree(item, childObjectData, refsVal);
        if (child) {
            const QString text = child->text();
            if (childrenFetched.contains(text))
                continue;
            childrenFetched.insert(text);
            item->insertChild(child, sorted);
        }
    }

    return item;
}

void QmlV8DebuggerClient::updateEvaluationResult(int sequence, bool success,
                                                 const QVariant &bodyVal, const QVariant &refsVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "evaluate",
    //      "body"        : ...
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    WatchHandler *watchHandler = d->engine->watchHandler();
    if (d->updateLocalsAndWatchers.contains(sequence)) {
        d->updateLocalsAndWatchers.removeOne(sequence);
        //Update the locals
        foreach (int index, d->currentFrameScopes)
            d->scope(index);
        //Also update "this"
        QByteArray iname("local.this");
        const WatchItem *parent = watchHandler->findItem(iname);
        d->localsAndWatchers.insertMulti(parent->id, iname);
        d->lookup(QList<int>() << parent->id);

    } else if (d->debuggerCommands.contains(sequence)) {
        d->updateLocalsAndWatchers.removeOne(sequence);
        QmlV8ObjectData body = extractData(bodyVal, refsVal);
        using namespace QmlJS;
        ConsoleManagerInterface *consoleManager = ConsoleManagerInterface::instance();
        if (consoleManager) {
            ConsoleItem *item = constructLogItemTree(consoleManager->rootItem(), body, refsVal);
            if (item)
                consoleManager->printToConsolePane(item);
        }
        //Update the locals
        foreach (int index, d->currentFrameScopes)
            d->scope(index);

    } else {
        QmlV8ObjectData body = extractData(bodyVal, refsVal);
        if (d->evaluatingExpression.contains(sequence)) {
            QString exp =  d->evaluatingExpression.take(sequence);
            //Do we have request to evaluate a local?
            if (exp.startsWith(QLatin1String("local."))) {
                const WatchItem *item = watchHandler->findItem(exp.toLatin1());
                createWatchDataList(item, body.properties, refsVal);
            } else {
                QByteArray iname = watchHandler->watcherName(exp.toLatin1());
                SDEBUG(QString(iname));

                auto item = new WatchItem(iname, exp);
                item->exp = exp.toLatin1();
                item->id = body.handle;
                if (success) {
                    item->type = body.type;
                    item->value = body.value.toString();
                    item->wantsChildren = body.properties.count();
                } else {
                    //Do not set type since it is unknown
                    item->setError(body.value.toString());
                }
                watchHandler->insertItem(item);
                createWatchDataList(item, body.properties, refsVal);
            }
            //Insert the newly evaluated expression to the Watchers Window
        }
    }
}

void QmlV8DebuggerClient::expandLocalsAndWatchers(const QVariant &bodyVal, const QVariant &refsVal)
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

    QStringList handlesList = body.keys();
    WatchHandler *watchHandler = d->engine->watchHandler();
    foreach (const QString &handle, handlesList) {
        QmlV8ObjectData bodyObjectData = extractData(
                    body.value(handle), refsVal);
        QByteArray prepend = d->localsAndWatchers.take(handle.toInt());

        if (prepend.startsWith("local.") || prepend.startsWith("watch.")) {
            // Data for expanded local/watch.
            // Could be an object or function.
            const WatchItem *parent = watchHandler->findItem(prepend);
            createWatchDataList(parent, bodyObjectData.properties, refsVal);
        } else {
            //rest
            auto item = new WatchItem;
            item->exp = prepend;
            item->name = QLatin1String(item->exp);
            item->iname = QByteArray("local.") + item->exp;
            item->id = handle.toInt();

            item->type = bodyObjectData.type;
            item->value = bodyObjectData.value.toString();

            item->setHasChildren(bodyObjectData.properties.count());

            d->engine->watchHandler()->insertItem(item);
        }
    }
}

void QmlV8DebuggerClient::createWatchDataList(const WatchItem *parent,
                                              const QVariantList &properties,
                                              const QVariant &refsVal)
{
    if (properties.count()) {
        QTC_ASSERT(parent, return);
        foreach (const QVariant &property, properties) {
            QmlV8ObjectData propertyData = extractData(property, refsVal);
            auto item = new WatchItem;
            item->name = QString::fromUtf8(propertyData.name);

            //Check for v8 specific local data
            if (item->name.startsWith(QLatin1Char('.')) || item->name.isEmpty())
                continue;
            if (parent->type == "object") {
                if (parent->value == _("Array"))
                    item->exp = parent->exp + '[' + item->name.toLatin1() + ']';
                else if (parent->value == _("Object"))
                    item->exp = parent->exp + '.' + item->name.toLatin1();
            } else {
                item->exp = item->name.toLatin1();
            }

            item->iname = parent->iname + '.' + item->name.toLatin1();
            item->id = propertyData.handle;
            item->type = propertyData.type;
            item->value = propertyData.value.toString();
            item->setHasChildren(propertyData.properties.count());
            d->engine->watchHandler()->insertItem(item);
        }
    }
}

void QmlV8DebuggerClient::highlightExceptionCode(int lineNumber,
                                                 const QString &filePath,
                                                 const QString &errorMessage)
{
    QList<IEditor *> editors = DocumentModel::editorsForFilePath(filePath);

    // set up the format for the errors
    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setUnderlineColor(Qt::red);

    foreach (IEditor *editor, editors) {
        TextEditor::TextEditorWidget *ed = qobject_cast<TextEditor::TextEditorWidget *>(editor->widget());
        if (!ed)
            continue;

        QList<QTextEdit::ExtraSelection> selections;
        QTextEdit::ExtraSelection sel;
        sel.format = errorFormat;
        QTextCursor c(ed->document()->findBlockByNumber(lineNumber - 1));
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
        ed->setExtraSelections(TextEditor::TextEditorWidget::DebuggerExceptionSelection, selections);

        QString message = QString(_("%1: %2: %3")).arg(filePath).arg(lineNumber)
                .arg(errorMessage);
        d->engine->showMessage(message, ConsoleOutput);
    }
}

void QmlV8DebuggerClient::clearExceptionSelection()
{
    QList<QTextEdit::ExtraSelection> selections;

    foreach (IEditor *editor, DocumentModel::editorsForOpenedDocuments()) {
        TextEditor::TextEditorWidget *ed = qobject_cast<TextEditor::TextEditorWidget *>(editor->widget());
        if (!ed)
            continue;

        ed->setExtraSelections(TextEditor::TextEditorWidget::DebuggerExceptionSelection, selections);
    }

}

} // Internal
} // Debugger
