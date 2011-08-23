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

#include "watchdata.h"
#include "watchhandler.h"
#include "breakpoint.h"
#include "breakhandler.h"
#include "debuggerconstants.h"
#include "qmlengine.h"
#include "stackhandler.h"
#include "debuggercore.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QtGui/QTextDocument>
#include <QtGui/QMessageBox>

#define INITIALPARAMS "seq" << ':' << ++d->sequence << ',' << "type" << ':' << "request"

using namespace Json;

namespace Debugger {
namespace Internal {

class QmlV8DebuggerClientPrivate
{
public:
    explicit QmlV8DebuggerClientPrivate(QmlV8DebuggerClient *) :
        sequence(0), ping(0), engine(0)
    {

    }

    int sequence;
    int ping;
    QmlEngine *engine;
    QHash<BreakpointModelId,int> breakpoints;
    QHash<int,BreakpointModelId> breakpointsSync;
    QHash<int,QByteArray> locals;
    QHash<int,QByteArray> watches;
    QByteArray frames;
};

QmlV8DebuggerClient::QmlV8DebuggerClient(QmlJsDebugClient::QDeclarativeDebugConnection* client)
    : QmlDebuggerClient(client, QLatin1String("V8Debugger")),
      d(new QmlV8DebuggerClientPrivate(this))
{
}

QmlV8DebuggerClient::~QmlV8DebuggerClient()
{
    delete d;
}

QByteArray QmlV8DebuggerClient::packMessage(QByteArray& message)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "V8DEBUG";
    rs << cmd << message;
    return reply;
}

void QmlV8DebuggerClient::executeStep()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "continue";

    JsonInputStream(request) << ',' << "arguments" << ':';
    JsonInputStream(request) << '{' << "stepaction" << ':' << "in";
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';


    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::executeStepOut()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "continue";

    JsonInputStream(request) << ',' << "arguments" << ':';
    JsonInputStream(request) << '{' << "stepaction" << ':' << "out";
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';


    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::executeNext()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "continue";

    JsonInputStream(request) << ',' << "arguments" << ':';
    JsonInputStream(request) << '{' << "stepaction" << ':' << "next";
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';


    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::executeStepI()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "continue";

    JsonInputStream(request) << ',' << "arguments" << ':';
    JsonInputStream(request) << '{' << "stepaction" << ':' << "in";
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';


    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::continueInferior()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "continue";
    JsonInputStream(request) << '}';


    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::interruptInferior()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "interrupt";

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::connect()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "connect";

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::disconnect()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "disconnect";

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::activateFrame(int index)
{
    setLocals(index);
}

void QmlV8DebuggerClient::insertBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = d->engine->breakHandler();
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "setbreakpoint";
    JsonInputStream(request) << ',' << "arguments" << ':' << '{';
    if (handler->breakpointData(id).type == BreakpointByFileAndLine) {
        JsonInputStream(request) << "type" << ':' << "script";
        JsonInputStream(request) << ',' << "target" << ':' << QFileInfo(handler->fileName(id)).fileName().toUtf8();
        JsonInputStream(request) << ',' << "line" << ':' << handler->lineNumber(id) - 1;
    } else if (handler->breakpointData(id).type == BreakpointByFunction) {
        JsonInputStream(request) << "type" << ':' << "function";
        JsonInputStream(request) << ',' << "target" << ':' << handler->functionName(id).toUtf8();
    }
    JsonInputStream(request) << '}';
    JsonInputStream(request) << '}';

