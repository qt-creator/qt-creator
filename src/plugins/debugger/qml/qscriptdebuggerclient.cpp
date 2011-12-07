/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "qscriptdebuggerclient.h"

#include "watchdata.h"
#include "watchhandler.h"
#include "breakpoint.h"
#include "breakhandler.h"
#include "debuggerconstants.h"
#include "qmlengine.h"
#include "stackhandler.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <QTextDocument>
#include <QFileInfo>
#include <QMessageBox>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

struct JSAgentBreakpointData
{
    QByteArray functionName;
    QByteArray fileUrl;
    qint32 lineNumber;
};

struct JSAgentStackData
{
    QByteArray functionName;
    QByteArray fileUrl;
    qint32 lineNumber;
};

uint qHash(const JSAgentBreakpointData &b)
{
    return b.lineNumber ^ qHash(b.fileUrl);
}

QDataStream &operator<<(QDataStream &s, const JSAgentBreakpointData &data)
{
    return s << data.functionName << data.fileUrl << data.lineNumber;
}

QDataStream &operator<<(QDataStream &s, const JSAgentStackData &data)
{
    return s << data.functionName << data.fileUrl << data.lineNumber;
}

QDataStream &operator>>(QDataStream &s, JSAgentBreakpointData &data)
{
    return s >> data.functionName >> data.fileUrl >> data.lineNumber;
}

QDataStream &operator>>(QDataStream &s, JSAgentStackData &data)
{
    return s >> data.functionName >> data.fileUrl >> data.lineNumber;
}

bool operator==(const JSAgentBreakpointData &b1, const JSAgentBreakpointData &b2)
{
    return b1.lineNumber == b2.lineNumber && b1.fileUrl == b2.fileUrl;
}

typedef QSet<JSAgentBreakpointData> JSAgentBreakpoints;
typedef QList<JSAgentStackData> JSAgentStackFrames;


static QDataStream &operator>>(QDataStream &s, WatchData &data)
{
    data = WatchData();
    QByteArray name;
    QByteArray value;
    QByteArray type;
    bool hasChildren = false;
    s >> data.exp >> name >> value >> type >> hasChildren >> data.id;
    data.name = QString::fromUtf8(name);
    data.setType(type, false);
    data.setValue(QString::fromUtf8(value));
    data.setHasChildren(hasChildren);
    data.setAllUnneeded();
    return s;
}

class QScriptDebuggerClientPrivate
{
public:
    explicit QScriptDebuggerClientPrivate(QScriptDebuggerClient *) :
        ping(0), engine(0)
    {

    }

    int ping;
    QmlEngine *engine;
    JSAgentBreakpoints breakpoints;

    void logSendMessage(const QString &msg) const;
    void logReceiveMessage(const QString &msg) const;
};

QScriptDebuggerClient::QScriptDebuggerClient(QmlJsDebugClient::QDeclarativeDebugConnection* client)
    : QmlDebuggerClient(client, QLatin1String("JSDebugger")),
      d(new QScriptDebuggerClientPrivate(this))
{
}

QScriptDebuggerClient::~QScriptDebuggerClient()
{
    delete d;
}

void QScriptDebuggerClient::executeStep()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPINTO";
    rs << cmd;
    d->logSendMessage(cmd);
    sendMessage(reply);
}

void QScriptDebuggerClient::executeStepOut()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPOUT";
    rs << cmd;
    d->logSendMessage(cmd);
    sendMessage(reply);
}

void QScriptDebuggerClient::executeNext()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPOVER";
    rs << cmd;
    d->logSendMessage(cmd);
    sendMessage(reply);
}

void QScriptDebuggerClient::executeStepI()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPINTO";
    rs << cmd;
    d->logSendMessage(cmd);
    sendMessage(reply);
}

void QScriptDebuggerClient::continueInferior()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "CONTINUE";
    rs << cmd;
    d->logSendMessage(cmd);
    sendMessage(reply);
}

