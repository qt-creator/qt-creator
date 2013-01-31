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
#include <QTextStream>
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
        ping(0), sessionStarted(false), engine(0)
    {

    }

    int ping;
    bool sessionStarted;
    QmlEngine *engine;
    JSAgentBreakpoints breakpoints;

    void logSendMessage(const QString &msg) const;
    void logReceiveMessage(const QString &msg) const;
};

QScriptDebuggerClient::QScriptDebuggerClient(QmlDebug::QmlDebugConnection* client)
    : BaseQmlDebuggerClient(client, QLatin1String("JSDebugger")),
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
    d->logSendMessage(QLatin1String(cmd));
    sendMessage(reply);
}

void QScriptDebuggerClient::executeStepOut()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPOUT";
    rs << cmd;
    d->logSendMessage(QLatin1String(cmd));
    sendMessage(reply);
}

void QScriptDebuggerClient::executeNext()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPOVER";
    rs << cmd;
    d->logSendMessage(QLatin1String(cmd));
    sendMessage(reply);
}

void QScriptDebuggerClient::executeStepI()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "STEPINTO";
    rs << cmd;
    d->logSendMessage(QLatin1String(cmd));
    sendMessage(reply);
}

void QScriptDebuggerClient::executeRunToLine(const ContextData &data)
{
    JSAgentBreakpointData bp;
    bp.fileUrl = QUrl::fromLocalFile(data.fileName).toString().toUtf8();
    bp.lineNumber = data.lineNumber;
    bp.functionName = "TEMPORARY";
    d->breakpoints.insert(bp);
    synchronizeBreakpoints();
    continueInferior();
}

void QScriptDebuggerClient::continueInferior()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "CONTINUE";
    rs << cmd;
    d->logSendMessage(QLatin1String(cmd));
    sendMessage(reply);
}

void QScriptDebuggerClient::interruptInferior()
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "INTERRUPT";
    rs << cmd;
    d->logSendMessage(QLatin1String(cmd));
    sendMessage(reply);
}

void QScriptDebuggerClient::startSession()
{
    //Flush buffered data
    flushSendBuffer();

    //Set all breakpoints
    BreakHandler *handler = d->engine->breakHandler();
    DebuggerEngine * engine = d->engine->isSlaveEngine() ?
                d->engine->masterEngine() : d->engine;
    foreach (BreakpointModelId id, handler->engineBreakpointIds(engine)) {
        QTC_CHECK(handler->state(id) == BreakpointInsertProceeding);
        handler->notifyBreakpointInsertOk(id);
    }
    d->sessionStarted = true;
}

void QScriptDebuggerClient::endSession()
{
    d->sessionStarted = false;
}

void QScriptDebuggerClient::activateFrame(int index)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "ACTIVATE_FRAME";
    rs << cmd
       << index;
    d->logSendMessage(QLatin1String(cmd) + QLatin1Char(' ') + QString::number(index));
    sendMessage(reply);
}

void QScriptDebuggerClient::insertBreakpoint(const BreakpointModelId &id,
                                             int adjustedLine,
                                             int /*adjustedColumn*/)
{
    BreakHandler *handler = d->engine->breakHandler();
    JSAgentBreakpointData bp;
    bp.fileUrl = QUrl::fromLocalFile(handler->fileName(id)).toString().toUtf8();
    bp.lineNumber = adjustedLine;
    bp.functionName = handler->functionName(id).toUtf8();
    d->breakpoints.insert(bp);

    BreakpointResponse br = handler->response(id);
    br.lineNumber = adjustedLine;
    handler->setResponse(id, br);
    if (d->sessionStarted && handler->state(id) == BreakpointInsertProceeding)
        handler->notifyBreakpointInsertOk(id);
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
        BreakpointResponse br = handler->response(id);
        insertBreakpoint(id, br.lineNumber);
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

    QString logBreakpoints;
    QTextStream str(&logBreakpoints);
    str << cmd << " (";
    bool first = true;
    foreach (const JSAgentBreakpointData &bp, d->breakpoints) {
        if (first)
            first = false;
        else
            str << ", ";
        str << '[' << bp.functionName << ", " << bp.fileUrl << ", " << bp.lineNumber << ']';
    }
    str << ')';
    d->logSendMessage(logBreakpoints);

    sendMessage(reply);
}

