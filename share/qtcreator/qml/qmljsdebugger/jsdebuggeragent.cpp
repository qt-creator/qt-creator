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

#include "jsdebuggeragent.h"
#include "qt_private/qdeclarativedebughelper_p.h"

#include <qdatetime.h>
#include <qdebug.h>
#include <qcoreapplication.h>
#include <qset.h>
#include <qurl.h>
#include <qscriptcontextinfo.h>
#include <qscriptengine.h>
#include <qscriptvalueiterator.h>

namespace QmlJSDebugger {

enum JSDebuggerState
{
    NoState,
    SteppingIntoState,
    SteppingOverState,
    SteppingOutState,
    StoppedState
};

struct JSAgentWatchData
{
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

struct JSAgentStackData
{
    QByteArray functionName;
    QByteArray fileUrl;
    qint32 lineNumber;
};

QDataStream &operator<<(QDataStream &s, const JSAgentStackData &data)
{
    return s << data.functionName << data.fileUrl << data.lineNumber;
}

struct JSAgentBreakpointData
{
    QByteArray functionName;
    QByteArray fileUrl;
    qint32 lineNumber;
};

typedef QSet<JSAgentBreakpointData> JSAgentBreakpoints;

QDataStream &operator<<(QDataStream &s, const JSAgentBreakpointData &data)
{
    return s << data.functionName << data.fileUrl << data.lineNumber;
}

QDataStream &operator>>(QDataStream &s, JSAgentBreakpointData &data)
{
    return s >> data.functionName >> data.fileUrl >> data.lineNumber;
}

bool operator==(const JSAgentBreakpointData &b1, const JSAgentBreakpointData &b2)
{
    return b1.lineNumber == b2.lineNumber && b1.fileUrl == b2.fileUrl;
}

uint qHash(const JSAgentBreakpointData &b)
{
    return b.lineNumber ^ qHash(b.fileUrl);
}

class JSDebuggerAgentPrivate
{
public:
    JSDebuggerAgentPrivate(JSDebuggerAgent *q)
        : q(q), state(NoState)
    {}

    void continueExec();
    void recordKnownObjects(const QList<JSAgentWatchData> &);
    QList<JSAgentWatchData> getLocals(QScriptContext *);
    void positionChange(qint64 scriptId, int lineNumber, int columnNumber);
    QScriptEngine *engine() { return q->engine(); }
    void stopped();
    void messageReceived(const QByteArray &message);
    void sendMessage(const QByteArray &message) { q->sendMessage(message); }

public:
    JSDebuggerAgent *q;
    JSDebuggerState state;
    int stepDepth;
    int stepCount;

    QEventLoop loop;
    QHash<qint64, QString> filenames;
    JSAgentBreakpoints breakpoints;
    // breakpoints by filename (without path)
    QHash<QString, JSAgentBreakpointData> fileNameToBreakpoints;
    QStringList watchExpressions;
    QSet<qint64> knownObjectIds;
};

class SetupExecEnv
{
public:
    SetupExecEnv(JSDebuggerAgentPrivate *a)
        : agent(a),
          previousState(a->state),
          hadException(a->engine()->hasUncaughtException())
    {
        agent->state = StoppedState;
    }

    ~SetupExecEnv()
    {
        if (!hadException && agent->engine()->hasUncaughtException())
            agent->engine()->clearExceptions();
        agent->state = previousState;
    }

private:
    JSDebuggerAgentPrivate *agent;
    JSDebuggerState previousState;
    bool hadException;
};

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
        result.append(data);
    }
    if (result.isEmpty()) {
        JSAgentWatchData data;
        data.name = "<no initialized data>";
        data.hasChildren = false;
        data.value = " ";
        data.objectId = 0;
        result.append(data);
    }
    return result;
}

void JSDebuggerAgentPrivate::recordKnownObjects(const QList<JSAgentWatchData>& list)
{
    foreach (const JSAgentWatchData &data, list)
        knownObjectIds << data.objectId;
}

