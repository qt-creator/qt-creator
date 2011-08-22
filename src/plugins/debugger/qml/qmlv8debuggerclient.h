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

#ifndef QMLV8DEBUGGERCLIENT_H
#define QMLV8DEBUGGERCLIENT_H

#include "qmldebuggerclient.h"
#include "stackframe.h"
#include "watchdata.h"
#include "qmlengine.h"
#include "json.h"

namespace Debugger {
namespace Internal {

class QmlV8DebuggerClientPrivate;

class QmlV8DebuggerClient : public QmlDebuggerClient
{
    Q_OBJECT

public:
    explicit QmlV8DebuggerClient(QmlJsDebugClient::QDeclarativeDebugConnection *client);
    ~QmlV8DebuggerClient();

    void connect();
    void disconnect();

    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();

    void continueInferior();
    void interruptInferior();

    void activateFrame(int index);

    void insertBreakpoint(BreakpointModelId id, BreakHandler *handler);
    void removeBreakpoint(BreakpointModelId id, BreakHandler *handler);
    void changeBreakpoint(BreakpointModelId id, BreakHandler *handler);
    void updateBreakpoints();

    void assignValueInDebugger(const QByteArray expr, const quint64 &id,
                                       const QString &property, const QString value);

    void updateWatchData(const WatchData *data);
    void executeDebuggerCommand(const QString &command);

    void synchronizeWatchers(const QStringList &watchers);

    void expandObject(const QByteArray &iname, quint64 objectId);

    void setEngine(QmlEngine *engine);

signals:
    void notifyDebuggerStopped();

protected:
    void messageReceived(const QByteArray &data);

private:
    void backtrace();
    void setStackFrames(QByteArray &);
    void setLocals(int frameIndex);
    void setExpression(QByteArray &message);
    void expandLocal(QByteArray &message);
    void setPropertyValue(Json::JsonValue &refs, Json::JsonValue &property, QByteArray &prepend);
    int indexInRef(const Json::JsonValue &refs, int refIndex);
    QByteArray packMessage(QByteArray& message);

private:
    QmlV8DebuggerClientPrivate *d;
    friend class QmlV8DebuggerClientPrivate;
};

} // Internal
} // Debugger

#endif // QMLV8DEBUGGERCLIENT_H
