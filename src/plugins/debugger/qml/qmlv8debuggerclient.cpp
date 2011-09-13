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

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QTextBlock>
#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QtGui/QTextDocument>
#include <QtGui/QMessageBox>

#define INITIALPARAMS "seq" << ':' << ++d->sequence << ',' << "type" << ':' << "request"

using namespace Core;
using namespace Json;

namespace Debugger {
namespace Internal {

struct ExceptionInfo
{
    int sourceLine;
    QString filePath;
    QString errorMessage;
};

class QmlV8DebuggerClientPrivate
{
public:
    explicit QmlV8DebuggerClientPrivate(QmlV8DebuggerClient *) :
        handleException(false),
        sequence(0),
        ping(0),
        engine(0)
    {
    }

    bool handleException;
    int sequence;
    int ping;
    QmlEngine *engine;
    QHash<BreakpointModelId,int> breakpoints;
    QHash<int,BreakpointModelId> breakpointsSync;
    QHash<int,QByteArray> locals;
    QHash<int,QByteArray> watches;
    QByteArray frames;
    QScopedPointer<ExceptionInfo> exceptionInfo;
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

QByteArray QmlV8DebuggerClient::packMessage(const QByteArray &message)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "V8DEBUG";
    rs << cmd << message;
    return reply;
}

void QmlV8DebuggerClient::breakOnException(Exceptions exceptionsType, bool enabled)
{
    //TODO: Have to deal with NoExceptions
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "setexceptionbreak";

    JsonInputStream(request) << ',' << "arguments" << ':';
    if (exceptionsType == AllExceptions)
        JsonInputStream(request) << '{' << "type" << ':' << "all";
    else if (exceptionsType == UncaughtExceptions)
        JsonInputStream(request) << '{' << "type" << ':' << "uncaught";

    JsonInputStream(request) << ',' << "enabled" << ':' << enabled;
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';


    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::storeExceptionInformation(const QByteArray &message)
{
    JsonValue response(message);

    JsonValue body = response.findChild("body");

    d->exceptionInfo.reset(new ExceptionInfo);
    d->exceptionInfo->sourceLine = body.findChild("sourceLine").toVariant().toInt();
    QUrl fileUrl(body.findChild("script").findChild("name").toVariant().toString());
    d->exceptionInfo->filePath = d->engine->toFileInProject(fileUrl);
    d->exceptionInfo->errorMessage = body.findChild("exception").findChild("text").toVariant().toString();
}

void QmlV8DebuggerClient::handleException()
{
    EditorManager *editorManager = EditorManager::instance();
    QList<IEditor *> openedEditors = editorManager->openedEditors();

    // set up the format for the errors
    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setUnderlineColor(Qt::red);

    foreach (IEditor *editor, openedEditors) {
        if (editor->file()->fileName() == d->exceptionInfo->filePath) {
            TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
            if (!ed)
                continue;

            QList<QTextEdit::ExtraSelection> selections;
            QTextEdit::ExtraSelection sel;
            sel.format = errorFormat;
            QTextCursor c(ed->document()->findBlockByNumber(d->exceptionInfo->sourceLine));
            const QString text = c.block().text();
            for (int i = 0; i < text.size(); ++i) {
                if (! text.at(i).isSpace()) {
                    c.setPosition(c.position() + i);
                    break;
                }
            }
            c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            sel.cursor = c;

            sel.format.setToolTip(d->exceptionInfo->errorMessage);

            selections.append(sel);
            ed->setExtraSelections(TextEditor::BaseTextEditorWidget::DebuggerExceptionSelection, selections);

            d->engine->showMessage(d->exceptionInfo->errorMessage, ScriptConsoleOutput);
        }
    }

    //Delete the info even if the code hasnt been highlighted
    d->exceptionInfo.reset();
}

void QmlV8DebuggerClient::clearExceptionSelection()
{
    //Check if break was due to exception
    if (d->handleException) {
        EditorManager *editorManager = EditorManager::instance();
        QList<IEditor *> openedEditors = editorManager->openedEditors();
        QList<QTextEdit::ExtraSelection> selections;

        foreach (IEditor *editor, openedEditors) {
            TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
            if (!ed)
                continue;

            ed->setExtraSelections(TextEditor::BaseTextEditorWidget::DebuggerExceptionSelection, selections);
        }
        d->handleException = false;
    }
}

void QmlV8DebuggerClient::continueDebugging(StepAction type)
{
    clearExceptionSelection();

    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "continue";

    if (type != Continue) {
        JsonInputStream(request) << ',' << "arguments" << ':';

        switch (type) {
        case In: JsonInputStream(request) << '{' << "stepaction" << ':' << "in";
            break;
        case Out: JsonInputStream(request) << '{' << "stepaction" << ':' << "out";
            break;
        case Next: JsonInputStream(request) << '{' << "stepaction" << ':' << "next";
            break;
        default:break;
        }

        JsonInputStream(request) << '}';
    }

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::executeStep()
{
    continueDebugging(In);
}

void QmlV8DebuggerClient::executeStepOut()
{
    continueDebugging(Out);
}

void QmlV8DebuggerClient::executeNext()
{
    continueDebugging(Next);
}

void QmlV8DebuggerClient::executeStepI()
{
    continueDebugging(In);
}

void QmlV8DebuggerClient::continueInferior()
{
    continueDebugging(Continue);
}

void QmlV8DebuggerClient::interruptInferior()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "interrupt";

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));

}

void QmlV8DebuggerClient::startSession()
{
    //Set up Exception Handling first
    //TODO: For now we enable breaks for all exceptions
    breakOnException(AllExceptions, true);

    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "connect";

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::endSession()
{
    clearExceptionSelection();

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

bool QmlV8DebuggerClient::acceptsBreakpoint(const BreakpointModelId &id)
{
    BreakpointType type = d->engine->breakHandler()->breakpointData(id).type;
    return ((type == BreakpointOnSignalHandler) || (type == BreakpointByFunction));
}

void QmlV8DebuggerClient::insertBreakpoint(const BreakpointModelId &id)
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
    } else if (handler->breakpointData(id).type == BreakpointOnSignalHandler) {
        JsonInputStream(request) << "type" << ':' << "event";
        JsonInputStream(request) << ',' << "target" << ':' << handler->functionName(id).toUtf8();
    }
    JsonInputStream(request) << '}';
    JsonInputStream(request) << '}';

    d->breakpointsSync.insert(d->sequence,id);
    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::removeBreakpoint(const BreakpointModelId &id)
{
    int breakpoint = d->breakpoints.value(id);
    d->breakpoints.remove(id);

    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "clearbreakpoint";

    JsonInputStream(request) << ',' << "arguments" << ':';
    JsonInputStream(request) << '{' << "breakpoint" << ':' << breakpoint;
    JsonInputStream(request) << '}';

    JsonInputStream(request) << '}';

    sendMessage(packMessage(request));
}

void QmlV8DebuggerClient::changeBreakpoint(const BreakpointModelId &/*id*/)
{
}

void QmlV8DebuggerClient::updateBreakpoints()
{
}

void QmlV8DebuggerClient::assignValueInDebugger(const QByteArray /*expr*/, const quint64 &/*id*/,
                                                const QString &/*property*/, const QString &/*value*/)
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

void QmlV8DebuggerClient::listBreakpoints()
{
    QByteArray request;

    JsonInputStream(request) << '{' << INITIALPARAMS ;
    JsonInputStream(request) << ',' << "command" << ':' << "listbreakpoints";
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
        const QString type = value.findChild("type").toVariant().toString();

        if (type == "response") {

            if (!value.findChild("success").toVariant().toBool()) {
                //TODO:: Error
                qDebug() << Q_FUNC_INFO << value.toString(true,2);
                return;
            }

            const QString debugCommand(value.findChild("command").toVariant().toString());
            if (debugCommand == "backtrace") {
                setStackFrames(response);

            } else if (debugCommand == "lookup") {
                expandLocal(response);

            } else if (debugCommand == "setbreakpoint") {
                int sequence = value.findChild("request_seq").toVariant().toInt();
                int breakpoint = value.findChild("body").findChild("breakpoint").toVariant().toInt();
                BreakpointModelId id = d->breakpointsSync.take(sequence);
                d->breakpoints.insert(id,breakpoint);

                //If this is an event breakpoint then set state = BreakpointInsertOk
                const QString breakpointType = value.findChild("body").findChild("type").toVariant().toString();
                if (breakpointType == "event") {
                    d->engine->breakHandler()->notifyBreakpointInsertOk(id);
                }

            } else if (debugCommand == "evaluate") {
                setExpression(response);

            } else if (debugCommand == "listbreakpoints") {
                updateBreakpoints(response);
                backtrace();

            } else {
                //TODO::
                //qDebug() << Q_FUNC_INFO << value.toString(true,2);
            }

        } else if (type == "event") {
            const QString event(value.findChild("event").toVariant().toString());

            if (event == "break") {
                d->engine->inferiorSpontaneousStop();
                listBreakpoints();
            } else if (event == "exception") {
                d->handleException = true;
                d->engine->inferiorSpontaneousStop();
                storeExceptionInformation(response);
                backtrace();
            }
        }
    }
}

void QmlV8DebuggerClient::setStackFrames(const QByteArray &message)
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

