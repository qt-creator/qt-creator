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
#include "moduleshandler.h"

#include <utils/qtcassert.h>

#include <qtscripteditor/qtscripteditorconstants.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/ifile.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QToolTip>

#include <QtScript/QScriptContext>
#include <QtScript/QScriptClassPropertyIterator>
#include <QtScript/QScriptContextInfo>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptEngineAgent>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>

//#define DEBUG_SCRIPT 1
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
    SDEBUG("ScriptAgent::exceptionCatch: " << scriptId << &exception);
}

void ScriptAgent::exceptionThrow(qint64 scriptId, const QScriptValue &exception,
    bool hasHandler)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(exception)
    Q_UNUSED(hasHandler)
    SDEBUG("ScriptAgent::exceptionThrow: " << scriptId << &exception
        << hasHandler);
}

void ScriptAgent::functionEntry(qint64 scriptId)
{
    Q_UNUSED(scriptId)
    q->maybeBreakNow(true);
}

void ScriptAgent::functionExit(qint64 scriptId, const QScriptValue &returnValue)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(returnValue)
    SDEBUG("ScriptAgent::functionExit: " << scriptId << &returnValue);
}

void ScriptAgent::positionChange(qint64 scriptId, int lineNumber, int columnNumber)
{
    SDEBUG("ScriptAgent::position: " << lineNumber);
    Q_UNUSED(scriptId)
    Q_UNUSED(lineNumber)
    Q_UNUSED(columnNumber)
    q->maybeBreakNow(false);
}

void ScriptAgent::scriptLoad(qint64 scriptId, const QString &program,
    const QString &fileName, int baseLineNumber)
{
    Q_UNUSED(scriptId)
    Q_UNUSED(program)
    Q_UNUSED(fileName)
    Q_UNUSED(baseLineNumber)
    SDEBUG("ScriptAgent::scriptLoad: " << program << fileName
      << baseLineNumber);
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

ScriptEngine::ScriptEngine(DebuggerManager *parent)
{
    q = parent;
    qq = parent->engineInterface();
    // created in startDebugger()
    m_scriptEngine = 0;
    m_scriptAgent = 0;
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
    SDEBUG("ScriptEngine::exitDebugger()");
    m_stopped = false;
    m_stopOnNextLine = false;
    m_scriptEngine->abortEvaluation();
    qq->notifyInferiorExited();
}

void ScriptEngine::startDebugger(const DebuggerStartParametersPtr &sp)
{
    if (!m_scriptEngine)
        m_scriptEngine = new QScriptEngine(this);
    if (!m_scriptAgent)
        m_scriptAgent = new ScriptAgent(this, m_scriptEngine);
    m_scriptEngine->setAgent(m_scriptAgent);
    m_scriptEngine->setProcessEventsInterval(1 /*ms*/);

    m_stopped = false;
    m_stopOnNextLine = false;
    m_scriptEngine->abortEvaluation();

    QFileInfo fi(sp->executable);
    m_scriptFileName = fi.absoluteFilePath();
    QFile scriptFile(m_scriptFileName);
    if (!scriptFile.open(QIODevice::ReadOnly)) {
        emit startFailed();
        return;
    }
    QTextStream stream(&scriptFile);
    m_scriptContents = stream.readAll();
    scriptFile.close();
    attemptBreakpointSynchronization();
    qq->notifyInferiorRunningRequested();
    QTimer::singleShot(0, this, SLOT(runInferior()));
    emit startSuccessful();
}

void ScriptEngine::continueInferior()
{
    SDEBUG("ScriptEngine::continueInferior()");
    m_stopped = false;
    m_stopOnNextLine = false;
}

void ScriptEngine::runInferior()
{
    //QDir dir(QApplication::applicationDirPath());
    //if (dir.dirName() == QLatin1String("debug") || dir.dirName() == QLatin1String("release"))
    //    dir.cdUp();
    //dir.cdUp();
    //dir.cdUp();
    QDir dir("/home/apoenitz/dev/qtscriptgenerator");
    if (!dir.cd("plugins")) {
        fprintf(stderr, "plugins folder does not exist -- did you build the bindings?\n");
        return;
    }
    QStringList paths = qApp->libraryPaths();
    paths <<  dir.absolutePath();
    qApp->setLibraryPaths(paths);

    SDEBUG("ScriptEngine::runInferior()");
    QStringList extensions;
    extensions << "qt.core"
               << "qt.gui"
               << "qt.xml"
               << "qt.svg"
               << "qt.network"
               << "qt.sql"
               << "qt.opengl"
               << "qt.webkit"
               << "qt.xmlpatterns"
               << "qt.uitools";
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
                     qPrintable(failExtensions.join(", ")), qPrintable(dir.absolutePath()));
        }
    }

    QScriptValue result = m_scriptEngine->evaluate(m_scriptContents, m_scriptFileName);
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
    BreakHandler *handler = qq->breakHandler();
    bool updateNeeded = false;
    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        if (data->pending) {
            data->pending = false; // FIXME
            updateNeeded = true;
        }
        if (data->bpNumber.isEmpty()) {
            data->bpNumber = QString::number(index + 1);
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

void ScriptEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    Q_UNUSED(mousePos)
    Q_UNUSED(editor)
    Q_UNUSED(cursorPos)

    if (q->status() != DebuggerInferiorStopped) {
        //SDEBUG("SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED");
        return;
    }
    // Check mime type and get expression (borrowing some C++ - functions)
    const QString javaScriptMimeType = QLatin1String(QtScriptEditor::Constants::C_QTSCRIPTEDITOR_MIMETYPE);
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
    if (exp.isEmpty() || exp.startsWith("#"))  {
        QToolTip::hideText();
        return;
    }

    if (!hasLetterOrNumber(exp)) {
        QToolTip::showText(m_toolTipPos, tr("'%1' contains no identifier").arg(exp));
        return;
    }

    if (exp.startsWith('"') && exp.endsWith('"'))  {
        QToolTip::showText(m_toolTipPos, tr("String literal %1").arg(exp));
        return;
    }

    if (exp.startsWith("++") || exp.startsWith("--"))
        exp = exp.mid(2);

    if (exp.endsWith("++") || exp.endsWith("--"))
        exp = exp.mid(2);

    if (exp.startsWith("<") || exp.startsWith("["))
        return;

    if (hasSideEffects(exp)) {
        QToolTip::showText(m_toolTipPos,
            tr("Cowardly refusing to evaluate expression '%1' "
               "with potential side effects").arg(exp));
        return;
    }

#if 0
    //if (m_manager->status() != DebuggerInferiorStopped)
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
    XSDEBUG("ASSIGNING: " << expression + '=' + value);
    m_scriptEngine->evaluate(expression + '=' + value);
    updateLocals();
}

