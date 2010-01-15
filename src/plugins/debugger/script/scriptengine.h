/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_SCRIPTENGINE_H
#define DEBUGGER_SCRIPTENGINE_H

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QPoint>
#include <QtCore/QSet>
#include <QtCore/QVariant>
#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE
class QAction;
class QAbstractItemModel;
class QSplitter;
class QScriptEngine;
class QScriptValue;
QT_END_NAMESPACE

#include "idebuggerengine.h"

namespace Debugger {
namespace Internal {

class ScriptAgent;
class WatchData;

class ScriptEngine : public IDebuggerEngine
{
    Q_OBJECT

public:
    ScriptEngine(DebuggerManager *manager);
    ~ScriptEngine();

private:
    // IDebuggerEngine implementation
    void stepExec();
    void stepOutExec();
    void nextExec();
    void stepIExec();
    void nextIExec();

    void shutdown();
    void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);
    void startDebugger(const DebuggerStartParametersPtr &sp);

    void exitDebugger();

    void continueInferior();
    Q_SLOT void runInferior();
    void interruptInferior();

    void runToLineExec(const QString &fileName, int lineNumber);
    void runToFunctionExec(const QString &functionName);
    void jumpToLineExec(const QString &fileName, int lineNumber);

    void activateFrame(int index);
    void selectThread(int index);

    void attemptBreakpointSynchronization();

    void assignValueInDebugger(const QString &expr, const QString &value);
    void executeDebuggerCommand(const QString & command);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    virtual QList<Symbol> moduleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles() {}
    void reloadFullStack() {}

    bool supportsThreads() const { return true; }
    bool checkForBreakCondition(bool byFunction);
    void updateWatchData(const WatchData &data);
    void updateLocals();
    void updateSubItem(const WatchData &data);

    Q_SLOT void showDebuggerOutput(int channel, const QString &m);

private:
    friend class ScriptAgent;

    bool importExtensions();

    QSharedPointer<QScriptEngine> m_scriptEngine;
    QString m_scriptContents;
    QString m_scriptFileName;
    QScopedPointer<ScriptAgent> m_scriptAgent;

    bool m_stopped;
    bool m_stopOnNextLine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_SCRIPTENGINE_H
