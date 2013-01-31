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

#include "scriptengine.h"

#include "debuggerstartparameters.h"
#include "breakhandler.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerstringutils.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "sourceutils.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "debuggertooltipmanager.h"

#include <utils/qtcassert.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/idocument.h>
#include <coreplugin/scriptmanager/scriptmanager.h>
#include <coreplugin/icore.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <QApplication>
#include <QMessageBox>
#include <QToolTip>

#include <QScriptContext>
#include <QScriptClassPropertyIterator>
#include <QScriptContextInfo>
#include <QScriptEngine>
#include <QScriptEngineAgent>
#include <QScriptValue>
#include <QScriptValueIterator>


namespace Debugger {
namespace Internal {

enum { debugScript = 0 };

#define SDEBUG(s) do { if (debugScript) qDebug() << s; } while (0)
#define XSDEBUG(s) qDebug() << s

///////////////////////////////////////////////////////////////////////
//
// ScriptEngine
//
///////////////////////////////////////////////////////////////////////

class ScriptAgent : public QScriptEngineAgent
{
public:
    ScriptAgent(ScriptEngine *debugger, QScriptEngine *script);
    ~ScriptAgent() {}

    void contextPop();
    void contextPush();
    void exceptionCatch(qint64 scriptId, const QScriptValue &exception);
    void exceptionThrow(qint64 scriptId, const QScriptValue & exception,
        bool hasHandler);
    void functionEntry(qint64 scriptId);
    void functionExit(qint64 scriptId, const QScriptValue &returnValue);
    void positionChange(qint64 scriptId, int lineNumber, int columnNumber);
    void scriptLoad(qint64 id, const QString &program, const QString &fileName,
        int baseLineNumber);
    void scriptUnload(qint64 id);

    void showMessage(const QString &msg);

private:
    void maybeBreakNow(bool byFunction);

    ScriptEngine *q;
    int m_depth;
    int m_contextDepth;
};

ScriptAgent::ScriptAgent(ScriptEngine *debugger, QScriptEngine *script)
  : QScriptEngineAgent(script), q(debugger), m_depth(0), m_contextDepth(0)
{}

void ScriptAgent::showMessage(const QString &msg)
{
    SDEBUG(msg);
    q->showMessage(msg, LogMisc);
}

void ScriptAgent::contextPop()
{
    //showMessage(_("ContextPop: %1").arg(m_contextDepth));
    --m_contextDepth;
}

void ScriptAgent::contextPush()
{
    ++m_contextDepth;
    //showMessage(_("ContextPush: %1 ").arg(m_contextDepth));
}

void ScriptAgent::exceptionCatch(qint64 scriptId, const QScriptValue & exception)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(exception)
    showMessage(_("An exception was caught on %1: '%2'").
                   arg(scriptId).arg(exception.toString()));
}

void ScriptAgent::exceptionThrow(qint64 scriptId, const QScriptValue &exception,
    bool hasHandler)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(exception)
    Q_UNUSED(hasHandler)
    showMessage(_("An exception occurred on %1: '%2'").
                   arg(scriptId).arg(exception.toString()));
}

void ScriptAgent::functionEntry(qint64 scriptId)
{
    Q_UNUSED(scriptId)
    ++m_depth;
    //showMessage(_("Function entry occurred on %1, depth: %2").arg(scriptId));
    q->checkForBreakCondition(true);
}

void ScriptAgent::functionExit(qint64 scriptId, const QScriptValue &returnValue)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(returnValue)
    --m_depth;
    //showMessage(_("Function exit occurred on %1: '%2'").
    //              arg(scriptId).arg(returnValue.toString()));
}

void ScriptAgent::positionChange(qint64 scriptId, int lineNumber, int columnNumber)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(lineNumber)
    Q_UNUSED(columnNumber)
    //showMessage(_("Position: %1").arg(lineNumber));
    q->checkForBreakCondition(false);
}

void ScriptAgent::scriptLoad(qint64 scriptId, const QString &program,
    const QString &fileName, int baseLineNumber)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(program)
    Q_UNUSED(fileName)
    Q_UNUSED(baseLineNumber)
    showMessage(_("Loaded: %1 id: %2").arg(fileName).arg(scriptId));
}

void ScriptAgent::scriptUnload(qint64 scriptId)
{
    Q_UNUSED(scriptId)
    showMessage(_("Unload script id %1 ").arg(scriptId));
}