void ScriptEngine::maybeBreakNow(bool byFunction)
{
    QScriptContext *context = m_scriptEngine->currentContext();
    QScriptContextInfo info(context);

    //
    // Update breakpoints
    //
    QString functionName = info.functionName();
    QString fileName = info.fileName();
    int lineNumber = info.lineNumber();
    if (byFunction)
        lineNumber = info.functionStartLineNumber();

    BreakHandler *handler = qq->breakHandler();

    if (m_stopOnNextLine) {
        m_stopOnNextLine = false;
    } else {
        int index = 0;
        for (; index != handler->size(); ++index) {
            BreakpointData *data = handler->at(index);
            if (byFunction) {
                if (!functionName.isEmpty() && data->funcName == functionName)
                    break;
            } else {
                if (info.lineNumber() == data->lineNumber.toInt()
                        && fileName == data->fileName)
                    break;
            }
        }

        if (index == handler->size())
            return;

        // we just run into a breakpoint
        //SDEBUG("RESOLVING BREAKPOINT AT " << fileName << lineNumber);
        BreakpointData *data = handler->at(index);
        data->bpLineNumber = QString::number(lineNumber);
        data->bpFileName = fileName;
        data->bpFuncName = functionName;
        data->markerLineNumber = lineNumber;
        data->markerFileName = fileName;
        data->pending = false;
        data->updateMarker();
    }

    qq->notifyInferiorStopped();
    StackFrame frame;
    frame.file = fileName;      
    frame.line = lineNumber;
    q->gotoLocation(frame, true);
    updateLocals();
}

