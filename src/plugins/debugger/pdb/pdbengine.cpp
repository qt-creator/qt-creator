/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#define QT_NO_CAST_FROM_ASCII

#include "pdbengine.h"

#include "debuggeractions.h"
#include "debuggerdialogs.h"
#include "breakhandler.h"
#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "debuggerstringutils.h"
#include "../gdb/gdbmi.h"

#include <utils/qtcassert.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/ifile.h>
//#include <coreplugin/scriptmanager/scriptmanager.h>
#include <coreplugin/icore.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

#include <QtGui/QApplication>
#include <QtGui/QToolTip>
#include <QtGui/QMessageBox>


#define DEBUG_SCRIPT 1
#if DEBUG_SCRIPT
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s


#define CB(callback) &PdbEngine::callback, STRINGIFY(callback)

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PdbEngine
//
///////////////////////////////////////////////////////////////////////

PdbEngine::PdbEngine(DebuggerManager *manager)
    : IDebuggerEngine(manager)
{}

PdbEngine::~PdbEngine()
{}

void PdbEngine::executeDebuggerCommand(const QString &command)
{
    XSDEBUG("PdbEngine::executeDebuggerCommand:" << command);
    if (state() == DebuggerNotReady) {
        debugMessage(_("PDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + command);
        return;
    }
    m_pdbProc.write(command.toLatin1() + "\n");
}

void PdbEngine::postCommand(const QByteArray &command,
//                 PdbCommandFlags flags,
                 PdbCommandCallback callback,
                 const char *callbackName,
                 const QVariant &cookie)
{
    PdbCommand cmd;
    cmd.command = command;
    cmd.callback = callback;
    cmd.callbackName = callbackName;
    cmd.cookie = cookie;
    m_commands.enqueue(cmd);
    showDebuggerInput(LogMisc, _(cmd.command));
    m_pdbProc.write(cmd.command + "\n");
}

void PdbEngine::shutdown()
{
    exitDebugger();
}

void PdbEngine::exitDebugger()
{
    if (state() == DebuggerNotReady)
        return;
    SDEBUG("PdbEngine::exitDebugger()");
    m_pdbProc.kill();
    //if (m_scriptEngine->isEvaluating())
    //    m_scriptEngine->abortEvaluation();
    setState(InferiorShuttingDown);
    setState(InferiorShutDown);
    setState(EngineShuttingDown);
    //m_scriptEngine->setAgent(0);
    setState(DebuggerNotReady);
}

void PdbEngine::startDebugger(const DebuggerStartParametersPtr &sp)
{
    setState(AdapterStarting);

    m_scriptFileName = QFileInfo(sp->executable).absoluteFilePath();
    QFile scriptFile(m_scriptFileName);
    if (!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        //debugMessage("STARTING " +m_scriptFileName + "FAILED");
        manager()->showDebuggerOutput(LogError, QString::fromLatin1("Cannot open %1: %2").
                         arg(m_scriptFileName, scriptFile.errorString()));
        emit startFailed();
        return;
    }
    setState(AdapterStarted);
    setState(InferiorStarting);
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Running requested..."), 5000);

    m_pdbProc.disconnect(); // From any previous runs

    m_pdb = _("/usr/bin/python");
    debugMessage(_("STARTING PDB ") + m_pdb);
    QStringList gdbArgs;
    gdbArgs += _("-i");
    gdbArgs += _("/usr/bin/pdb");
    gdbArgs += m_scriptFileName;
    //gdbArgs += args;

    connect(&m_pdbProc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handlePdbError(QProcess::ProcessError)));
    connect(&m_pdbProc, SIGNAL(finished(int, QProcess::ExitStatus)),
        SLOT(handlePdbFinished(int, QProcess::ExitStatus)));
    connect(&m_pdbProc, SIGNAL(readyReadStandardOutput()),
        SLOT(readPdbStandardOutput()));
    connect(&m_pdbProc, SIGNAL(readyReadStandardError()),
        SLOT(readPdbStandardError()));

    // We will stop immediatly, so setup a proper callback.
    PdbCommand cmd;
    cmd.callback = &PdbEngine::handleUpdateAll;
    m_commands.enqueue(cmd);

    m_pdbProc.start(m_pdb, gdbArgs);
    //qDebug() << "STARTING:" << m_pdb << gdbArgs;

    if (!m_pdbProc.waitForStarted()) {
        const QString msg = tr("Unable to start pdb '%1': %2")
            .arg(m_pdb, m_pdbProc.errorString());
        setState(AdapterStartFailed);
        debugMessage(_("ADAPTER START FAILED"));
        if (!msg.isEmpty()) {
            const QString title = tr("Adapter start failed");
            Core::ICore::instance()->showWarningWithOptions(title, msg);
        }
        shutdown();
        emit startFailed();
        return;
    }

    emit startSuccessful();
    setState(InferiorRunning);
    attemptBreakpointSynchronization();

    debugMessage(_("PDB STARTED, INITIALIZING IT"));
    const QByteArray dumperSourcePath =
        Core::ICore::instance()->resourcePath().toLocal8Bit() + "/gdbmacros/";
    postCommand("execfile('" + dumperSourcePath + "pdumper.py')",
        CB(handleLoadDumper));
}

void PdbEngine::runInferior()
{
    SDEBUG("PdbEngine::runInferior()");
    // FIXME: setState(InferiorRunning);
}

void PdbEngine::interruptInferior()
{
    setState(InferiorStopped);
}

void PdbEngine::executeStep()
{
    m_manager->resetLocation();
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
    postCommand("step", CB(handleUpdateAll));
}

void PdbEngine::executeStepI()
{
    m_manager->resetLocation();
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
    postCommand("step", CB(handleUpdateAll));
}

void PdbEngine::executeStepOut()
{
    m_manager->resetLocation();
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
    postCommand("finish", CB(handleUpdateAll));
}

void PdbEngine::executeNext()
{
    m_manager->resetLocation();
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
    postCommand("next", CB(handleUpdateAll));
}

void PdbEngine::executeNextI()
{
    m_manager->resetLocation();
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
    postCommand("next", CB(handleUpdateAll));
}

void PdbEngine::continueInferior()
{
    m_manager->resetLocation();
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
    // Callback will be triggered e.g. when breakpoint is hit.
    postCommand("continue", CB(handleUpdateAll));
}

void PdbEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    SDEBUG("FIXME:  PdbEngine::runToLineExec()");
}

void PdbEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  PdbEngine::runToFunctionExec()");
}

void PdbEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    XSDEBUG("FIXME:  PdbEngine::jumpToLineExec()");
}

void PdbEngine::activateFrame(int frameIndex)
{
    manager()->resetLocation();
    if (state() != InferiorStopped && state() != InferiorUnrunnable)
        return;

    StackHandler *stackHandler = manager()->stackHandler();
    int oldIndex = stackHandler->currentIndex();

    //if (frameIndex == stackHandler->stackSize()) {
    //    reloadFullStack();
    //    return;
    //}

    QTC_ASSERT(frameIndex < stackHandler->stackSize(), return);

    if (oldIndex != frameIndex) {
        // Assuming the command always succeeds this saves a roundtrip.
        // Otherwise the lines below would need to get triggered
        // after a response to this -stack-select-frame here.
        stackHandler->setCurrentIndex(frameIndex);
        //postCommand("-stack-select-frame " + QByteArray::number(frameIndex),
        //    CB(handleStackSelectFrame));
    }
    manager()->gotoLocation(stackHandler->currentFrame(), true);
}

void PdbEngine::selectThread(int index)
{
    Q_UNUSED(index)
}

static QByteArray breakpointLocation(const BreakpointData *data)
{
    if (!data->funcName.isEmpty())
        return data->funcName.toLatin1();
    // In this case, data->funcName is something like '*0xdeadbeef'
    if (data->lineNumber.toInt() == 0)
        return data->funcName.toLatin1();
    //QString loc = data->useFullPath ? data->fileName : breakLocation(data->fileName);
    // The argument is simply a C-quoted version of the argument to the
    // non-MI "break" command, including the "original" quoting it wants.
    //return "\"\\\"" + GdbMi::escapeCString(data->fileName).toLocal8Bit() + "\\\":"
    //    + data->lineNumber + '"';
    return data->fileName.toLocal8Bit() + ":" + data->lineNumber;
}