///////////////////////////////////////////////////////////////////////
//
// ScriptEngine
//
///////////////////////////////////////////////////////////////////////

ScriptEngine::ScriptEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("ScriptEngine"));
}

ScriptEngine::~ScriptEngine()
{
}

void ScriptEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    Q_UNUSED(command)
    Q_UNUSED(languages)
    XSDEBUG("FIXME:  ScriptEngine::executeDebuggerCommand()");
}

void ScriptEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    SDEBUG("ScriptEngine::shutdownInferior()");
    m_scriptEngine->setAgent(0);
    //m_scriptAgent.reset(0);
    m_stopped = false;
    m_stopOnNextLine = false;
    if (m_scriptEngine->isEvaluating())
        m_scriptEngine->abortEvaluation();
    notifyInferiorShutdownOk();
}

void ScriptEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_scriptEngine->setAgent(0);
    notifyEngineShutdownOk();
}

void ScriptEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("STARTING SCRIPT DEBUGGER"), LogMisc);
    if (m_scriptEngine.isNull())
        m_scriptEngine = Core::ICore::scriptManager()->scriptEngine();
    QTC_CHECK(!m_scriptAgent);
    m_scriptAgent.reset(new ScriptAgent(this, m_scriptEngine.data()));
    m_scriptEngine->setAgent(m_scriptAgent.data());
    //m_scriptEngine->setAgent(new ScriptAgent(this, m_scriptEngine.data()));
    /* Keep the gui alive (have the engine call processEvents() while the script
     * is run in the foreground). */
    m_scriptEngine->setProcessEventsInterval(1 /*ms*/);

    m_stopped = false;
    m_stopOnNextLine = false;
    m_scriptEngine->abortEvaluation();

    notifyEngineSetupOk();
}

void ScriptEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    m_scriptFileName = QFileInfo(startParameters().executable).absoluteFilePath();
    showMessage(_("SCRIPT FILE: ") + m_scriptFileName);
    QFile scriptFile(m_scriptFileName);
    if (!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        showMessageBox(QMessageBox::Critical, tr("Error:"),
            _("Cannot open script file %1:\n%2").
          arg(m_scriptFileName, scriptFile.errorString()));
        notifyInferiorSetupFailed();
        return;
    }
    QTextStream stream(&scriptFile);
    m_scriptContents = stream.readAll();
    scriptFile.close();
    attemptBreakpointSynchronization();
    notifyInferiorSetupOk();
}

void ScriptEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    SDEBUG("ScriptEngine::continueInferior()");
    m_stopped = false;
    m_stopOnNextLine = false;
}

static const char *qtExtensionsC[] = {
    "qt.core", "qt.gui", "qt.xml", "qt.svg", "qt.network",
    "qt.sql", "qt.opengl", "qt.webkit", "qt.xmlpatterns", "qt.uitools"
};

void ScriptEngine::importExtensions()
{
    SDEBUG("ScriptEngine::importExtensions()");
    QStringList extensions;
    const int extCount = sizeof(qtExtensionsC)/sizeof(const char *);
    for (int  e = 0; e < extCount; e++)
        extensions.append(QLatin1String(qtExtensionsC[e]));
    if (m_scriptEngine->importedExtensions().contains(extensions.front()))
        return; // true;
    QDir dir(QLatin1String("/home/apoenitz/dev/qtscriptgenerator"));
    if (!dir.cd(QLatin1String("plugins"))) {
        fprintf(stderr, "plugins folder does not exist -- did you build the bindings?\n");
        return; // false;
    }
    QStringList paths = qApp->libraryPaths();
    paths <<  dir.absolutePath();
    qApp->setLibraryPaths(paths);
    QStringList failExtensions;
    foreach (const QString &ext, extensions) {
        QScriptValue ret = m_scriptEngine->importExtension(ext);
        if (ret.isError())
            failExtensions.append(ext);
    }
    if (!failExtensions.isEmpty()) {
        if (failExtensions.size() == extensions.size()) {
            qWarning("Failed to import Qt bindings!\n"
                     "Plugins directory searched: %s/script\n"
                     "Make sure that the bindings have been built, "
                     "and that this executable and the plugins are "
                     "using compatible Qt libraries.", qPrintable(dir.absolutePath()));
        } else {
            qWarning("Failed to import some Qt bindings: %s\n"
                     "Plugins directory searched: %s/script\n"
                     "Make sure that the bindings have been built, "
                     "and that this executable and the plugins are "
                     "using compatible Qt libraries.",
                     qPrintable(failExtensions.join(QLatin1String(", "))),
                        qPrintable(dir.absolutePath()));
        }
    }
    return; // failExtensions.isEmpty();
}

void ScriptEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    notifyEngineRunAndInferiorRunOk();
    showStatusMessage(tr("Running requested..."), 5000);
    showMessage(QLatin1String("Running: ") + m_scriptFileName, LogMisc);
    importExtensions();
    const QScriptValue result =
        m_scriptEngine->evaluate(m_scriptContents, m_scriptFileName);
    QString msg;
    if (m_scriptEngine->hasUncaughtException()) {
        msg = _("An exception occurred during execution at line: %1\n%2\n")
            .arg(m_scriptEngine->uncaughtExceptionLineNumber())
            .arg(m_scriptEngine->uncaughtException().toString());
        msg += m_scriptEngine->uncaughtExceptionBacktrace()
            .join(QString(QLatin1Char('\n')));
    } else {
        msg = _("Evaluation returns '%1'").arg(result.toString());
    }
    showMessage(msg, LogMisc);
    showMessage(_("This was the outermost function."));
    notifyInferiorExited();
}

void ScriptEngine::interruptInferior()
{
    m_stopOnNextLine = true;
    XSDEBUG("ScriptEngine::interruptInferior()");
}

void ScriptEngine::executeStep()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    //SDEBUG("ScriptEngine::stepExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::executeStepI()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    //SDEBUG("ScriptEngine::stepIExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::executeStepOut()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    //SDEBUG("ScriptEngine::stepOutExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::executeNext()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    //SDEBUG("ScriptEngine::nextExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::executeNextI()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    //SDEBUG("ScriptEngine::nextIExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::executeRunToLine(const ContextData &data)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    Q_UNUSED(data)
    SDEBUG("FIXME:  ScriptEngine::runToLineExec()");
}

void ScriptEngine::executeRunToFunction(const QString &functionName)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  ScriptEngine::runToFunctionExec()");
}

void ScriptEngine::executeJumpToLine(const ContextData &data)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    Q_UNUSED(data)
    XSDEBUG("FIXME:  ScriptEngine::jumpToLineExec()");
}

void ScriptEngine::activateFrame(int index)
{
    Q_UNUSED(index)
}

void ScriptEngine::selectThread(ThreadId threadId)
{
    Q_UNUSED(threadId)
}

bool ScriptEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    const QString fileName = breakHandler()->fileName(id);
    return fileName.endsWith(QLatin1String(".js"));
}

void ScriptEngine::attemptBreakpointSynchronization()
{
    QTC_ASSERT(false, /* FIXME */);
/*
    BreakHandler *handler = breakHandler();
    bool updateNeeded = false;
    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        if (data->pending) {
            data->pending = false; // FIXME
            updateNeeded = true;
        }
        if (data->bpNumber.isEmpty()) {
            data->bpNumber = QByteArray::number(index + 1);
            updateNeeded = true;
        }
        if (!data->fileName.isEmpty() && data->markerFileName().isEmpty()) {
            data->setMarkerFileName(data->fileName);
            data->setMarkerLineNumber(data->lineNumber);
            updateNeeded = true;
        }
    }
    if (updateNeeded)
        handler->updateMarkers();
*/
}

void ScriptEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void ScriptEngine::loadAllSymbols()
{
}

void ScriptEngine::reloadModules()
{
}