void QScriptDebuggerClient::assignValueInDebugger(const WatchData *data,
                                                  const QString &expr,
                                                  const QVariant &valueV)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXEC";
    rs << cmd;
    QString expression = QString(_("%1 = %2;")).arg(expr).arg(valueV.toString());
    rs << data->iname << expression;
    d->logSendMessage(QString::fromLatin1("%1 %2 %3 %4").
                      arg(QLatin1String(cmd), QLatin1String(data->iname), expr,
                          valueV.toString()));
    sendMessage(reply);
}

void QScriptDebuggerClient::updateWatchData(const WatchData &data)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXEC";
    rs << cmd;
    rs << data.iname << data.name;
    d->logSendMessage(QLatin1String(cmd) + QLatin1Char(' ') + QLatin1String(data.iname)
                      + QLatin1Char(' ') + data.name);
    sendMessage(reply);
}

void QScriptDebuggerClient::executeDebuggerCommand(const QString &command)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXEC";
    QByteArray console = "console";
    rs << cmd << console << command;
    d->logSendMessage(QLatin1String(cmd) + QLatin1Char(' ') + QLatin1String(console)
                      + QLatin1Char(' ') +  command);
    sendMessage(reply);
}

void QScriptDebuggerClient::synchronizeWatchers(const QStringList &watchers)
{
    // send watchers list
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "WATCH_EXPRESSIONS";
    rs << cmd;
    d->logSendMessage(QString::fromLatin1("%1 (%2)").arg(QLatin1String(cmd),
                                                         watchers.join(QLatin1String(", "))));
    sendMessage(reply);
}

void QScriptDebuggerClient::expandObject(const QByteArray &iname, quint64 objectId)
{
    //Check if id is valid
    if (qint64(objectId) == -1)
        return;

    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd = "EXPAND";
    rs << cmd;
    rs << iname << objectId;
    d->logSendMessage(QLatin1String(cmd) + QLatin1Char(' ') + QLatin1String(iname)
                      + QString::number(objectId));
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
    d->logSendMessage(QLatin1String(cmd));
    sendMessage(reply);
}