    d->breakpointsSync.insert(d->sequence,id);
    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::removeBreakpoint(BreakpointModelId id)
{
    QList<int> breakpoints = d->breakpoints.values(id);
    d->breakpoints.remove(id);

    foreach (int bp, breakpoints) {
        QByteArray request;

        JsonInputStream(request) << '{' << INITIALPARAMS ;
        JsonInputStream(request) << ',' << "command" << ':' << "clearbreakpoint";

        JsonInputStream(request) << ',' << "arguments" << ':';
        JsonInputStream(request) << '{' << "breakpoint" << ':' << bp;
        JsonInputStream(request) << '}';

        JsonInputStream(request) << '}';

        sendMessage(packMessage(request));
    }
}

void QmlV8DebuggerClient::changeBreakpoint(BreakpointModelId /*id*/)
{
}

void QmlV8DebuggerClient::updateBreakpoints()
{
}

void QmlV8DebuggerClient::assignValueInDebugger(const QByteArray /*expr*/, const quint64 &/*id*/,
                                                const QString &/*property*/, const QString /*value*/)
{
    //TODO::
}

void QmlV8DebuggerClient::updateWatchData(const WatchData *data)
{
    if (!data->iname.startsWith("watch."))
        return;

    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "evaluate";

    JsonInputStream(request) << ',' << "arguments" << ':';
    JsonInputStream(request) << '{' << "expression" << ':' << data->exp;
    JsonInputStream(request) << ',' << "frame" << ':' << d->engine->stackHandler()->currentFrame().level;
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';

    d->watches.insert(d->sequence,data->iname);

    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::executeDebuggerCommand(const QString &command)
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "evaluate";

    JsonInputStream(request) << ',' << "arguments" << ':';
    JsonInputStream(request) << '{' << "expression" << ':' << command;
    JsonInputStream(request) << ',' << "global" << ':' << true;
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::synchronizeWatchers(const QStringList &/*watchers*/)
{
    //TODO:: send watchers list
}

void QmlV8DebuggerClient::expandObject(const QByteArray &iname, quint64 objectId)
{
    d->locals.insert(objectId,iname);
    QList<int> ids;
    ids.append(objectId);

    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "lookup";

    JsonInputStream(request) << ',' << "arguments" << ':';
    JsonInputStream(request) << '{' << "handles" << ':' << ids;
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::backtrace()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "backtrace";
    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == "V8DEBUG") {
        QByteArray response;
        ds >> response;

        JsonValue value(response);
        QString type = value.findChild("type").toVariant().toString();

        if (type == "response") {

            if (!value.findChild("success").toVariant().toBool()) {
                //TODO:: Error
                qDebug() << Q_FUNC_INFO << value.toString(true,2);
                return;
            }

            QString debugCommand(value.findChild("command").toVariant().toString());
            if (debugCommand == "backtrace") {
                setStackFrames(response);

            } else if (debugCommand == "lookup") {
                expandLocal(response);

            } else if (debugCommand == "setbreakpoint") {
                int sequence = value.findChild("request_seq").toVariant().toInt();
                int breakpoint = value.findChild("body").findChild("breakpoint").toVariant().toInt();
                d->breakpoints.insertMulti(d->breakpointsSync.take(sequence),breakpoint);

            } else if (debugCommand == "evaluate") {
                setExpression(response);

            } else {
                //TODO::
                //qDebug() << Q_FUNC_INFO << value.toString(true,2);
            }

        } else if (type == "event") {
            QString event(value.findChild("event").toVariant().toString());

            if (event == "break") {
                d->engine->inferiorSpontaneousStop();
                backtrace();
            }
        }
    }
}

void QmlV8DebuggerClient::setStackFrames(QByteArray &message)
{
    d->frames = message;
    JsonValue response(message);

    JsonValue refs = response.findChild("refs");
    JsonValue body = response.findChild("body");

    int totalFrames = body.findChild("totalFrames").toVariant().toInt();
    JsonValue stackFrames = body.findChild("frames");

    StackFrames ideStackFrames;
    for (int i = 0; i != totalFrames; ++i) {

        JsonValue stackFrame = stackFrames.childAt(i);

        StackFrame frame;

        int frameIndex = stackFrame.findChild("index").toVariant().toInt();
        frame.level = frameIndex;

        frame.line = stackFrame.findChild("line").toVariant().toInt() + 1;

        int index = indexInRef(refs,stackFrame.findChild("func").findChild("ref").toVariant().toInt());
        if (index != -1) {
            JsonValue func = refs.childAt(index);
            frame.function = func.findChild("name").toVariant().toString();
        }

        index = indexInRef(refs,stackFrame.findChild("script").findChild("ref").toVariant().toInt());
        if (index != -1) {
            JsonValue script = refs.childAt(index);
            frame.file = d->engine->toFileInProject(script.findChild("name").toVariant().toString());
            frame.usable = QFileInfo(frame.file).isReadable();
        }
        ideStackFrames << frame;
    }

    d->engine->stackHandler()->setFrames(ideStackFrames);

    QString fileName;
    QString file;
    QString function;
    int line = -1;

    if (!ideStackFrames.isEmpty()) {
        file = ideStackFrames.at(0).file;
        fileName = QFileInfo(file).fileName();
        line = ideStackFrames.at(0).line;
        function = ideStackFrames.at(0).function;
    }

    BreakHandler *handler = d->engine->breakHandler();
    foreach (BreakpointModelId id, handler->engineBreakpointIds(d->engine)) {
        QString processedFilename = QFileInfo(handler->fileName(id)).fileName();
        if (processedFilename == fileName && handler->lineNumber(id) == line) {
            QTC_ASSERT(handler->state(id) == BreakpointInserted,/**/);
            BreakpointResponse br = handler->response(id);
            br.fileName = file;
            br.lineNumber = line;
            br.functionName = function;
            handler->setResponse(id, br);
        }
    }

    if (!ideStackFrames.isEmpty()) {
        setLocals(0);
        d->engine->gotoLocation(ideStackFrames.value(0));
    }

}

void QmlV8DebuggerClient::setLocals(int frameIndex)
{
    JsonValue response(d->frames);

    JsonValue refs = response.findChild("refs");
    JsonValue body = response.findChild("body");

    int totalFrames = body.findChild("totalFrames").toVariant().toInt();
    JsonValue stackFrames = body.findChild("frames");


    for (int i = 0; i != totalFrames; ++i) {

        JsonValue stackFrame = stackFrames.childAt(i);
        int index = stackFrame.findChild("index").toVariant().toInt();
        if (index != frameIndex)
            continue;

        JsonValue locals = stackFrame.findChild("locals");

        d->engine->watchHandler()->beginCycle();

        int localsCount = locals.childCount();
        for (int j = 0; j != localsCount; ++j) {
            JsonValue local = locals.childAt(j);

            WatchData data;
            data.exp = local.findChild("name").toVariant().toByteArray();
            //Check for v8 specific local
            if (data.exp.startsWith("."))
                continue;

            data.name = data.exp;
            data.iname = "local." + data.exp;
            JsonValue val = refs.childAt(indexInRef(refs,local.findChild("value").findChild("ref").toVariant().toInt()));
            data.type = val.findChild("type").toVariant().toByteArray();

            if (data.type == "object") {
                data.hasChildren = true;
                data.value = val.findChild("className").toVariant().toByteArray();

            } else if (data.type == "function" || data.type == "undefined") {
                data.hasChildren = false;
                data.value = val.findChild("text").toVariant().toByteArray();

            } else {
                data.hasChildren = false;
                data.value = val.findChild("value").toVariant().toByteArray();
            }

            data.id = val.findChild("handle").toVariant().toInt();

            d->engine->watchHandler()->insertData(data);

            if (d->engine->watchHandler()->expandedINames().contains(data.iname)) {
                expandObject(data.iname, data.id);
            }
        }

        d->engine->watchHandler()->endCycle();
    }
}

void QmlV8DebuggerClient::expandLocal(QByteArray &message)
{
    JsonValue response(message);

    JsonValue refs = response.findChild("refs");
    JsonValue body = response.findChild("body");
    JsonValue details = body.childAt(0);

    int id = QString(details.name()).toInt();
    QByteArray prepend = d->locals.take(id);

    JsonValue properties = details.findChild("properties");
    int propertiesCount = properties.childCount();

    for (int k = 0; k != propertiesCount; ++k) {
        JsonValue property = properties.childAt(k);
        setPropertyValue(refs,property,prepend);
    }
}

void QmlV8DebuggerClient::setExpression(QByteArray &message)
{
    JsonValue response(message);
    JsonValue body = response.findChild("body");

    int seq = response.findChild("request_seq").toVariant().toInt();

    if (!d->watches.contains(seq)) {
        d->engine->showMessage(body.findChild("text").toVariant().toString(), ScriptConsoleOutput);
        return;
    }
    //TODO::
//    JsonValue refs = response.findChild("refs");
//    JsonValue body = response.findChild("body");
//    JsonValue details = body.childAt(0);

//    int id = QString(details.name()).toInt();
//    QByteArray prepend = d->locals.take(id);

//    JsonValue properties = details.findChild("properties");
//    int propertiesCount = properties.childCount();

//    for (int k = 0; k != propertiesCount; ++k) {
//        JsonValue property = properties.childAt(k);
//        setPropertyValue(refs,property,prepend);
//    }
}

void QmlV8DebuggerClient::setPropertyValue(JsonValue &refs, JsonValue &property, QByteArray &prepend)
{
    WatchData data;
    data.exp = property.findChild("name").toVariant().toByteArray();
    data.name = data.exp;
    data.iname = prepend + '.' + data.exp;
    JsonValue val = refs.childAt(indexInRef(refs,property.findChild("ref").toVariant().toInt()));
    data.type = val.findChild("type").toVariant().toByteArray();

    if (data.type == "object") {
        data.hasChildren = true;
        data.value = val.findChild("className").toVariant().toByteArray();

    } else if (data.type == "function") {
        data.hasChildren = false;
        data.value = val.findChild("text").toVariant().toByteArray();

    } else {
        data.hasChildren = false;
        data.value = val.findChild("value").toVariant().toByteArray();
    }

    data.id = val.findChild("handle").toVariant().toInt();

    d->engine->watchHandler()->insertData(data);

    if (d->engine->watchHandler()->expandedINames().contains(data.iname)) {
        expandObject(data.iname, data.id);
    }
}

int QmlV8DebuggerClient::indexInRef(const JsonValue &refs, int refIndex)
{
    for (int i = 0; i != refs.childCount(); ++i) {
        JsonValue ref = refs.childAt(i);
        int index = ref.findChild("handle").toVariant().toInt();
        if (index == refIndex)
            return i;
    }
    return -1;
}

void QmlV8DebuggerClient::setEngine(QmlEngine *engine)
{
    d->engine = engine;
}

} // Internal
} // Debugger
