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

#ifndef QSCRIPTDEBUGGERCLIENT_H
#define QSCRIPTDEBUGGERCLIENT_H

#include "baseqmldebuggerclient.h"
#include "stackframe.h"
#include "watchdata.h"
#include "qmlengine.h"

namespace Debugger {
namespace Internal {

class QScriptDebuggerClientPrivate;

class QScriptDebuggerClient : public BaseQmlDebuggerClient
{
    Q_OBJECT

public:
    QScriptDebuggerClient(QmlDebug::QmlDebugConnection *client);
    ~QScriptDebuggerClient();

    void startSession();
    void endSession();

    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();

    void executeRunToLine(const ContextData &data);

    void continueInferior();
    void interruptInferior();

    void activateFrame(int index);

    void insertBreakpoint(const BreakpointModelId &id, int adjustedLine,
                          int adjustedColumn = -1);
    void removeBreakpoint(const BreakpointModelId &id);
    void changeBreakpoint(const BreakpointModelId &id);
    void synchronizeBreakpoints();

    void assignValueInDebugger(const WatchData *data, const QString &expression,
                                       const QVariant &valueV);

    void updateWatchData(const WatchData &data);
    void executeDebuggerCommand(const QString &command);

    void synchronizeWatchers(const QStringList &watchers);

    void expandObject(const QByteArray &iname, quint64 objectId);

    void setEngine(QmlEngine *engine);

protected:
    void messageReceived(const QByteArray &data);

private:
    void sendPing();
    void insertLocalsAndWatches(QList<WatchData> &locals, QList<WatchData> &watches,
                                int stackFrameIndex);

private:
    QScriptDebuggerClientPrivate *d;
    friend class QScriptDebuggerClientPrivate;
};

} // Internal
} // Debugger

#endif // QSCRIPTDEBUGGERCLIENT_H