void QScriptDebuggerClient::messageReceived(const QByteArray &data)
{
    QByteArray rwData = data;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    QByteArray command;
    stream >> command;

    WatchHandler *watchHandler = d->engine->watchHandler();
    StackHandler *stackHandler = d->engine->stackHandler();

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
            frame.function = QLatin1String(stackFrames.at(i).functionName);
            frame.file = d->engine->toFileInProject(QUrl(QLatin1String(stackFrames.at(i).fileUrl)));
            frame.usable = QFileInfo(frame.file).isReadable();
            frame.level = i + 1;
            ideStackFrames << frame;
        }

        stackHandler->setFrames(ideStackFrames);

        bool becauseOfException;
        stream >> becauseOfException;

        logString += becauseOfException ? QLatin1String(" exception") : QLatin1String(" no_exception");

        if (becauseOfException) {
            QString error;
            stream >> error;

            logString += QLatin1Char(' ');
            logString += error;
            d->logReceiveMessage(logString);

            QString msg = stackFrames.isEmpty()
                    ? tr("<p>An uncaught exception occurred:</p><p>%1</p>")
                      .arg(Qt::escape(error))
                    : tr("<p>An uncaught exception occurred in '%1':</p><p>%2</p>")
                      .arg(QLatin1String(stackFrames.value(0).fileUrl), Qt::escape(error));
            showMessageBox(QMessageBox::Information, tr("Uncaught Exception"), msg);
        } else {
            QString file;
            int line = -1;

            if (!ideStackFrames.isEmpty()) {
                file = ideStackFrames.at(0).file;
                line = ideStackFrames.at(0).line;
            }

            QList<JSAgentBreakpointData> breakpoints(d->breakpoints.toList());
            foreach (const JSAgentBreakpointData &data, breakpoints) {
                if (data.fileUrl == QUrl::fromLocalFile(file).toString().toUtf8() &&
                        data.lineNumber == line &&
                        data.functionName == "TEMPORARY") {
                    breakpoints.removeOne(data);
                    d->breakpoints = JSAgentBreakpoints::fromList(breakpoints);
                    synchronizeBreakpoints();
                    break;
                }
            }

            d->logReceiveMessage(logString);
        }

        if (!ideStackFrames.isEmpty())
            d->engine->gotoLocation(ideStackFrames.value(0));

        insertLocalsAndWatches(locals, watches, stackHandler->currentIndex());

    } else if (command == "RESULT") {
        WatchData data;
        QByteArray iname;
        stream >> iname >> data;

        d->logReceiveMessage(QLatin1String(command) + QLatin1Char(' ')
                             +  QLatin1String(iname) + QLatin1Char(' ') + data.value);
        data.iname = iname;
        if (iname.startsWith("watch.")) {
            watchHandler->insertData(data);
        } else if (iname == "console") {
            d->engine->showMessage(data.value, ConsoleOutput);
        } else if (iname.startsWith("local.")) {
            data.name = data.name.left(data.name.indexOf(QLatin1Char(' ')));
            watchHandler->insertData(data);
        } else {
            qWarning() << "QmlEngine: Unexcpected result: " << iname << data.value;
        }
    } else if (command == "EXPANDED") {
        QList<WatchData> result;
        QByteArray iname;
        stream >> iname >> result;
        d->logReceiveMessage(QString::fromLatin1("%1 %2 (%3 x watchdata)").
                             arg(QLatin1String(command), QLatin1String(iname),
                                 QString::number(result.size())));
        bool needPing = false;

        foreach (WatchData data, result) {
            data.iname = iname + '.' + data.exp;
            watchHandler->insertData(data);

            if (watchHandler->isExpandedIName(data.iname) && qint64(data.id) != -1) {
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

        d->logReceiveMessage(QString::fromLatin1("%1 %2 (%3 x locals) (%4 x watchdata)").arg(
                             QLatin1String(command), QString::number(frameId),
                             QString::number(locals.size()), QString::number(watches.size())));

        insertLocalsAndWatches(locals, watches, frameId);

    } else if (command == "PONG") {
        int ping;
        stream >> ping;
        d->logReceiveMessage(QLatin1String(command) + QLatin1Char(' ') + QString::number(ping));
    } else {
        qDebug() << Q_FUNC_INFO << "Unknown command: " << command;
        d->logReceiveMessage(QLatin1String(command) + QLatin1String(" UNKNOWN COMMAND!!"));
    }

}

void QScriptDebuggerClient::insertLocalsAndWatches(QList<WatchData> &locals,
                                                   QList<WatchData> &watches,
                                                   int stackFrameIndex)
{
    WatchHandler *watchHandler = d->engine->watchHandler();
    watchHandler->removeAllData();
    if (stackFrameIndex < 0)
        return;
    const StackFrame frame = d->engine->stackHandler()->frameAt(stackFrameIndex);
    if (!frame.isUsable())
        return;

    bool needPing = false;
    foreach (WatchData data, watches) {
        data.iname = watchHandler->watcherName(data.exp);
        watchHandler->insertData(data);

        if (watchHandler->isExpandedIName(data.iname) && qint64(data.id) != -1) {
            needPing = true;
            expandObject(data.iname, data.id);
        }
    }

    foreach (WatchData data, locals) {
        if (data.name == QLatin1String("<no initialized data>"))
            data.name = tr("No Local Variables");
        data.iname = "local." + data.exp;
        watchHandler->insertData(data);

        if (watchHandler->isExpandedIName(data.iname) && qint64(data.id) != -1) {
            needPing = true;
            expandObject(data.iname, data.id);
        }
    }

    if (needPing)
        sendPing();
    emit stackFrameCompleted();
}

void QScriptDebuggerClient::setEngine(QmlEngine *engine)
{
    d->engine = engine;
    connect(this, SIGNAL(stackFrameCompleted()), engine, SIGNAL(stackFrameCompleted()));
}

void QScriptDebuggerClientPrivate::logSendMessage(const QString &msg) const
{
    if (engine)
        engine->logMessage(QLatin1String("QScriptDebuggerClient"), QmlEngine::LogSend, msg);
}

void QScriptDebuggerClientPrivate::logReceiveMessage(const QString &msg) const
{
    if (engine)
        engine->logMessage(QLatin1String("QScriptDebuggerClient"), QmlEngine::LogReceive, msg);
}

} // Internal
} // Debugger