void PdbEngine::attemptBreakpointSynchronization()
{
    BreakHandler *handler = manager()->breakHandler();
    //qDebug() << "ATTEMPT BP SYNC";
    bool updateNeeded = false;
    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        if (data->pending) {
            data->pending = false; // FIXME
            updateNeeded = true;
            QByteArray loc = breakpointLocation(data);
            postCommand("break " + loc, CB(handleBreakInsert), QVariant(index));
        }
/*
        if (data->bpNumber.isEmpty()) {
            data->bpNumber = QByteArray::number(index + 1);
            updateNeeded = true;
        }
        if (!data->fileName.isEmpty() && data->markerFileName().isEmpty()) {
            data->setMarkerFileName(data->fileName);
            data->setMarkerLineNumber(data->lineNumber.toInt());
            updateNeeded = true;
        }
*/
    }
    //if (updateNeeded)
    //    handler->updateMarkers();
}

void PdbEngine::handleBreakInsert(const PdbResponse &response)
{
    //qDebug() << "BP RESPONSE: " << response.data;
    // "Breakpoint 1 at /pdb/math.py:10"
    int index = response.cookie.toInt();
    BreakHandler *handler = manager()->breakHandler();
    BreakpointData *data = handler->at(index);
    QTC_ASSERT(data, return);
    QTC_ASSERT(response.data.startsWith("Breakpoint "), return);
    int pos1 = response.data.indexOf(" at ");
    QTC_ASSERT(pos1 != -1, return);
    QByteArray bpnr = response.data.mid(11, pos1 - 11);
    int pos2 = response.data.lastIndexOf(":");
    QByteArray file = response.data.mid(pos1 + 4, pos2 - pos1 - 4);
    QByteArray line = response.data.mid(pos2 + 1);
    data->bpNumber = bpnr;
    data->bpFileName = _(file);
    data->bpLineNumber = line;
    handler->updateMarkers();
}

void PdbEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void PdbEngine::loadAllSymbols()
{
}

void PdbEngine::reloadModules()
{
}

QList<Symbol> PdbEngine::moduleSymbols(const QString & /*moduleName*/)
{
    return QList<Symbol>();
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////


static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

void PdbEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, int cursorPos)
{
    Q_UNUSED(mousePos)
    Q_UNUSED(editor)
    Q_UNUSED(cursorPos)

    if (state() != InferiorStopped) {
        //SDEBUG("SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED");
        return;
    }
    // Check mime type and get expression (borrowing some C++ - functions)
    const QString javaPythonMimeType =
        QLatin1String("application/javascript");
    if (!editor->file() || editor->file()->mimeType() != javaPythonMimeType)
        return;

    int line;
    int column;
    QString exp = cppExpressionAt(editor, cursorPos, &line, &column);

/*
    if (m_toolTipCache.contains(exp)) {
        const WatchData & data = m_toolTipCache[exp];
        q->watchHandler()->removeChildren(data.iname);
        insertData(data);
        return;
    }
*/

    QToolTip::hideText();
    if (exp.isEmpty() || exp.startsWith(QLatin1Char('#')))  {
        QToolTip::hideText();
        return;
    }

    if (!hasLetterOrNumber(exp)) {
        QToolTip::showText(m_toolTipPos, tr("'%1' contains no identifier").arg(exp));
        return;
    }

    if (exp.startsWith(QLatin1Char('"')) && exp.endsWith(QLatin1Char('"'))) {
        QToolTip::showText(m_toolTipPos, tr("String literal %1").arg(exp));
        return;
    }

    if (exp.startsWith(QLatin1String("++")) || exp.startsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.endsWith(QLatin1String("++")) || exp.endsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.startsWith(QLatin1Char('<')) || exp.startsWith(QLatin1Char('[')))
        return;

    if (hasSideEffects(exp)) {
        QToolTip::showText(m_toolTipPos,
            tr("Cowardly refusing to evaluate expression '%1' "
               "with potential side effects").arg(exp));
        return;
    }

#if 0
    //if (m_manager->status() != InferiorStopped)
    //    return;

    // FIXME: 'exp' can contain illegal characters
    m_toolTip = WatchData();
    m_toolTip.exp = exp;
    m_toolTip.name = exp;
    m_toolTip.iname = tooltipIName;
    insertData(m_toolTip);
#endif
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void PdbEngine::assignValueInDebugger(const QString &expression,
    const QString &value)
{
    Q_UNUSED(expression);
    Q_UNUSED(value);
    SDEBUG("ASSIGNING: " << (expression + QLatin1Char('=') + value));
#if 0
    m_scriptEngine->evaluate(expression + QLatin1Char('=') + value);
    updateLocals();
#endif
}


void PdbEngine::updateWatchData(const WatchData &data)
{
    Q_UNUSED(data);
    updateAll();
}

void PdbEngine::handlePdbError(QProcess::ProcessError error)
{
    debugMessage(_("HANDLE PDB ERROR"));
    switch (error) {
    case QProcess::Crashed:
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        m_pdbProc.kill();
        setState(EngineShuttingDown, true);
        m_manager->showMessageBox(QMessageBox::Critical, tr("Pdb I/O Error"),
                       errorMessage(error));
        break;
    }
}

QString PdbEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The Pdb process failed to start. Either the "
                "invoked program '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(m_pdb);
        case QProcess::Crashed:
            return tr("The Pdb process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the Pdb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the Pdb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the Pdb process occurred. ");
    }
}

