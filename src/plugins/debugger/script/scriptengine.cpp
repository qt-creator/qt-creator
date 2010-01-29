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

#define QT_NO_CAST_FROM_ASCII

#include "scriptengine.h"

#include "debuggerdialogs.h"
#include "breakhandler.h"
#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <utils/qtcassert.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/ifile.h>
#include <coreplugin/scriptmanager/scriptmanager.h>
#include <coreplugin/icore.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QApplication>
#include <QtGui/QToolTip>

#include <QtScript/QScriptContext>
#include <QtScript/QScriptClassPropertyIterator>
#include <QtScript/QScriptContextInfo>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptEngineAgent>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>

// #define DEBUG_SCRIPT 1
#if DEBUG_SCRIPT
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s


namespace Debugger {
namespace Internal {

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

private:
    void maybeBreakNow(bool byFunction);

    ScriptEngine *q;
};

ScriptAgent::ScriptAgent(ScriptEngine *debugger, QScriptEngine *script)
  : QScriptEngineAgent(script), q(debugger)
{}

void ScriptAgent::contextPop()
{
    SDEBUG("ScriptAgent::contextPop: ");
}

void ScriptAgent::contextPush()
{
    SDEBUG("ScriptAgent::contextPush: ");
}

void ScriptAgent::exceptionCatch(qint64 scriptId, const QScriptValue & exception)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(exception)
    const QString msg = QString::fromLatin1("An exception was caught on %1: '%2'").
                        arg(scriptId).arg(exception.toString());
    SDEBUG(msg);
    q->showDebuggerOutput(LogMisc, msg);
}

void ScriptAgent::exceptionThrow(qint64 scriptId, const QScriptValue &exception,
    bool hasHandler)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(exception)
    Q_UNUSED(hasHandler)
    const QString msg = QString::fromLatin1("An exception occurred on %1: '%2'").
                        arg(scriptId).arg(exception.toString());
    SDEBUG(msg);
    q->showDebuggerOutput(LogMisc, msg);
}

void ScriptAgent::functionEntry(qint64 scriptId)
{
    Q_UNUSED(scriptId)
    q->showDebuggerOutput(LogMisc, QString::fromLatin1("Function entry occurred on %1").arg(scriptId));
    q->checkForBreakCondition(true);
}

void ScriptAgent::functionExit(qint64 scriptId, const QScriptValue &returnValue)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(returnValue)
    const QString msg = QString::fromLatin1("Function exit occurred on %1: '%2'").arg(scriptId).arg(returnValue.toString());
    SDEBUG(msg);
    q->showDebuggerOutput(LogMisc, msg);
}

void ScriptAgent::positionChange(qint64 scriptId, int lineNumber, int columnNumber)
{
    SDEBUG("ScriptAgent::position: " << lineNumber);
    Q_UNUSED(scriptId)
    Q_UNUSED(lineNumber)
    Q_UNUSED(columnNumber)
    q->checkForBreakCondition(false);
}

void ScriptAgent::scriptLoad(qint64 scriptId, const QString &program,
    const QString &fileName, int baseLineNumber)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(program)
    Q_UNUSED(fileName)
    Q_UNUSED(baseLineNumber)
    q->showDebuggerOutput(LogMisc, QString::fromLatin1("Loaded: %1 id: %2").arg(fileName).arg(scriptId));
}

void ScriptAgent::scriptUnload(qint64 scriptId)
{
    Q_UNUSED(scriptId)
    SDEBUG("ScriptAgent::scriptUnload: " << scriptId);
}


///////////////////////////////////////////////////////////////////////
//
// ScriptEngine
//
///////////////////////////////////////////////////////////////////////

ScriptEngine::ScriptEngine(DebuggerManager *manager)
    : IDebuggerEngine(manager)
{
}

ScriptEngine::~ScriptEngine()
{
}

void ScriptEngine::executeDebuggerCommand(const QString &command)
{
    Q_UNUSED(command)
    XSDEBUG("FIXME:  ScriptEngine::executeDebuggerCommand()");
}

void ScriptEngine::shutdown()
{
    exitDebugger();
}

void ScriptEngine::exitDebugger()
{
    if (state() == DebuggerNotReady)
        return;
    SDEBUG("ScriptEngine::exitDebugger()");
    m_stopped = false;
    m_stopOnNextLine = false;
    if (m_scriptEngine->isEvaluating())
        m_scriptEngine->abortEvaluation();
    setState(InferiorShuttingDown);
    setState(InferiorShutDown);

    setState(EngineShuttingDown);
    m_scriptEngine->setAgent(0);
    setState(DebuggerNotReady);
}