void QScriptDebuggerClient::interruptInferior()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "INTERRUPT";
    rs << cmd;
    d->logSendMessage(cmd);
    sendMessage(reply);
}

void QScriptDebuggerClient::startSession()
{
    //Flush buffered data
    flushSendBuffer();

    //Set all breakpoints
    BreakHandler *handler = d->engine->breakHandler();
    foreach (BreakpointModelId id, handler->engineBreakpointIds(d->engine)) {
        QTC_CHECK(handler->state(id) == BreakpointInsertProceeding);
        handler->notifyBreakpointInsertOk(id);
    }
}

void QScriptDebuggerClient::endSession()
{
}

void QScriptDebuggerClient::activateFrame(int index)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "ACTIVATE_FRAME";
    rs << cmd
       << index;
    d->logSendMessage(QString(_("%1 %2")).arg(cmd, QString::number(index)));
    sendMessage(reply);
}

void QScriptDebuggerClient::insertBreakpoint(const BreakpointModelId &id)
{
    BreakHandler *handler = d->engine->breakHandler();
    JSAgentBreakpointData bp;
    bp.fileUrl = QUrl::fromLocalFile(handler->fileName(id)).toString().toUtf8();
    bp.lineNumber = handler->lineNumber(id);
    bp.functionName = handler->functionName(id).toUtf8();
    d->breakpoints.insert(bp);
}

void QScriptDebuggerClient::removeBreakpoint(const BreakpointModelId &id)
{
    BreakHandler *handler = d->engine->breakHandler();
    JSAgentBreakpointData bp;
    bp.fileUrl = QUrl::fromLocalFile(handler->fileName(id)).toString().toUtf8();
    bp.lineNumber = handler->lineNumber(id);
    bp.functionName = handler->functionName(id).toUtf8();
    d->breakpoints.remove(bp);
}

void QScriptDebuggerClient::changeBreakpoint(const BreakpointModelId &id)
{
    BreakHandler *handler = d->engine->breakHandler();
    if (handler->isEnabled(id)) {
        insertBreakpoint(id);
    } else {
        removeBreakpoint(id);
    }
    BreakpointResponse br = handler->response(id);
    br.enabled = handler->isEnabled(id);
    handler->setResponse(id, br);
}

void QScriptDebuggerClient::synchronizeBreakpoints()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "BREAKPOINTS";
    rs << cmd
       << d->breakpoints;

    QStringList logBreakpoints;
    foreach (const JSAgentBreakpointData &bp, d->breakpoints) {
         logBreakpoints << QString("[%1, %2, %3]").arg(bp.functionName,
                                                       bp.fileUrl,
                                                       QString::number(bp.lineNumber));
    }
    d->logSendMessage(QString(_("%1 (%2)")).arg(cmd, logBreakpoints.join(", ")));

    sendMessage(reply);
}

void QScriptDebuggerClient::assignValueInDebugger(const QByteArray expr, const quint64 &id,
                                                  const QString &property, const QString &value)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "SET_PROPERTY";
    rs << cmd;
    rs << expr << id << property << value;
    d->logSendMessage(QString(_("%1 %2 %3 %4 %5")).arg(cmd, expr, QString::number(id), property,
                                                    value));
    sendMessage(reply);
}

void QScriptDebuggerClient::updateWatchData(const WatchData &data)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXEC";
    rs << cmd;
    rs << data.iname << data.name;
    d->logSendMessage(QString(_("%1 %2 %3")).arg(cmd, data.iname, data.name));
    sendMessage(reply);
}

void QScriptDebuggerClient::executeDebuggerCommand(const QString &command)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXEC";
    QByteArray console = "console";
    rs << cmd << console << command;
    d->logSendMessage(QString(_("%1 %2 %3")).arg(cmd, console, command));
    sendMessage(reply);
}