    if (!ideStackFrames.isEmpty()) {
        setLocals(0);
        d->engine->gotoLocation(ideStackFrames.value(0));
    }

    if (d->handleException) {
        handleException();
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

void QmlV8DebuggerClient::expandLocal(const QByteArray &message)
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

void QmlV8DebuggerClient::setExpression(const QByteArray &message)
{
    JsonValue response(message);
    JsonValue body = response.findChild("body");

    int seq = response.findChild("request_seq").toVariant().toInt();

    //Console
    if (!d->watches.contains(seq)) {
        d->engine->showMessage(body.findChild("text").toVariant().toString(), ScriptConsoleOutput);
        return;
    }

    //TODO: For watch point
}

void QmlV8DebuggerClient::updateBreakpoints(const QByteArray &message)
{
    JsonValue response(message);

    JsonValue body = response.findChild("body");

    QList<JsonValue> breakpoints = body.findChild("breakpoints").children();
    BreakHandler *handler = d->engine->breakHandler();

    foreach (const JsonValue &bp, breakpoints) {

        int bpIndex = bp.findChild("number").toVariant().toInt();
        BreakpointModelId id = d->breakpoints.key(bpIndex);
        BreakpointResponse br = handler->response(id);

        if (!br.pending)
            continue;

        br.hitCount = bp.findChild("hit_count").toVariant().toInt();

        QList<JsonValue> actualLocations = bp.findChild("actual_locations").children();
        foreach (const JsonValue &location, actualLocations) {
            int line = location.findChild("line").toVariant().toInt() + 1; //Add the offset
            br.lineNumber = line;
            br.correctedLineNumber = line;
            handler->setResponse(id,br);
            handler->notifyBreakpointInsertOk(id);
        }
    }
}

void QmlV8DebuggerClient::setPropertyValue(const JsonValue &refs, const JsonValue &property, const QByteArray &prepend)
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