void ScriptEngine::startDebugger(const DebuggerStartParametersPtr &sp)
{
    setState(AdapterStarting);
    if (m_scriptEngine.isNull())
        m_scriptEngine = Core::ICore::instance()->scriptManager()->scriptEngine();
    if (!m_scriptAgent)
        m_scriptAgent.reset(new ScriptAgent(this, m_scriptEngine.data()));
    m_scriptEngine->setAgent(m_scriptAgent.data());
    /* Keep the gui alive (have the engine call processEvents() while the script
     * is run in the foreground). */
    m_scriptEngine->setProcessEventsInterval(1 /*ms*/);

    m_stopped = false;
    m_stopOnNextLine = false;
    m_scriptEngine->abortEvaluation();

    setState(AdapterStarted);
    setState(InferiorStarting);

    m_scriptFileName = QFileInfo (sp->executable).absoluteFilePath();
    QFile scriptFile(m_scriptFileName);
    if (!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        manager()->showDebuggerOutput(LogError, QString::fromLatin1("Cannot open %1: %2").
                                      arg(m_scriptFileName, scriptFile.errorString()));
        emit startFailed();
        return;
    }
    QTextStream stream(&scriptFile);
    m_scriptContents = stream.readAll();
    scriptFile.close();
    attemptBreakpointSynchronization();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Running requested..."), 5000);
    manager()->showDebuggerOutput(LogMisc, QLatin1String("Running: ") + m_scriptFileName);
    QTimer::singleShot(0, this, SLOT(runInferior()));
    emit startSuccessful();
}

void ScriptEngine::continueInferior()
{
    SDEBUG("ScriptEngine::continueInferior()");
    m_stopped = false;
    m_stopOnNextLine = false;
}

static const char *qtExtensionsC[] = {
    "qt.core", "qt.gui", "qt.xml", "qt.svg", "qt.network",
    "qt.sql", "qt.opengl", "qt.webkit", "qt.xmlpatterns", "qt.uitools"
};

bool ScriptEngine::importExtensions()
{
    SDEBUG("ScriptEngine::importExtensions()");
    QStringList extensions;
    const int extCount = sizeof(qtExtensionsC)/sizeof(const char *);
    for (int  e = 0; e < extCount; e++)
        extensions.append(QLatin1String(qtExtensionsC[e]));
    if (m_scriptEngine->importedExtensions().contains(extensions.front()))
        return true;
    QDir dir(QLatin1String("/home/apoenitz/dev/qtscriptgenerator"));
    if (!dir.cd(QLatin1String("plugins"))) {
        fprintf(stderr, "plugins folder does not exist -- did you build the bindings?\n");
        return false;
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
                     qPrintable(failExtensions.join(QLatin1String(", "))), qPrintable(dir.absolutePath()));
        }
    }
    return failExtensions.isEmpty();
}

void ScriptEngine::runInferior()
{
    SDEBUG("ScriptEngine::runInferior()");
    importExtensions();
    setState(InferiorRunning);
    const QScriptValue result = m_scriptEngine->evaluate(m_scriptContents, m_scriptFileName);
    setState(InferiorStopping);
    setState(InferiorStopped);
    if (m_scriptEngine->hasUncaughtException()) {
        QString msg = QString::fromLatin1("An exception occurred during execution at line: %1\n%2\n").
                      arg(m_scriptEngine->uncaughtExceptionLineNumber()).arg(m_scriptEngine->uncaughtException().toString());
        msg += m_scriptEngine->uncaughtExceptionBacktrace().join(QString(QLatin1Char('\n')));
        showDebuggerOutput(LogMisc, msg);
    } else {
        showDebuggerOutput(LogMisc, QString::fromLatin1("Evaluation returns '%1'").arg(result.toString()));
    }
    exitDebugger();
}

void ScriptEngine::interruptInferior()
{
    m_stopped = false;
    m_stopOnNextLine = true;
    XSDEBUG("ScriptEngine::interruptInferior()");
}

