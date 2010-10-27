/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtSCriptTools module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "jsdebuggeragent.h"
#include "qt_private/qdeclarativedebughelper_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qset.h>
#include <QtScript/qscriptcontextinfo.h>
#include <QtScript/qscriptengine.h>
#include <QtScript/qscriptvalueiterator.h>

namespace QmlJSDebugger {

class SetupExecEnv
{
public:
    SetupExecEnv(JSDebuggerAgent *a)
        : agent(a),
          previousState(a->state),
          hadException(a->engine()->hasUncaughtException())
    {
        agent->state = JSDebuggerAgent::Stopped;
    }

    ~SetupExecEnv() {
        if (!hadException && agent->engine()->hasUncaughtException())
            agent->engine()->clearExceptions();
        agent->state = previousState;
    }

private:
    JSDebuggerAgent *agent;
    JSDebuggerAgent::State previousState;
    bool hadException;
};

class JSAgentWatchData
{
public:
    QByteArray exp;
    QByteArray name;
    QByteArray value;
    QByteArray type;
    bool hasChildren;
    quint64 objectId;
};

QDataStream &operator<<(QDataStream &s, const JSAgentWatchData &data)
{
    return s << data.exp << data.name << data.value
        << data.type << data.hasChildren << data.objectId;
}

class JSAgentStackData
{
public:
    QByteArray functionName;
    QByteArray fileName;
    qint32 lineNumber;
};

QDataStream &operator<<(QDataStream &s, const JSAgentStackData &data)
{
    return s << data.functionName << data.fileName << data.lineNumber;
}


static JSAgentWatchData fromScriptValue(const QString &expression,
    const QScriptValue &value)
{
    static const QString arrayStr = QCoreApplication::translate
        ("Debugger::JSAgentWatchData", "[Array of length %1]");
    static const QString undefinedStr = QCoreApplication::translate
        ("Debugger::JSAgentWatchData", "<undefined>");

    JSAgentWatchData data;
    data.exp = expression.toUtf8();
    data.name = data.exp;
    data.hasChildren = false;
    data.value = value.toString().toUtf8();
    data.objectId = value.objectId();
    if (value.isArray()) {
        data.type = "Array";
        data.value = arrayStr.arg(value.property("length").toString()).toUtf8();
        data.hasChildren = true;
    } else if (value.isBool()) {
        data.type = "Bool";
        // data.value = value.toBool() ? "true" : "false";
    } else if (value.isDate()) {
        data.type = "Date";
        data.value = value.toDateTime().toString().toUtf8();
    } else if (value.isError()) {
        data.type = "Error";
    } else if (value.isFunction()) {
        data.type = "Function";
    } else if (value.isUndefined()) {
        data.type = undefinedStr.toUtf8();
    } else if (value.isNumber()) {
        data.type = "Number";
    } else if (value.isRegExp()) {
        data.type = "RegExp";
    } else if (value.isString()) {
        data.type = "String";
    } else if (value.isVariant()) {
        data.type = "Variant";
    } else if (value.isQObject()) {
        const QObject *obj = value.toQObject();
        data.type = "Object";
        data.value += '[';
        data.value += obj->metaObject()->className();
        data.value += ']';
        data.hasChildren = true;
    } else if (value.isObject()) {
        data.type = "Object";
        data.hasChildren = true;
        data.value = "[Object]";
    } else if (value.isNull()) {
        data.type = "<null>";
    } else {
        data.type = "<unknown>";
    }
    return data;
}

static QList<JSAgentWatchData> expandObject(const QScriptValue &object)
{
    QList<JSAgentWatchData> result;
    QScriptValueIterator it(object);
    QByteArray expPrefix = '@' + QByteArray::number(object.objectId(), 16) + "->";
    while (it.hasNext()) {
        it.next();
        if (it.flags() & QScriptValue::SkipInEnumeration)
            continue;
        if (/*object.isQObject() &&*/ it.value().isFunction()) {
            // Cosmetics: skip all functions and slot, there are too many of them,
            // and it is not useful information in the debugger.
            continue;
        }
        JSAgentWatchData data = fromScriptValue(it.name(), it.value());
        data.exp.prepend(expPrefix);
        result.append(data);
    }
    if (result.isEmpty()) {
        JSAgentWatchData data;
        data.name = "<no initialized data>";
        data.hasChildren = false;
        data.value = " ";
        data.exp.prepend(expPrefix);
        data.objectId = 0;
        result.append(data);
    }
    return result;
}

void JSDebuggerAgent::recordKnownObjects(const QList<JSAgentWatchData>& list)
{
    foreach (const JSAgentWatchData &data, list)
        knownObjectIds << data.objectId;
}

QList<JSAgentWatchData> JSDebuggerAgent::getLocals(QScriptContext *ctx)
{
    QList<JSAgentWatchData> locals;
    if (ctx) {
        QScriptValue activationObject = ctx->activationObject();
        QScriptValue thisObject = ctx->thisObject();
        locals = expandObject(activationObject);
        if (thisObject.isObject() && thisObject.objectId() != engine()->globalObject().objectId())
            locals.prepend(fromScriptValue("this", thisObject));
        recordKnownObjects(locals);
        knownObjectIds << activationObject.objectId();
    }
    return locals;
}

/*!
  Constructs a new agent for the given \a engine. The agent will
  report debugging-related events (e.g. step completion) to the given
  \a backend.
*/
JSDebuggerAgent::JSDebuggerAgent(QScriptEngine *engine)
    : QDeclarativeDebugService("JSDebugger"), QScriptEngineAgent(engine)
    , state(NoState)
{}

JSDebuggerAgent::JSDebuggerAgent(QDeclarativeEngine *engine)
    : QDeclarativeDebugService("JSDebugger")
    , QScriptEngineAgent(QDeclarativeDebugHelper::getScriptEngine(engine))
    , state(NoState)
{}

/*!
  Destroys this QScriptDebuggerAgent.
*/
JSDebuggerAgent::~JSDebuggerAgent()
{}

/*!
  \reimp
*/
void JSDebuggerAgent::scriptLoad(qint64 id, const QString & program,
                                      const QString &fileName, int )
{
    Q_UNUSED(program);
    filenames.insert(id, QUrl(fileName).toLocalFile());
}

/*!
  \reimp
*/
void JSDebuggerAgent::scriptUnload(qint64 id)
{
    filenames.remove(id);
}

/*!
  \reimp
*/
void JSDebuggerAgent::contextPush()
{
}

/*!
  \reimp
*/
void JSDebuggerAgent::contextPop()
{
}

/*!
  \reimp
*/
void JSDebuggerAgent::functionEntry(qint64 scriptId)
{
    Q_UNUSED(scriptId);
    stepDepth++;
}

/*!
  \reimp
*/
void JSDebuggerAgent::functionExit(qint64 scriptId, const QScriptValue &returnValue)
{
    Q_UNUSED(scriptId);
    Q_UNUSED(returnValue);
    stepDepth--;
}

/*!
  \reimp
*/
void JSDebuggerAgent::positionChange(qint64 scriptId,
                                   int lineNumber, int columnNumber)
{
    Q_UNUSED(columnNumber);

    if (state == Stopped)
        return; //no re-entrency

    // check breakpoints
    if (!breakpointList.isEmpty()) {
        QHash<qint64, QString>::const_iterator it = filenames.constFind(scriptId);
        if (it == filenames.constEnd()) {
            // It is possible that the scripts are loaded before the agent is attached
            QString filename = QUrl(QScriptContextInfo(engine()->currentContext()).fileName()).toLocalFile();
            QPair<QString, qint32> key = qMakePair(filename, lineNumber);
            it = filenames.insert(scriptId, filename);
        }
        QPair<QString, qint32> key = qMakePair(*it, lineNumber);
        if (breakpointList.contains(key)) {
            stopped();
            return;
        }
    }

    switch (state) {
    case NoState:
    case Stopped:
        // Do nothing
        break;
    case SteppingOutState:
        if (stepDepth >= 0)
            break;
        //fallthough
    case SteppingOverState:
        if (stepDepth > 0)
            break;
        //fallthough
    case SteppingIntoState:
        stopped();
        break;
    }

}

/*!
  \reimp
*/
void JSDebuggerAgent::exceptionThrow(qint64 scriptId,
                                   const QScriptValue &exception,
                                   bool hasHandler)
{
    Q_UNUSED(scriptId);
    Q_UNUSED(exception);
    Q_UNUSED(hasHandler);
//    qDebug() << Q_FUNC_INFO << exception.toString() << hasHandler;
#if 0 //sometimes, we get exceptions that we should just ignore.
    if (!hasHandler && state != Stopped)
        stopped(true, exception);
#endif
}

/*!
  \reimp
*/
void JSDebuggerAgent::exceptionCatch(qint64 scriptId,
                                          const QScriptValue &exception)
{
    Q_UNUSED(scriptId);
    Q_UNUSED(exception);
}

bool JSDebuggerAgent::supportsExtension(QScriptEngineAgent::Extension extension) const
{
    return extension == QScriptEngineAgent::DebuggerInvocationRequest;
}

QVariant JSDebuggerAgent::extension(QScriptEngineAgent::Extension extension, const QVariant& argument)
{
    if (extension == QScriptEngineAgent::DebuggerInvocationRequest) {
        stopped();
        return QVariant();
    }
    return QScriptEngineAgent::extension(extension, argument);
}



void JSDebuggerAgent::messageReceived(const QByteArray& message)
{
    QDataStream ds(message);
    QByteArray command;
    ds >> command;
    if (command == "BREAKPOINTS") {
        ds >> breakpointList;
    } else if (command == "WATCH_EXPRESSIONS") {
        ds >> watchExpressions;
    } else if (command == "STEPOVER") {
        stepDepth = 0;
        state = SteppingOverState;
        continueExec();
    } else if (command == "STEPINTO" || command == "INTERRUPT") {
        stepDepth = 0;
        state = SteppingIntoState;
        continueExec();
    } else if (command == "STEPOUT") {
        stepDepth = 0;
        state = SteppingOutState;
        continueExec();
    } else if (command == "CONTINUE") {
        state = NoState;
        continueExec();
    } else if (command == "EXEC") {
        SetupExecEnv execEnv(this);

        QByteArray id;
        QString expr;
        ds >> id >> expr;

        JSAgentWatchData data = fromScriptValue(expr, engine()->evaluate(expr));
        knownObjectIds << data.objectId;

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("RESULT") << id << data;
        sendMessage(reply);
    } else if (command == "EXPAND") {
        SetupExecEnv execEnv(this);

        QByteArray requestId;
        quint64 objectId;
        ds >> requestId >> objectId;
        QScriptValue v;
        if (knownObjectIds.contains(objectId))
            v = engine()->objectById(objectId);

        QList<JSAgentWatchData> result = expandObject(v);
        recordKnownObjects(result);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("EXPANDED") << requestId << result;
        sendMessage(reply);

    } else if (command == "ACTIVATE_FRAME") {
        SetupExecEnv execEnv(this);

        int frameId;
        ds >> frameId;

        int deep = 0;
        QScriptContext *ctx = engine()->currentContext();
        while (ctx && deep < frameId) {
            ctx = ctx->parentContext();
            deep++;
        }

        QList<JSAgentWatchData> locals = getLocals(ctx);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("LOCALS") << frameId << locals;
        sendMessage(reply);
    } else if (command == "SET_PROPERTY") {
        SetupExecEnv execEnv(this);

        QByteArray id;
        qint64 objectId;
        QString property;
        QString value;
        ds >> id >> objectId >> property >> value;

        if (knownObjectIds.contains(objectId)) {
            QScriptValue object;
            object = engine()->objectById(objectId);

            if (object.isObject()) {
                QScriptValue result = engine()->evaluate(value);
                object.setProperty(property, result);
            }
        }

        //TODO: feedback
    } else if (command == "PING") {
        int ping;
        ds >> ping;
        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("PONG") << ping;
        sendMessage(reply);
    } else {
        qDebug() << Q_FUNC_INFO << "Unknown command" << command;
    }

    QDeclarativeDebugService::messageReceived(message);
}

void JSDebuggerAgent::stopped(bool becauseOfException, const QScriptValue &exception)
{
    knownObjectIds.clear();
    state = Stopped;
    QList<JSAgentStackData> backtrace;

    for (QScriptContext* ctx = engine()->currentContext(); ctx; ctx = ctx->parentContext()) {
        QScriptContextInfo info(ctx);

        JSAgentStackData frame;
        frame.functionName = info.functionName().toUtf8();
        if (frame.functionName.isEmpty()) {
            if (ctx->parentContext()) {
                switch (info.functionType()) {
                case QScriptContextInfo::ScriptFunction:
                    frame.functionName = "<anonymous>";
                    break;
                case QScriptContextInfo::NativeFunction:
                    frame.functionName = "<native>";
                    break;
                case QScriptContextInfo::QtFunction:
                case QScriptContextInfo::QtPropertyFunction:
                    frame.functionName = "<native slot>";
                    break;
                }
            } else {
                frame.functionName = "<global>";
            }
        }
        frame.lineNumber = info.lineNumber();
        // if the line number is unknown, fallback to the function line number
        if (frame.lineNumber == -1)
            frame.lineNumber = info.functionStartLineNumber();
        frame.fileName = QUrl(info.fileName()).toLocalFile().toUtf8();
        backtrace.append(frame);
    }
    QList<JSAgentWatchData> watches;
    foreach (const QString &expr, watchExpressions)
        watches << fromScriptValue(expr, engine()->evaluate(expr));
    recordKnownObjects(watches);

    QList<JSAgentWatchData> locals = getLocals(engine()->currentContext());

    if (!becauseOfException) {
        // Clear any exceptions occurred during locals evaluation.
        engine()->clearExceptions();
    }

    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STOPPED") << backtrace << watches << locals
        << becauseOfException << exception.toString();
    sendMessage(reply);

    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

void JSDebuggerAgent::continueExec()
{
    loop.quit();
}

void JSDebuggerAgent::statusChanged(Status status)
{
    engine()->setAgent((status == QDeclarativeDebugService::Enabled) ? this : 0);
}

} // namespace QmlJSDebugger
