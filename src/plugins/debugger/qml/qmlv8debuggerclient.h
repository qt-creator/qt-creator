/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLV8DEBUGGERCLIENT_H
#define QMLV8DEBUGGERCLIENT_H

#include "baseqmldebuggerclient.h"

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
    void resetSession();

    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();

    void executeRunToLine(const ContextData &data);

    void continueInferior();
    void interruptInferior();

    void activateFrame(int index);

    bool acceptsBreakpoint(Breakpoint bp);
    void insertBreakpoint(Breakpoint bp, int adjustedLine,
                          int adjustedColumn = -1);
    void removeBreakpoint(Breakpoint bp);
    void changeBreakpoint(Breakpoint bp);
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
    void expandLocalsAndWatchers(const QVariant &bodyVal, const QVariant &refsVal);
    void createWatchDataList(const WatchItem *parent,
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