void ScriptEngine::updateLocals()
{
    QScriptContext *context = m_scriptEngine->currentContext();
    qq->watchHandler()->beginCycle();
    //SDEBUG("UPDATE LOCALS");

    //
    // Build stack
    //  
    QList<StackFrame> stackFrames;
    int i = 0;
    for (QScriptContext *c = context; c; c = c->parentContext(), ++i) {
        QScriptContextInfo info(c);
        StackFrame frame;
        frame.level = i;
        frame.file = info.fileName();
        frame.function = info.functionName();
        frame.from = QString::number(info.functionStartLineNumber());
        frame.to = QString::number(info.functionEndLineNumber());
        frame.line = info.lineNumber();
    
        if (frame.function.isEmpty())
            frame.function = "<global scope>";
        //frame.address = ...;
        stackFrames.append(frame);
    }
    qq->stackHandler()->setFrames(stackFrames);

    //
    // Build locals
    //
    WatchData data;
    data.iname = "local";
    data.name = "local";
    data.scriptValue = context->activationObject();
    qq->watchHandler()->beginCycle();
    updateSubItem(data);
    qq->watchHandler()->endCycle();

    // FIXME: Use an extra thread. This here is evil
    m_stopped = true;
    q->showStatusMessage(tr("Stopped."), 5000);
    while (m_stopped) {
        //SDEBUG("LOOPING");
        QApplication::processEvents();
    }
    //SDEBUG("RUNNING AGAIN");
}

void ScriptEngine::updateWatchData(const WatchData &data)
{
    updateSubItem(data);
    //qq->watchHandler()->rebuildModel();
}

void ScriptEngine::updateSubItem(const WatchData &data0)
{
    WatchData data = data0;
    //SDEBUG("\nUPDATE SUBITEM: " << data.toString());
    QTC_ASSERT(data.isValid(), return);

    if (data.isTypeNeeded() || data.isValueNeeded()) {
        QScriptValue ob = data.scriptValue;
        if (ob.isArray()) {
            data.setType("Array");
            data.setValue(" ");
        } else if (ob.isBool()) {
            data.setType("Bool");
            data.setValue(ob.toBool() ? "true" : "false");
            data.setHasChildren(false);
        } else if (ob.isDate()) {
            data.setType("Date");
            data.setValue(ob.toDateTime().toString().toUtf8());
            data.setHasChildren(false);
        } else if (ob.isError()) {
            data.setType("Error");
            data.setValue(" ");
        } else if (ob.isFunction()) {
            data.setType("Function");
            data.setValue(" ");
        } else if (ob.isNull()) {
            data.setType("<null>");
            data.setValue("<null>");
        } else if (ob.isNumber()) {
            data.setType("Number");
            data.setValue(QString::number(ob.toNumber()).toUtf8());
            data.setHasChildren(false);
        } else if (ob.isObject()) {
            data.setType("Object");
            data.setValue(" ");
        } else if (ob.isQMetaObject()) {
            data.setType("QMetaObject");
            data.setValue(" ");
        } else if (ob.isQObject()) {
            data.setType("QObject");
            data.setValue(" ");
        } else if (ob.isRegExp()) {
            data.setType("RegExp");
            data.setValue(ob.toRegExp().pattern().toUtf8());
        } else if (ob.isString()) {
            data.setType("String");
            data.setValue(ob.toString().toUtf8());
        } else if (ob.isVariant()) {
            data.setType("Variant");
            data.setValue(" ");
        } else if (ob.isUndefined()) {
            data.setType("<undefined>");
            data.setValue("<unknown>");
            data.setHasChildren(false);
        } else {
            data.setType("<unknown>");
            data.setValue("<unknown>");
            data.setHasChildren(false);
        }
        qq->watchHandler()->insertData(data);
        return;
    }

    if (data.isChildrenNeeded()) {
        int numChild = 0;
        QScriptValueIterator it(data.scriptValue);
        while (it.hasNext()) {
            it.next();
            WatchData data1;
            data1.iname = data.iname + "." + it.name();
            data1.exp = it.name();
            data1.name = it.name();
            data1.scriptValue = it.value();
            if (qq->watchHandler()->isExpandedIName(data1.iname))
                data1.setChildrenNeeded();
            else
                data1.setChildrenUnneeded();
            qq->watchHandler()->insertData(data1);
            ++numChild;
        }
        //SDEBUG("  ... CHILDREN: " << numChild);
        data.setHasChildren(numChild > 0);
        data.setChildrenUnneeded();
        qq->watchHandler()->insertData(data);
        return;
    }

    if (data.isHasChildrenNeeded()) {
        int numChild = 0;
        QScriptValueIterator it(data.scriptValue);
        while (it.hasNext()) {
            it.next();
            ++numChild;
        }
        data.setHasChildren(numChild > 0);
        //SDEBUG("  ... CHILDCOUNT: " << numChild);
        qq->watchHandler()->insertData(data);
        return;
    }

    QTC_ASSERT(false, return);
}

IDebuggerEngine *createScriptEngine(DebuggerManager *parent)
{
    return new ScriptEngine(parent);
}

} // namespace Internal
} // namespace Debugger