void PdbEngine::handlePdbFinished(int code, QProcess::ExitStatus type)
{
    debugMessage(_("PDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));
    //shutdown();
    //initializeVariables();
    setState(DebuggerNotReady, true);
}

void PdbEngine::readPdbStandardError()
{
    QByteArray err = m_pdbProc.readAllStandardError();
    qWarning() << "Unexpected pdb stderr:" << err;
    showDebuggerOutput(LogDebug, _("Unexpected pdb stderr: " + err));
}

void PdbEngine::readPdbStandardOutput()
{
    m_inbuffer.append(m_pdbProc.readAllStandardOutput());
    //qDebug() << "BUFFER FROM: '" << m_inbuffer << "'";
    int pos = 1;
    while ((pos = m_inbuffer.indexOf("(Pdb)")) != -1) {
        PdbResponse response;
        response.data = m_inbuffer.left(pos).trimmed();
        showDebuggerOutput(LogDebug, _(response.data));
        m_inbuffer = m_inbuffer.mid(pos + 6);
        QTC_ASSERT(!m_commands.isEmpty(),
            qDebug() << "RESPONSE: " << response.data; return)
        PdbCommand cmd = m_commands.dequeue();
        response.cookie = cmd.cookie;
        if (cmd.callback) {
            //qDebug() << "EXECUTING CALLBACK " << cmd.callbackName
            //    << " RESPONSE: " << response.data;
            (this->*cmd.callback)(response);
        } else {
            qDebug() << "NO CALLBACK FOR RESPONSE: " << response.data;
        }
    }
    //qDebug() << "BUFFER LEFT: '" << m_inbuffer << "'";
}

void PdbEngine::handleResponse(const QByteArray &response0)
{
    QByteArray response = response0;
    qDebug() << "RESPONSE: '" << response << "'";
    if (response.startsWith("--Call--")) {
        qDebug() << "SKIPPING '--Call--' MARKER";
        response = response.mid(9);
    }
    if (response.startsWith("--Return--")) {
        qDebug() << "SKIPPING '--Return--' MARKER";
        response = response.mid(11);
    }
    if (response.startsWith("> ")) {
        int pos1 = response.indexOf('(');
        int pos2 = response.indexOf(')', pos1);
        if (pos1 != -1 && pos2 != -1) {
            int lineNumber = response.mid(pos1 + 1, pos2 - pos1 - 1).toInt();
            QByteArray fileName = response.mid(2, pos1 - 2);
            qDebug() << " " << pos1 << pos2 << lineNumber << fileName 
                << response.mid(pos1 + 1, pos2 - pos1 - 1);
            StackFrame frame;
            frame.file = _(fileName);
            frame.line = lineNumber;
            if (frame.line > 0 && QFileInfo(frame.file).exists()) {
                manager()->gotoLocation(frame, true);
                setState(InferiorStopping);
                setState(InferiorStopped);
                return;
            }
        }
    }
    qDebug() << "COULD NOT PARSE RESPONSE: '" << response << "'";
}

void PdbEngine::handleUpdateAll(const PdbResponse &response)
{
    Q_UNUSED(response);
    updateAll();
}

void PdbEngine::updateAll()
{
    setState(InferiorStopping);
    setState(InferiorStopped);

    WatchHandler *handler = m_manager->watchHandler();

    QByteArray watchers;
    //if (!m_toolTipExpression.isEmpty())
    //    watchers += m_toolTipExpression.toLatin1()
    //        + '#' + tooltipINameForExpression(m_toolTipExpression.toLatin1());

    QHash<QByteArray, int> watcherNames = handler->watcherNames();
    QHashIterator<QByteArray, int> it(watcherNames);
    while (it.hasNext()) {
        it.next();
        if (!watchers.isEmpty())
            watchers += "##";
        if (it.key() == WatchHandler::watcherEditPlaceHolder().toLatin1())
            watchers += "<Edit>#watch." + QByteArray::number(it.value());
        else
            watchers += it.key() + "#watch." + QByteArray::number(it.value());
    }

    QByteArray options;
    if (theDebuggerBoolSetting(UseDebuggingHelpers))
        options += "fancy,";
    if (theDebuggerBoolSetting(AutoDerefPointers))
        options += "autoderef,";
    if (options.isEmpty())
        options += "defaults,";
    options.chop(1);

    postCommand("bt", CB(handleBacktrace));
    postCommand("qdebug('" + options + "','"
        + handler->expansionRequests() + "','"
        + handler->typeFormatRequests() + "','"
        + handler->individualFormatRequests() + "','"
        + watchers.toHex() + "')", CB(handleListLocals));
}

void PdbEngine::handleBacktrace(const PdbResponse &response)
{
    //qDebug() << " BACKTRACE: '" << response.data << "'";
    // "  /usr/lib/python2.6/bdb.py(368)run()"
    // "-> exec cmd in globals, locals"
    // "  <string>(1)<module>()"
    // "  /python/math.py(19)<module>()"
    // "-> main()"
    // "  /python/math.py(14)main()"
    // "-> print cube(3)"
    // "  /python/math.py(7)cube()"
    // "-> x = square(a)"
    // "> /python/math.py(2)square()"
    // "-> def square(a):"

    // Populate stack view.
    QList<StackFrame> stackFrames;
    int level = 0;
    int currentIndex = -1;
    foreach (const QByteArray &line, response.data.split('\n')) {
        //qDebug() << "  LINE: '" << line << "'";
        if (line.startsWith("> ") || line.startsWith("  ")) {
            int pos1 = line.indexOf('(');
            int pos2 = line.indexOf(')', pos1);
            if (pos1 != -1 && pos2 != -1) {
                int lineNumber = line.mid(pos1 + 1, pos2 - pos1 - 1).toInt();
                QByteArray fileName = line.mid(2, pos1 - 2);
                //qDebug() << " " << pos1 << pos2 << lineNumber << fileName 
                //    << line.mid(pos1 + 1, pos2 - pos1 - 1);
                StackFrame frame;
                frame.file = _(fileName);
                frame.line = lineNumber;
                frame.function = _(line.mid(pos2 + 1));
                if (frame.line > 0 && QFileInfo(frame.file).exists()) {
                    if (line.startsWith("> "))
                        currentIndex = level;
                    frame.level = level;
                    stackFrames.prepend(frame);
                    ++level;
                }
            }
        }
    }
    const int frameCount = stackFrames.size();
    for (int i = 0; i != frameCount; ++i) 
        stackFrames[i].level = frameCount - stackFrames[i].level - 1; 
    manager()->stackHandler()->setFrames(stackFrames);

    // Select current frame.
    if (currentIndex != -1) {
        currentIndex = frameCount - currentIndex - 1;
        manager()->stackHandler()->setCurrentIndex(currentIndex);
        manager()->gotoLocation(stackFrames.at(currentIndex), true);
    }
}

void PdbEngine::handleListLocals(const PdbResponse &response)
{
    //qDebug() << " LOCALS: '" << response.data << "'";
    QByteArray out = response.data.trimmed();

    GdbMi all;
    all.fromStringMultiple(out);
    //qDebug() << "ALL: " << all.toString();

    //GdbMi data = all.findChild("data");
    QList<WatchData> list;
    WatchHandler *watchHandler = manager()->watchHandler();
    foreach (const GdbMi &child, all.children()) {
        WatchData dummy;
        dummy.iname = child.findChild("iname").data();
        dummy.name = _(child.findChild("name").data());
        //qDebug() << "CHILD: " << child.toString();
        parseWatchData(watchHandler->expandedINames(), dummy, child, &list);
    }
    watchHandler->insertBulkData(list);
}

void PdbEngine::handleLoadDumper(const PdbResponse &response)
{
    Q_UNUSED(response);
    //qDebug() << " DUMPERS LOADED '" << response.data << "'";
    continueInferior();
}

void PdbEngine::debugMessage(const QString &msg)
{
    showDebuggerOutput(LogDebug, msg);
}

IDebuggerEngine *createPdbEngine(DebuggerManager *manager)
{
    return new PdbEngine(manager);
}


} // namespace Internal
} // namespace Debugger