QList<JSAgentWatchData> JSDebuggerAgentPrivate::getLocals(QScriptContext *ctx)
{
    QList<JSAgentWatchData> locals;
    if (ctx) {
        QScriptValue activationObject = ctx->activationObject();
        QScriptValue thisObject = ctx->thisObject();
        locals = expandObject(activationObject);
        if (thisObject.isObject()
                && thisObject.objectId() != engine()->globalObject().objectId()
                && QScriptValueIterator(thisObject).hasNext())
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
    : QDeclarativeDebugService("JSDebugger")
    , QScriptEngineAgent(engine)
    , d(new JSDebuggerAgentPrivate(this))
{
    if (status() == Enabled)
        engine->setAgent(this);
}

JSDebuggerAgent::JSDebuggerAgent(QDeclarativeEngine *engine)
    : QDeclarativeDebugService("JSDebugger")
    , QScriptEngineAgent(QDeclarativeDebugHelper::getScriptEngine(engine))
    , d(new JSDebuggerAgentPrivate(this))
{
    if (status() == Enabled)
        QDeclarativeDebugHelper::getScriptEngine(engine)->setAgent(this);
}

/*!
  Destroys this QScriptDebuggerAgent.
*/
JSDebuggerAgent::~JSDebuggerAgent()
{
    delete d;
}

/*!
  \reimp
*/
void JSDebuggerAgent::scriptLoad(qint64 id, const QString &program,
                                 const QString &fileName, int)
{
    Q_UNUSED(program);
    d->filenames.insert(id, fileName);
}

/*!
  \reimp
*/
void JSDebuggerAgent::scriptUnload(qint64 id)
{
    d->filenames.remove(id);
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
    d->stepDepth++;
}

/*!
  \reimp
*/
void JSDebuggerAgent::functionExit(qint64 scriptId, const QScriptValue &returnValue)
{
    Q_UNUSED(scriptId);
    Q_UNUSED(returnValue);
    d->stepDepth--;
}

/*!
  \reimp
*/
void JSDebuggerAgent::positionChange(qint64 scriptId, int lineNumber, int columnNumber)
{
    d->positionChange(scriptId, lineNumber, columnNumber);
}

QString fileName(const QString &fileUrl)
{
    int lastDelimiterPos = fileUrl.lastIndexOf(QLatin1Char('/'));
    return fileUrl.mid(lastDelimiterPos, fileUrl.size() - lastDelimiterPos);
}

void JSDebuggerAgentPrivate::positionChange(qint64 scriptId, int lineNumber, int columnNumber)
{
    Q_UNUSED(columnNumber);

    if (state == StoppedState)
        return; //no re-entrency

    // check breakpoints
    if (!breakpoints.isEmpty()) {
        QScriptContext *ctx = engine()->currentContext();
        QScriptContextInfo info(ctx);

        if (info.functionType() == QScriptContextInfo::ScriptFunction) {
            QHash<qint64, QString>::const_iterator it = filenames.constFind(scriptId);
            if (it == filenames.constEnd()) {
                // It is possible that the scripts are loaded before the agent is attached
                QString filename = info.fileName();

                JSAgentStackData frame;
                frame.functionName = info.functionName().toUtf8();
                it = filenames.insert(scriptId, filename);
            }

            const QString filePath = it->toUtf8();
            JSAgentBreakpoints bps = fileNameToBreakpoints.values(fileName(filePath)).toSet();

            foreach (const JSAgentBreakpointData &bp, bps) {
                if (bp.lineNumber == lineNumber) {
                    stopped();
                    return;
                }
            }
        }
    }

    switch (state) {
    case NoState:
    case StoppedState:
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
    if (!hasHandler && state != StoppedState)
        stopped(true, exception);
#endif
}

/*!
  \reimp
*/
void JSDebuggerAgent::exceptionCatch(qint64 scriptId, const QScriptValue &exception)
{
    Q_UNUSED(scriptId);
    Q_UNUSED(exception);
}

bool JSDebuggerAgent::supportsExtension(Extension extension) const
{
    return extension == QScriptEngineAgent::DebuggerInvocationRequest;
}

QVariant JSDebuggerAgent::extension(Extension extension, const QVariant &argument)
{
    if (extension == QScriptEngineAgent::DebuggerInvocationRequest) {
        d->stopped();
        return QVariant();
    }
    return QScriptEngineAgent::extension(extension, argument);
}

void JSDebuggerAgent::messageReceived(const QByteArray &message)
{
    d->messageReceived(message);
}

void JSDebuggerAgentPrivate::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);
    QByteArray command;
    ds >> command;
    if (command == "BREAKPOINTS") {
        ds >> breakpoints;

        fileNameToBreakpoints.clear();
        foreach (const JSAgentBreakpointData &bp, breakpoints) {
            fileNameToBreakpoints.insertMulti(fileName(bp.fileUrl), bp);
        }

        //qDebug() << "BREAKPOINTS";
        //foreach (const JSAgentBreakpointData &bp, breakpoints)
        //    qDebug() << "BREAKPOINT: " << bp.fileName << bp.lineNumber;
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

        QList<JSAgentWatchData> watches;
        QList<JSAgentWatchData> locals = getLocals(ctx);

        // re-evaluate watches given the frame's context
        QScriptContext *currentCtx = engine()->pushContext();
        currentCtx->setActivationObject(ctx->activationObject());
        currentCtx->setThisObject(ctx->thisObject());
        foreach (const QString &expr, watchExpressions)
            watches << fromScriptValue(expr, engine()->evaluate(expr));
        recordKnownObjects(watches);
        engine()->popContext();

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("LOCALS") << frameId << locals << watches;
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

    q->baseMessageReceived(message);
}

void JSDebuggerAgentPrivate::stopped()
{
    bool becauseOfException = false;
    const QScriptValue &exception = QScriptValue();

    knownObjectIds.clear();
    state = StoppedState;
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

        frame.fileUrl = info.fileName().toUtf8();
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

void JSDebuggerAgentPrivate::continueExec()
{
    loop.quit();
}

void JSDebuggerAgent::statusChanged(Status status)
{
    engine()->setAgent((status == QDeclarativeDebugService::Enabled) ? this : 0);
}

void JSDebuggerAgent::baseMessageReceived(const QByteArray &message)
{
    QDeclarativeDebugService::messageReceived(message);
}

} // namespace QmlJSDebugger