void ScriptEngine::stepExec()
{
    //SDEBUG("ScriptEngine::stepExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::stepIExec()
{
    //SDEBUG("ScriptEngine::stepIExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::stepOutExec()
{
    //SDEBUG("ScriptEngine::stepOutExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::nextExec()
{
    //SDEBUG("ScriptEngine::nextExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::nextIExec()
{
    //SDEBUG("ScriptEngine::nextIExec()");
    m_stopped = false;
    m_stopOnNextLine = true;
}

void ScriptEngine::runToLineExec(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    SDEBUG("FIXME:  ScriptEngine::runToLineExec()");
}

void ScriptEngine::runToFunctionExec(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  ScriptEngine::runToFunctionExec()");
}

void ScriptEngine::jumpToLineExec(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
    XSDEBUG("FIXME:  ScriptEngine::jumpToLineExec()");
}

void ScriptEngine::activateFrame(int index)
{
    Q_UNUSED(index)
}

void ScriptEngine::selectThread(int index)
{
    Q_UNUSED(index)
}

void ScriptEngine::attemptBreakpointSynchronization()
{
    BreakHandler *handler = manager()->breakHandler();
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
        if (!data->fileName.isEmpty() && data->markerFileName.isEmpty()) {
            data->markerFileName = data->fileName;
            data->markerLineNumber = data->lineNumber.toInt();
            updateNeeded = true;
        }
    }
    if (updateNeeded)
        handler->updateMarkers();
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

QList<Symbol> ScriptEngine::moduleSymbols(const QString & /*moduleName*/)
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

void ScriptEngine::setToolTipExpression(const QPoint &mousePos,
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
    const QString javaScriptMimeType =
        QLatin1String("application/javascript");
    if (!editor->file() || editor->file()->mimeType() != javaScriptMimeType)
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

void ScriptEngine::assignValueInDebugger(const QString &expression,
    const QString &value)
{
    XSDEBUG("ASSIGNING: " << (expression + QLatin1Char('=') + value));
    m_scriptEngine->evaluate(expression + QLatin1Char('=') + value);
    updateLocals();
}

static BreakpointData *findBreakPointByFunction(BreakHandler *handler,
                                                const QString &functionName)
{
    const int count = handler->size();
    for (int b = 0; b < count; b++) {
        BreakpointData *data = handler->at(b);
        if (data->funcName == functionName)
            return data;
    }
    return 0;
}

static BreakpointData *findBreakPointByFileName(BreakHandler *handler,
                                              int lineNumber,
                                              const QString &fileName)
{
    const int count = handler->size();
    for (int b = 0; b < count; b++) {
        BreakpointData *data = handler->at(b);
        if (lineNumber == data->lineNumber.toInt() && fileName == data->fileName)
            return data;
    }
    return 0;
}

bool ScriptEngine::checkForBreakCondition(bool byFunction)
{
    const QScriptContext *context = m_scriptEngine->currentContext();
    const QScriptContextInfo info(context);

    // Update breakpoints
    const QString functionName = info.functionName();
    const QString fileName = info.fileName();
    const int lineNumber = byFunction? info.functionStartLineNumber() : info.lineNumber();
    SDEBUG("checkForBreakCondition" << byFunction << functionName << lineNumber << fileName);
    if (m_stopOnNextLine) {
        // Interrupt inferior
        m_stopOnNextLine = false;
    } else {
        if (byFunction && functionName.isEmpty())
            return false;
        BreakpointData *data = byFunction ?
                               findBreakPointByFunction(manager()->breakHandler(), functionName) :
                               findBreakPointByFileName(manager()->breakHandler(), lineNumber, fileName);
        if (!data)
            return false;

        // we just run into a breakpoint
        //SDEBUG("RESOLVING BREAKPOINT AT " << fileName << lineNumber);
        data->bpLineNumber = QByteArray::number(lineNumber);
        data->bpFileName = fileName;
        data->bpFuncName = functionName;
        data->markerLineNumber = lineNumber;
        data->markerFileName = fileName;
        data->pending = false;
        data->updateMarker();
    }
    setState(InferiorStopping);
    setState(InferiorStopped);
    SDEBUG("Stopped at " << lineNumber << fileName);
    showStatusMessage(tr("Stopped at %1:%2.").arg(fileName).arg(lineNumber), 5000);

    StackFrame frame;
    frame.file = fileName;
    frame.line = lineNumber;
    manager()->gotoLocation(frame, true);
    updateLocals();
    return true;
}

void ScriptEngine::updateLocals()
{
    QScriptContext *context = m_scriptEngine->currentContext();
    manager()->watchHandler()->beginCycle();
    //SDEBUG("UPDATE LOCALS");

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
    manager()->stackHandler()->setFrames(stackFrames);

    //
    // Build locals, deactivate agent meanwhile.
    //
    m_scriptEngine->setAgent(0);

    WatchData data;
    data.iname = "local";
    data.name = QString::fromLatin1(data.iname);
    data.scriptValue = context->activationObject();
    manager()->watchHandler()->beginCycle();
    updateSubItem(data);
    manager()->watchHandler()->endCycle();
    // FIXME: Use an extra thread. This here is evil
    m_stopped = true;
    showStatusMessage(tr("Stopped."), 5000);
    while (m_stopped) {
        //SDEBUG("LOOPING");
        QApplication::processEvents();
    }
    setState(InferiorRunningRequested);
    setState(InferiorRunning);
    // Clear any exceptions occurred during locals evaluation.
    m_scriptEngine->clearExceptions();
    m_scriptEngine->setAgent(m_scriptAgent.data());
    SDEBUG("Continuing");
}

void ScriptEngine::updateWatchData(const WatchData &data)
{
    updateSubItem(data);
}

static inline QString msgDebugInsert(const WatchData &d0, const QList<WatchData>& children)
{
    QString rc;
    QTextStream str(&rc);
    str << "INSERTING " << d0.toString() << '\n';
    foreach(const WatchData &c, children)
        str << "          " << c.toString() << '\n';
    return rc;
}

void ScriptEngine::updateSubItem(const WatchData &data0)
{
    WatchData data = data0;
    QList<WatchData> children;
    SDEBUG("\nUPDATE SUBITEM: " << data.toString() << data.scriptValue.toString());
    QTC_ASSERT(data.isValid(), return);

    if (data.isTypeNeeded() || data.isValueNeeded()) {
        const QScriptValue &ob = data.scriptValue;
        if (ob.isArray()) {
            data.setType(QLatin1String("Array"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isBool()) {
            data.setType(QLatin1String("Bool"), false);
            data.setValue(ob.toBool() ? QLatin1String("true") : QLatin1String("false"));
            data.setHasChildren(false);
        } else if (ob.isDate()) {
            data.setType(QLatin1String("Date"), false);
            data.setValue(ob.toDateTime().toString());
            data.setHasChildren(false);
        } else if (ob.isError()) {
            data.setType(QLatin1String("Error"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isFunction()) {
            data.setType(QLatin1String("Function"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isNull()) {
            const QString nullValue = QLatin1String("<null>");
            data.setType(nullValue, false);
            data.setValue(nullValue);
        } else if (ob.isNumber()) {
            data.setType(QLatin1String("Number"), false);
            data.setValue(QString::number(ob.toNumber()));
            data.setHasChildren(false);
        } else if (ob.isObject()) {
            data.setType(QLatin1String("Object"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isQMetaObject()) {
            data.setType(QLatin1String("QMetaObject"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isQObject()) {
            data.setType(QLatin1String("QObject"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isRegExp()) {
            data.setType(QLatin1String("RegExp"), false);
            data.setValue(ob.toRegExp().pattern());
        } else if (ob.isString()) {
            data.setType(QLatin1String("String"), false);
            data.setValue(ob.toString());
        } else if (ob.isVariant()) {
            data.setType(QLatin1String("Variant"), false);
            data.setValue(QString(QLatin1Char(' ')));
        } else if (ob.isUndefined()) {
            data.setType(QLatin1String("<undefined>"), false);
            data.setValue(QLatin1String("<unknown>"));
            data.setHasChildren(false);
        } else {
            const QString unknown = QLatin1String("<unknown>");
            data.setType(unknown, false);
            data.setValue(unknown);
            data.setHasChildren(false);
        }
    }

    if (data.isChildrenNeeded()) {
        QScriptValueIterator it(data.scriptValue);
        while (it.hasNext()) {
            it.next();
            WatchData data1;
            data1.iname = data.iname + '.' + it.name().toLatin1();
            data1.exp = it.name().toLatin1();
            data1.name = it.name();
            data1.scriptValue = it.value();
            if (manager()->watchHandler()->isExpandedIName(data1.iname)) {
                data1.setChildrenNeeded();
            } else {
                data1.setChildrenUnneeded();
            }
            children.push_back(data1);
        }
        data.setHasChildren(!children.isEmpty());
        data.setChildrenUnneeded();
    }

    if (data.isHasChildrenNeeded()) {
        QScriptValueIterator it(data.scriptValue);
        data.setHasChildren(it.hasNext());
    }

    SDEBUG(msgDebugInsert(data, children));
    manager()->watchHandler()->insertData(data);
    if (!children.isEmpty())
        manager()->watchHandler()->insertBulkData(children);
}

void ScriptEngine::showDebuggerOutput(int channel, const QString &m)
{
    manager()->showDebuggerOutput(channel, m);
}

IDebuggerEngine *createScriptEngine(DebuggerManager *manager)
{
    return new ScriptEngine(manager);
}

} // namespace Internal
} // namespace Debugger
