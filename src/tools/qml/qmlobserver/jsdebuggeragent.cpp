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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qset.h>
#include <QtScript/qscriptengine.h>
#include <QtScript/QScriptContextInfo>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QDateTime>
#include <QtScript/qscriptvalueiterator.h>

QT_BEGIN_NAMESPACE

class JSAgentWatchData {
public:
    QByteArray exp;
    QString name;
    QString value;
    QString type;
    bool hasChildren;
    quint64 objectId;

    static JSAgentWatchData fromScriptValue(const QString &expression, const QScriptValue &value)
    {
        JSAgentWatchData data;
        data.exp = expression.toUtf8();
        data.name = expression;
        data.hasChildren = false;
        data.value = value.toString();
        data.objectId = value.objectId();
        if (value.isArray()) {
            data.type = QLatin1String("Array");
            data.value = QString::fromLatin1("[Array of length %1]").arg(value.property("length").toString());
            data.hasChildren = true;
        } else if (value.isBool()) {
            data.type = QLatin1String("Bool");
//            data.value = value.toBool() ? QLatin1String("true") : QLatin1String("false");
        } else if (value.isDate()) {
            data.type = QLatin1String("Date");
            data.value = value.toDateTime().toString();
        } else if (value.isError()) {
            data.type = QLatin1String("Error");
        } else if (value.isFunction()) {
            data.type = QLatin1String("Function");
        } else if (value.isUndefined()) {
            data.type = QLatin1String("<undefined>");
        } else if (value.isNumber()) {
            data.type = QLatin1String("Number");
        } else if (value.isRegExp()) {
            data.type = QLatin1String("RegExp");
        } else if (value.isString()) {
            data.type = QLatin1String("String");
        } else if (value.isVariant()) {
            data.type = QLatin1String("Variant");
        } else if (value.isQObject()) {
            const QObject *obj = value.toQObject();
            data.value = QString::fromLatin1("[%1]").arg(obj->metaObject()->className());
            data.type = QLatin1String("Object");
            data.hasChildren = true;
        } else if (value.isObject()) {
            data.type = QLatin1String("Object");
            data.hasChildren = true;
            data.type = QLatin1String("Object");
            data.value = QLatin1String("[Object]");
        } else if (value.isNull()) {
            data.type = QLatin1String("<null>");
        } else {
            data.type = QLatin1String("<unknown>");
        }
        return data;
    }
};


QDataStream& operator<<(QDataStream& s, const JSAgentWatchData& data)
{
    return s << data.exp << data.name << data.value << data.type << data.hasChildren << data.objectId;
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
        if (object.isQObject() && it.value().isFunction()) {
            // cosmetics: skip all signals and slot, there is too many of them,
            //  and it is not usefull in the debugger.
            continue;
        }
        JSAgentWatchData data = JSAgentWatchData::fromScriptValue(it.name(), it.value());
        data.exp.prepend(expPrefix);
        result << data;
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
            locals.prepend(JSAgentWatchData::fromScriptValue("this", thisObject));
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

    if(state == Stopped)
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
/* ... */
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
        State oldState = state;
        state = Stopped;
        QByteArray id;
        QString expr;
        ds >> id >> expr;

        JSAgentWatchData data = JSAgentWatchData::fromScriptValue(expr, engine()->evaluate(expr));
        knownObjectIds << data.objectId;
        // Clear any exceptions occurred during locals evaluation.
        engine()->clearExceptions();

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("RESULT") << id << data;
        sendMessage(reply);
        state = oldState;
    } else if (command == "EXPAND") {
        State oldState = state;
        state = Stopped;
        QByteArray requestId;
        quint64 objectId;
        ds >> requestId >> objectId;
        QScriptValue v;
        if (knownObjectIds.contains(objectId))
            v = engine()->objectById(objectId);

        QList<JSAgentWatchData> result = expandObject(v);
        recordKnownObjects(result);

        // Clear any exceptions occurred during locals evaluation.
        engine()->clearExceptions();

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("EXPANDED") << requestId << result;
        sendMessage(reply);
        state = oldState;
    } else if (command == "ACTIVATE_FRAME") {
        State oldState = state;
        state = Stopped;

        int frameId;
        ds >> frameId;

        int deep = 0;
        QScriptContext *ctx = engine()->currentContext();
        while (ctx && deep < frameId) {
            ctx = ctx->parentContext();
            deep++;
        }

        QList<JSAgentWatchData> locals = getLocals(ctx);

        // Clear any exceptions occurred during locals evaluation.
        engine()->clearExceptions();

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("LOCALS") << frameId << locals;
        sendMessage(reply);
        state = oldState;
    } else if (command == "SET_PROPERTY") {
        State oldState = state;
        state = Stopped;

        QByteArray id;
        qint64 objectId;
        QString property;
        QString value;
        ds >> id >> objectId >> property >> value;

        if (knownObjectIds.contains(objectId)) {
            QScriptValue object;
            object = engine()->objectById(objectId);

            if(object.isObject()) {
                QScriptValue result = engine()->evaluate(value);
                object.setProperty(property, result);
            }

            // Clear any exceptions occurred during locals evaluation.
            engine()->clearExceptions();
        }
        state = oldState;
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

void JSDebuggerAgent::stopped()
{
    knownObjectIds.clear();
    state = Stopped;
    QList<QPair<QString, QPair<QString, qint32> > > backtrace;

    for (QScriptContext* ctx = engine()->currentContext(); ctx; ctx = ctx->parentContext()) {
        QScriptContextInfo info(ctx);

        QString functionName = info.functionName();
        if (functionName.isEmpty()) {
            if (ctx->parentContext()) {
                switch (info.functionType()) {
                case QScriptContextInfo::ScriptFunction:
                    functionName = QLatin1String("<anonymous>");
                    break;
                case QScriptContextInfo::NativeFunction:
                    functionName = QLatin1String("<native>");
                    break;
                case QScriptContextInfo::QtFunction:
                case QScriptContextInfo::QtPropertyFunction:
                    functionName = QLatin1String("<native slot>");
                    break;
                }
            } else {
                functionName = QLatin1String("<global>");
            }
        }
        backtrace.append(qMakePair(functionName, qMakePair( QUrl(info.fileName()).toLocalFile(), info.lineNumber() ) ) );
    }
    QList<JSAgentWatchData> watches;
    foreach (const QString &expr, watchExpressions)
        watches << JSAgentWatchData::fromScriptValue(expr,  engine()->evaluate(expr));
    recordKnownObjects(watches);

    QList<JSAgentWatchData> locals = getLocals(engine()->currentContext());

    // Clear any exceptions occurred during locals evaluation.
    engine()->clearExceptions();

    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    rs << QByteArray("STOPPED") << backtrace << watches << locals;
    sendMessage(reply);

    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

void JSDebuggerAgent::continueExec()
{
    loop.quit();
}

void JSDebuggerAgent::enabledChanged(bool on)
{
    engine()->setAgent(on ? this : 0);
    QDeclarativeDebugService::enabledChanged(on);
}

QT_END_NAMESPACE