void QScriptDebuggerClient::synchronizeWatchers(const QStringList &watchers)
{
    // send watchers list
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "WATCH_EXPRESSIONS";
    rs << cmd;
    d->logSendMessage(QString(_("%1 (%2)")).arg(cmd, watchers.join(", ")));
    sendMessage(reply);
}

void QScriptDebuggerClient::expandObject(const QByteArray &iname, quint64 objectId)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXPAND";
    rs << cmd;
    rs << iname << objectId;
    d->logSendMessage(QString(_("%1 %2 %3")).arg(cmd, iname, QString::number(objectId)));
    sendMessage(reply);
}

void QScriptDebuggerClient::sendPing()
{
    d->ping++;
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "PING";
    rs << cmd;
    rs << d->ping;
    d->logSendMessage(cmd);
    sendMessage(reply);
}

void QScriptDebuggerClient::messageReceived(const QByteArray &data)
{
    QByteArray rwData = data;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    QByteArray command;
    stream >> command;

    if (command == "STOPPED") {
        d->engine->inferiorSpontaneousStop();

        QString logString = QString::fromLatin1(command);

        JSAgentStackFrames stackFrames;
        QList<WatchData> watches;
        QList<WatchData> locals;
        stream >> stackFrames >> watches >> locals;

        logString += QString::fromLatin1(" (%1 stack frames) (%2 watches)  (%3 locals)").
                arg(stackFrames.size()).arg(watches.size()).arg(locals.size());

        StackFrames ideStackFrames;
        for (int i = 0; i != stackFrames.size(); ++i) {
            StackFrame frame;
            frame.line = stackFrames.at(i).lineNumber;
            frame.function = stackFrames.at(i).functionName;
            frame.file = d->engine->toFileInProject(QUrl(stackFrames.at(i).fileUrl));
            frame.usable = QFileInfo(frame.file).isReadable();
            frame.level = i + 1;
            ideStackFrames << frame;
        }

        if (ideStackFrames.size() && ideStackFrames.back().function == "<global>")
            ideStackFrames.takeLast();
        d->engine->stackHandler()->setFrames(ideStackFrames);

        d->engine->watchHandler()->beginCycle();
        bool needPing = false;

        foreach (WatchData data, watches) {
            data.iname = d->engine->watchHandler()->watcherName(data.exp);
            d->engine->watchHandler()->insertData(data);

            if (d->engine->watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname,data.id);
            }
        }

        foreach (WatchData data, locals) {
            data.iname = "local." + data.exp;
            d->engine->watchHandler()->insertData(data);

            if (d->engine->watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname,data.id);
            }
        }

        if (needPing) {
            sendPing();
        } else {
            d->engine->watchHandler()->endCycle();
        }

        bool becauseOfException;
        stream >> becauseOfException;

        logString += becauseOfException ? " exception" : " no_exception";

        if (becauseOfException) {
            QString error;
            stream >> error;

            logString += QLatin1Char(' ');
            logString += error;
            d->logReceiveMessage(logString);

            QString msg = stackFrames.isEmpty()
                    ? tr("<p>An uncaught exception occurred:</p><p>%1</p>")
                      .arg(Qt::escape(error))
                    : tr("<p>An uncaught exception occurred in <i>%1</i>:</p><p>%2</p>")
                      .arg(stackFrames.value(0).fileUrl, Qt::escape(error));
            showMessageBox(QMessageBox::Information, tr("Uncaught Exception"), msg);
        } else {
            //
            // Make breakpoint non-pending
            //
            QString file;
            QString function;
            int line = -1;

            if (!ideStackFrames.isEmpty()) {
                file = ideStackFrames.at(0).file;
                line = ideStackFrames.at(0).line;
                function = ideStackFrames.at(0).function;
            }

            BreakHandler *handler = d->engine->breakHandler();
            foreach (BreakpointModelId id, handler->engineBreakpointIds(d->engine)) {
                QString processedFilename = handler->fileName(id);

                if (processedFilename == file && handler->lineNumber(id) == line) {
                    if (handler->state(id) == BreakpointInsertProceeding)
                        handler->notifyBreakpointInsertOk(id);
                    QTC_CHECK(handler->state(id) == BreakpointInserted);
                    BreakpointResponse br = handler->response(id);
                    br.fileName = file;
                    br.lineNumber = line;
                    br.functionName = function;
                    handler->setResponse(id, br);
                }
            }

            d->logReceiveMessage(logString);
        }

        if (!ideStackFrames.isEmpty())
            d->engine->gotoLocation(ideStackFrames.value(0));

    } else if (command == "RESULT") {
        WatchData data;
        QByteArray iname;
        stream >> iname >> data;

        d->logReceiveMessage(QString("%1 %2 %3").arg(QString(command), QString(iname),
                                                     QString(data.value)));
        data.iname = iname;
        if (iname.startsWith("watch.")) {
            d->engine->watchHandler()->insertData(data);
        } else if (iname == "console") {
            d->engine->showMessage(data.value, ScriptConsoleOutput);
        } else {
            qWarning() << "QmlEngine: Unexcpected result: " << iname << data.value;
        }
    } else if (command == "EXPANDED") {
        QList<WatchData> result;
        QByteArray iname;
        stream >> iname >> result;
        d->logReceiveMessage(QString("%1 %2 (%3 x watchdata)").arg( QString(command),
                                                                    QString(iname),
                                                                    QString::number(result.size())));
        bool needPing = false;

        foreach (WatchData data, result) {
            data.iname = iname + '.' + data.exp;
            d->engine->watchHandler()->insertData(data);

            if (d->engine->watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.id);
            }
        }
        if (needPing)
            sendPing();
    } else if (command == "LOCALS") {
        QList<WatchData> locals;
        QList<WatchData> watches;
        int frameId;
        stream >> frameId >> locals;
        if (!stream.atEnd()) { // compatibility with jsdebuggeragent from 2.1, 2.2
            stream >> watches;
        }

        d->logReceiveMessage(QString("%1 %2 (%3 x locals) (%4 x watchdata)").arg(
                                  QString(command), QString::number(frameId),
                                  QString::number(locals.size()),
                                  QString::number(watches.size())));
        d->engine->watchHandler()->beginCycle();
        bool needPing = false;
        foreach (WatchData data, watches) {
            data.iname = d->engine->watchHandler()->watcherName(data.exp);
            d->engine->watchHandler()->insertData(data);

            if (d->engine->watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.id);
            }
        }

        foreach (WatchData data, locals) {
            data.iname = "local." + data.exp;
            d->engine->watchHandler()->insertData(data);
            if (d->engine->watchHandler()->expandedINames().contains(data.iname)) {
                needPing = true;
                expandObject(data.iname, data.id);
            }
        }
        if (needPing)
            sendPing();
        else
            d->engine->watchHandler()->endCycle();

    } else if (command == "PONG") {
        int ping;
        stream >> ping;

        d->logReceiveMessage(QString("%1 %2").arg(QString(command), QString::number(ping)));

        if (ping == d->ping)
            d->engine->watchHandler()->endCycle();
    } else {
        qDebug() << Q_FUNC_INFO << "Unknown command: " << command;
        d->logReceiveMessage(QString("%1 UNKNOWN COMMAND!!").arg(QString(command)));
    }

}

void QScriptDebuggerClient::setEngine(QmlEngine *engine)
{
    d->engine = engine;
}

void QScriptDebuggerClientPrivate::logSendMessage(const QString &msg) const
{
    if (engine)
        engine->logMessage("QScriptDebuggerClient", QmlEngine::LogSend, msg);
}

void QScriptDebuggerClientPrivate::logReceiveMessage(const QString &msg) const
{
    if (engine)
        engine->logMessage("QScriptDebuggerClient", QmlEngine::LogReceive, msg);
}

} // Internal
} // Debugger
