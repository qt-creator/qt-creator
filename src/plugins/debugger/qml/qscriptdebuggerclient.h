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

#ifndef QSCRIPTDEBUGGERCLIENT_H
#define QSCRIPTDEBUGGERCLIENT_H

#include "qmldebuggerclient.h"
#include "stackframe.h"
#include "watchdata.h"
#include "qmlengine.h"

namespace Debugger {
namespace Internal {

class QScriptDebuggerClientPrivate;

class QScriptDebuggerClient : public QmlDebuggerClient
{
    Q_OBJECT

public:
    QScriptDebuggerClient(QmlJsDebugClient::QDeclarativeDebugConnection *client);
    ~QScriptDebuggerClient();

    void startSession();
    void endSession();

    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();

    void continueInferior();
    void interruptInferior();

    void activateFrame(int index);

    void insertBreakpoint(const BreakpointModelId &id);
    void removeBreakpoint(const BreakpointModelId &id);
    void changeBreakpoint(const BreakpointModelId &id);
    void updateBreakpoints();

    void assignValueInDebugger(const QByteArray expr, const quint64 &id,
                                       const QString &property, const QString &value);

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
    void sendPing();

private:
    QScriptDebuggerClientPrivate *d;
    friend class QScriptDebuggerClientPrivate;
};

} // Internal
} // Debugger

#endif // QSCRIPTDEBUGGERCLIENT_H