void ScriptEngine::requestModuleSymbols(const QString & /*moduleName*/)
{
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////


static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

bool ScriptEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, const DebuggerToolTipContext &ctx)
{
    Q_UNUSED(mousePos)
    Q_UNUSED(editor)

    if (state() != InferiorStopOk) {
        //SDEBUG("SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED");
        return false;
    }
    // Check mime type and get expression (borrowing some C++ - functions)
    const QString javaScriptMimeType =
        QLatin1String("application/javascript");
    if (!editor->document() || editor->document()->mimeType() != javaScriptMimeType)
        return false;

    int line;
    int column;
    QString exp = cppExpressionAt(editor, ctx.position, &line, &column);

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
        return false;
    }

    if (!hasLetterOrNumber(exp)) {
        QToolTip::showText(m_toolTipPos, tr("'%1' contains no identifier.").arg(exp));
        return false;
    }

    if (exp.startsWith(QLatin1Char('"')) && exp.endsWith(QLatin1Char('"'))) {
        QToolTip::showText(m_toolTipPos, tr("String literal %1.").arg(exp));
        return false;
    }

    if (exp.startsWith(QLatin1String("++")) || exp.startsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.endsWith(QLatin1String("++")) || exp.endsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.startsWith(QLatin1Char('<')) || exp.startsWith(QLatin1Char('[')))
        return false;

    if (hasSideEffects(exp)) {
        QToolTip::showText(m_toolTipPos,
            tr("Cowardly refusing to evaluate expression '%1' "
               "with potential side effects.").arg(exp));
        return false;
    }

#if 0
    //if (m_manager->status() != InferiorStopOk)
    //    return;

    // FIXME: 'exp' can contain illegal characters
    m_toolTip = WatchData();
    m_toolTip.exp = exp;
    m_toolTip.name = exp;
    m_toolTip.iname = tooltipIName;
    insertData(m_toolTip);
#endif
    return false;
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void ScriptEngine::assignValueInDebugger(const Internal::WatchData *,
                                         const QString &expression, const QVariant &value)
{
    SDEBUG("ASSIGNING: " << (expression + QLatin1Char('=') + value.toString()));
    m_scriptEngine->evaluate(expression + QLatin1Char('=') + value.toString());
    updateLocals();
}

bool ScriptEngine::checkForBreakCondition(bool byFunction)
{
    // FIXME: Should that ever happen after setAgent(0) in shutdownInferior()?
    // In practice, it does, so chicken out.
    if (targetState() == DebuggerFinished)
        return false;

    const QScriptContext *context = m_scriptEngine->currentContext();
    const QScriptContextInfo info(context);

    // Update breakpoints
    const QString functionName = info.functionName();
    const QString fileName = info.fileName();
    const int lineNumber = byFunction
        ? info.functionStartLineNumber() : info.lineNumber();
    SDEBUG(Q_FUNC_INFO << byFunction << functionName << lineNumber << fileName);
    if (m_stopOnNextLine) {
        // Interrupt inferior
        m_stopOnNextLine = false;
    } else {
        if (byFunction && functionName.isEmpty())
            return false;
        BreakHandler *handler = breakHandler();
        BreakpointModelId id = byFunction ?
           handler->findBreakpointByFunction(functionName) :
           handler->findBreakpointByFileAndLine(fileName, lineNumber, false);

        // Skip disabled breakpoint.
        if (!handler->isEnabled(id))
            return false;

        BreakpointResponse br;
        // We just run into a breakpoint.
        //SDEBUG("RESOLVING BREAKPOINT AT " << fileName << lineNumber);
        br.lineNumber = lineNumber;
        br.fileName = fileName;
        br.functionName = functionName;
        handler->notifyBreakpointInsertOk(id);
        handler->setResponse(id, br);
    }
    notifyInferiorSpontaneousStop();
    SDEBUG("Stopped at " << lineNumber << fileName);
    showStatusMessage(tr("Stopped at %1:%2.").arg(fileName).arg(lineNumber), 5000);
    gotoLocation(Location(fileName, lineNumber));
    updateLocals();
    return true;
}

void ScriptEngine::updateLocals()
{
    QScriptContext *context = m_scriptEngine->currentContext();
    SDEBUG(Q_FUNC_INFO);

    //
    // Build stack
    //
    QList<StackFrame> stackFrames;
    int i = 0;
    for (QScriptContext *c = context; c; c = c->parentContext(), ++i) {
        const QScriptContextInfo info(c);
        StackFrame frame;
        frame.level = i;
        frame.file = info.fileName();
        frame.function = info.functionName();
        frame.from = QString::number(info.functionStartLineNumber());
        frame.to = QString::number(info.functionEndLineNumber());
        frame.line = info.lineNumber();
        if (frame.function.isEmpty())
            frame.function = QLatin1String("<global scope>");
        //frame.address = ...;
        stackFrames.append(frame);
    }
    stackHandler()->setFrames(stackFrames);

    //
    // Build locals, deactivate agent meanwhile.
    //
    m_scriptEngine->setAgent(0);

    WatchData data;
    data.id = m_watchIdCounter++;
    m_watchIdToScriptValue.insert(data.id, context->activationObject());
    data.iname = "local";
    data.name = _(data.iname);

    updateSubItem(data);
    // FIXME: Use an extra thread. This here is evil.
    m_stopped = true;
    showStatusMessage(tr("Stopped."), 5000);
    while (m_stopped) {
        //SDEBUG("LOOPING");
        QApplication::processEvents();
    }
    // Clear any exceptions occurred during locals evaluation.
    m_scriptEngine->clearExceptions();
    m_scriptEngine->setAgent(m_scriptAgent.data());
    notifyInferiorRunOk();
}

void ScriptEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    Q_UNUSED(flags);
    updateSubItem(data);
}

static inline QString msgDebugInsert(const WatchData &d0, const QList<WatchData>& children)
{
    QString rc;
    QTextStream str(&rc);
    str << "INSERTING " << d0.toString() << '\n';
    foreach (const WatchData &c, children)
        str << "          " << c.toString() << '\n';
    return rc;
}

void ScriptEngine::updateSubItem(const WatchData &data0)
{
    WatchData data = data0;
    QList<WatchData> children;
    //SDEBUG("\nUPDATE SUBITEM: " << data.toString() << data.scriptValue.toString());
    QTC_ASSERT(data.isValid(), return);

    const QScriptValue &ob = m_watchIdToScriptValue.value(data.id);
    if (data.isTypeNeeded() || data.isValueNeeded()) {
        if (ob.isArray()) {
            data.setType("Array", false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isBool()) {
            data.setType("Bool", false);
            data.setValue(ob.toBool() ? QLatin1String("true") : QLatin1String("false"));
            data.setHasChildren(false);
        } else if (ob.isDate()) {
            data.setType("Date", false);
            data.setValue(ob.toDateTime().toString());
            data.setHasChildren(false);
        } else if (ob.isError()) {
            data.setType("Error", false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isFunction()) {
            data.setType("Function", false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isNull()) {
            const QString nullValue = QLatin1String("<null>");
            data.setType("<null>", false);
            data.setValue(nullValue);
        } else if (ob.isNumber()) {
            data.setType("Number", false);
            data.setValue(QString::number(ob.toNumber()));
            data.setHasChildren(false);
        } else if (ob.isObject()) {
            data.setType("Object", false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isQMetaObject()) {
            data.setType("QMetaObject", false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isQObject()) {
            data.setType("QObject", false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isRegExp()) {
            data.setType("RegExp", false);
            data.setValue(ob.toRegExp().pattern());
        } else if (ob.isString()) {
            data.setType("String", false);
            data.setValue(ob.toString());
        } else if (ob.isVariant()) {
            data.setType("Variant", false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isUndefined()) {
            data.setType("<undefined>", false);
            data.setValue(QLatin1String("<unknown>"));
            data.setHasChildren(false);
        } else {
            const QString unknown = QLatin1String("<unknown>");
            data.setType("<unknown>", false);
            data.setValue(unknown);
            data.setHasChildren(false);
        }
    }

    if (data.isChildrenNeeded()) {
        QScriptValueIterator it(ob);
        while (it.hasNext()) {
            it.next();
            WatchData data1;
            data1.iname = data.iname + '.' + it.name().toLatin1();
            data1.exp = it.name().toLatin1();
            data1.name = it.name();
            data.id = m_watchIdCounter++;
            m_watchIdToScriptValue.insert(data.id, it.value());
            if (watchHandler()->isExpandedIName(data1.iname))
                data1.setChildrenNeeded();
            else
                data1.setChildrenUnneeded();
            children.push_back(data1);
        }
        data.setHasChildren(!children.isEmpty());
        data.setChildrenUnneeded();
    }

    if (data.isHasChildrenNeeded()) {
        QScriptValueIterator it(ob);
        data.setHasChildren(it.hasNext());
    }

    SDEBUG(msgDebugInsert(data, children));
    watchHandler()->insertIncompleteData(data);
    if (!children.isEmpty())
        watchHandler()->insertData(children);
}

DebuggerEngine *createScriptEngine(const DebuggerStartParameters &sp)
{
    return new ScriptEngine(sp);
}

} // namespace Internal
} // namespace Debugger
