/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef DEBUGGER_IDEBUGGERENGINE_H
#define DEBUGGER_IDEBUGGERENGINE_H

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QPoint;
class QString;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class IDebuggerEngine : public QObject
{
public:
    IDebuggerEngine(QObject *parent = 0) : QObject(parent) {}

    virtual void shutdown() = 0;
    virtual void setToolTipExpression(const QPoint &pos, const QString &exp) = 0;
    virtual bool startDebugger() = 0;
    virtual void exitDebugger() = 0;
    virtual void updateWatchModel() = 0;

    virtual void stepExec() = 0;
    virtual void stepOutExec() = 0;
    virtual void nextExec() = 0;
    virtual void stepIExec() = 0;
    virtual void nextIExec() = 0;
    
    virtual void continueInferior() = 0;
    virtual void runInferior() = 0;
    virtual void interruptInferior() = 0;

    virtual void runToLineExec(const QString &fileName, int lineNumber) = 0;
    virtual void runToFunctionExec(const QString &functionName) = 0;
    virtual void jumpToLineExec(const QString &fileName, int lineNumber) = 0;
    virtual void assignValueInDebugger(const QString &expr, const QString &value) = 0;
    virtual void executeDebuggerCommand(const QString &command) = 0;

    virtual void activateFrame(int index) = 0;
    virtual void selectThread(int index) = 0;

    virtual void attemptBreakpointSynchronization() = 0;

    virtual void loadSessionData() = 0;
    virtual void saveSessionData() = 0;

    virtual void reloadDisassembler() = 0;

    virtual void reloadModules() = 0;
    virtual void loadSymbols(const QString &moduleName) = 0;
    virtual void loadAllSymbols() = 0;

    virtual void reloadRegisters() = 0;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_IDEBUGGERENGINE_H
