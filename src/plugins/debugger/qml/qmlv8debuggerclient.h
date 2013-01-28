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

#ifndef QMLV8DEBUGGERCLIENT_H
#define QMLV8DEBUGGERCLIENT_H

#include "baseqmldebuggerclient.h"
#include "stackframe.h"
#include "watchdata.h"

namespace Debugger {
namespace Internal {

class QmlV8DebuggerClientPrivate;

class QmlV8DebuggerClient : public BaseQmlDebuggerClient
{
    Q_OBJECT

    enum Exceptions
    {
        NoExceptions,
        UncaughtExceptions,
        AllExceptions
    };

    enum StepAction
    {
        Continue,
        In,
        Out,
        Next
    };

public:
    explicit QmlV8DebuggerClient(QmlDebug::QmlDebugConnection *client);
    ~QmlV8DebuggerClient();

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

    bool acceptsBreakpoint(const BreakpointModelId &id);
    void insertBreakpoint(const BreakpointModelId &id, int adjustedLine,
                          int adjustedColumn = -1);
    void removeBreakpoint(const BreakpointModelId &id);
    void changeBreakpoint(const BreakpointModelId &id);
    void synchronizeBreakpoints();

    void assignValueInDebugger(const WatchData *data,
                               const QString &expression,
                               const QVariant &valueV);

    void updateWatchData(const WatchData &);
    void executeDebuggerCommand(const QString &command);

    void synchronizeWatchers(const QStringList &watchers);

    void expandObject(const QByteArray &iname, quint64 objectId);

    void setEngine(QmlEngine *engine);

    void getSourceFiles();

protected:
    void messageReceived(const QByteArray &data);

private:
    void updateStack(const QVariant &bodyVal, const QVariant &refsVal);
    StackFrame extractStackFrame(const QVariant &bodyVal, const QVariant &refsVal);
    void setCurrentFrameDetails(const QVariant &bodyVal, const QVariant &refsVal);
    void updateScope(const QVariant &bodyVal, const QVariant &refsVal);

    void updateEvaluationResult(int sequence, bool success, const QVariant &bodyVal,
                                const QVariant &refsVal);
    void updateBreakpoints(const QVariant &bodyVal);

    void expandLocalsAndWatchers(const QVariant &bodyVal, const QVariant &refsVal);
    QList<WatchData> createWatchDataList(const WatchData *parent,
                                         const QVariantList &properties,
                                         const QVariant &refsVal);

    void highlightExceptionCode(int lineNumber, const QString &filePath,
                                const QString &errorMessage);
    void clearExceptionSelection();

private:
    QmlV8DebuggerClientPrivate *d;
    friend class QmlV8DebuggerClientPrivate;
};

} // Internal
} // Debugger

#endif // QMLV8DEBUGGERCLIENT_H
