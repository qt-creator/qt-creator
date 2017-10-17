/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlengine.h"

#include "interactiveinterpreter.h"
#include "qmlinspectoragent.h"
#include "qmlv8debuggerclientconstants.h"
#include "qmlengineutils.h"

#include <debugger/breakhandler.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/sourcefileshandler.h>
#include <debugger/stackhandler.h>
#include <debugger/threaddata.h>
#include <debugger/watchhandler.h>
#include <debugger/watchwindow.h>
#include <debugger/console/console.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/runnables.h>

#include <qmljseditor/qmljseditorconstants.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmldebug/qmldebugconnection.h>
#include <qmldebug/qpacketprotocol.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <app/app_version.h>
#include <utils/treemodel.h>
#include <utils/basetreeview.h>
#include <utils/qtcassert.h>
#include <utils/qtcfallthrough.h>

#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTimer>

#include <memory.h>

#define DEBUG_QML 0
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

#define CB(callback) [this](const QVariantMap &r) { callback(r); }

using namespace Core;
using namespace ProjectExplorer;
using namespace QmlDebug;
using namespace QmlJS;
using namespace TextEditor;
using namespace Utils;

namespace Debugger {
namespace Internal {

enum Exceptions
{
    NoExceptions,
    UncaughtExceptions,
    AllExceptions
};

enum StepAction
{
    Continue,
    StepIn,
    StepOut,
    Next
};

struct QmlV8ObjectData
{
    int handle = -1;
    int expectedProperties = -1;
    QString name;
    QString type;
    QVariant value;
    QVariantList properties;

    bool hasChildren() const
    {
        return expectedProperties > 0 || !properties.isEmpty();
    }
};

typedef std::function<void(const QVariantMap &)> QmlCallback;

struct LookupData
{
    QString iname;
    QString name;
    QString exp;
};

typedef QHash<int, LookupData> LookupItems; // id -> (iname, exp)

class QmlEnginePrivate : public QmlDebugClient
{
public:
    QmlEnginePrivate(QmlEngine *engine_, QmlDebugConnection *connection)
        : QmlDebugClient("V8Debugger", connection),
          engine(engine_),
          inspectorAgent(engine, connection)
    {}

    void messageReceived(const QByteArray &data) override;
    void stateChanged(State state) override;

    void continueDebugging(StepAction stepAction);

    void evaluate(const QString expr, qint64 context, const QmlCallback &cb);
    void lookup(const LookupItems &items);
    void backtrace();
    void updateLocals();
    void scope(int number, int frameNumber = -1);
    void scripts(int types = 4, const QList<int> ids = QList<int>(),
                 bool includeSource = false, const QVariant filter = QVariant());

    void setBreakpoint(const QString type, const QString target,
                       bool enabled = true,int line = 0, int column = 0,
                       const QString condition = QString(), int ignoreCount = -1);
    void clearBreakpoint(int breakpoint);
    void setExceptionBreak(Exceptions type, bool enabled = false);

    void flushSendBuffer();

    void handleBacktrace(const QVariantMap &response);
    void handleLookup(const QVariantMap &response);
    void handleExecuteDebuggerCommand(const QVariantMap &response);
    void handleEvaluateExpression(const QVariantMap &response, const QString &iname, const QString &expr);
    void handleFrame(const QVariantMap &response);
    void handleScope(const QVariantMap &response);
    void handleVersion(const QVariantMap &response);
    StackFrame extractStackFrame(const QVariant &bodyVal);

    bool canEvaluateScript(const QString &script);
    void updateScriptSource(const QString &fileName, int lineOffset, int columnOffset, const QString &source);

    void runCommand(const DebuggerCommand &command, const QmlCallback &cb = QmlCallback());
    void runDirectCommand(const QString &type, const QByteArray &msg = QByteArray());

    void clearRefs() { refVals.clear(); }
    void memorizeRefs(const QVariant &refs);
    QmlV8ObjectData extractData(const QVariant &data) const;
    void insertSubItems(WatchItem *parent, const QVariantList &properties);
    void checkForFinishedUpdate();
    ConsoleItem *constructLogItemTree(const QmlV8ObjectData &objectData);

public:
    QHash<int, QmlV8ObjectData> refVals; // The mapping of target object handles to retrieved values.
    int sequence = -1;
    QmlEngine *engine;
    QHash<BreakpointModelId, int> breakpoints;
    QHash<int, BreakpointModelId> breakpointsSync;
    QList<int> breakpointsTemp;

    LookupItems currentlyLookingUp; // Id -> inames

    //Cache
    QList<int> currentFrameScopes;
    QHash<int, int> stackIndexLookup;

    StepAction previousStepAction = Continue;

    QList<QByteArray> sendBuffer;

    QHash<QString, QTextDocument*> sourceDocuments;
    QHash<QString, QWeakPointer<BaseTextEditor> > sourceEditors;
    InteractiveInterpreter interpreter;
    ApplicationLauncher applicationLauncher;
    QmlInspectorAgent inspectorAgent;

    QList<quint32> queryIds;
    bool retryOnConnectFail = false;
    bool automaticConnect = false;
    bool unpausedEvaluate = false;
    bool contextEvaluate = false;

    QTimer connectionTimer;
    QmlDebug::QDebugMessageClient *msgClient = 0;

    QHash<int, QmlCallback> callbackForToken;
    QMetaObject::Connection startupMessageFilterConnection;

private:
    ConsoleItem *constructLogItemTree(const QmlV8ObjectData &objectData, QList<int> &seenHandles);
    void constructChildLogItems(ConsoleItem *item, const QmlV8ObjectData &objectData,
                             QList<int> &seenHandles);
};

static void updateDocument(IDocument *document, const QTextDocument *textDocument)
{
    if (auto baseTextDocument = qobject_cast<TextDocument *>(document))
        baseTextDocument->document()->setPlainText(textDocument->toPlainText());
}


///////////////////////////////////////////////////////////////////////
//
// QmlEngine
//
///////////////////////////////////////////////////////////////////////

QmlEngine::QmlEngine()
  :  d(new QmlEnginePrivate(this, new QmlDebugConnection(this)))
{
    setObjectName("QmlEngine");
    QmlDebugConnection *connection = d->connection();

    connect(stackHandler(), &StackHandler::stackChanged,
            this, &QmlEngine::updateCurrentContext);
    connect(stackHandler(), &StackHandler::currentIndexChanged,
            this, &QmlEngine::updateCurrentContext);
    connect(inspectorView(), &WatchTreeView::currentIndexChanged,
            this, &QmlEngine::updateCurrentContext);

    connect(&d->applicationLauncher, &ApplicationLauncher::processExited,
            this, &QmlEngine::disconnected);
    connect(&d->applicationLauncher, &ApplicationLauncher::appendMessage,
            this, &QmlEngine::appMessage);
    connect(&d->applicationLauncher, &ApplicationLauncher::processStarted,
            this, &QmlEngine::handleLauncherStarted);

    debuggerConsole()->setScriptEvaluator([this](const QString &expr) {
        executeDebuggerCommand(expr, QmlLanguage);
    });

    d->connectionTimer.setInterval(4000);
    d->connectionTimer.setSingleShot(true);
    connect(&d->connectionTimer, &QTimer::timeout,
            this, &QmlEngine::checkConnectionState);

    connect(connection, &QmlDebugConnection::logStateChange,
            this, &QmlEngine::showConnectionStateMessage);
    connect(connection, &QmlDebugConnection::logError, this,
            [this](const QString &error) { showMessage("QML Debugger: " + error, LogWarning); });

    connect(connection, &QmlDebugConnection::connectionFailed,
            this, &QmlEngine::connectionFailed);
    connect(connection, &QmlDebugConnection::connected,
            &d->connectionTimer, &QTimer::stop);
    connect(connection, &QmlDebugConnection::connected,
            this, &QmlEngine::connectionEstablished);
    connect(connection, &QmlDebugConnection::disconnected,
            this, &QmlEngine::disconnected);

    d->msgClient = new QDebugMessageClient(connection);
    connect(d->msgClient, &QDebugMessageClient::newState,
            this, [this](QmlDebugClient::State state) {
        logServiceStateChange(d->msgClient->name(), d->msgClient->serviceVersion(), state);
    });

    connect(d->msgClient, &QDebugMessageClient::message,
            this, &appendDebugOutput);
}

QmlEngine::~QmlEngine()
{
    QObject::disconnect(d->startupMessageFilterConnection);
    QSet<IDocument *> documentsToClose;

    QHash<QString, QWeakPointer<BaseTextEditor> >::iterator iter;
    for (iter = d->sourceEditors.begin(); iter != d->sourceEditors.end(); ++iter) {
        QWeakPointer<BaseTextEditor> textEditPtr = iter.value();
        if (textEditPtr)
            documentsToClose << textEditPtr.data()->document();
    }
    EditorManager::closeDocuments(documentsToClose.toList());

    delete d;
}

void QmlEngine::setState(DebuggerState state, bool forced)
{
    DebuggerEngine::setState(state, forced);
    updateCurrentContext();
}

void QmlEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    notifyInferiorSetupOk();

    if (d->automaticConnect)
        beginConnection();
}

void QmlEngine::handleLauncherStarted()
{
    // FIXME: The QmlEngine never calls notifyInferiorPid() triggering the
    // raising, so do it here manually for now.
    runTool()->runControl()->applicationProcessHandle().activate();
    tryToConnect();
}

void QmlEngine::appMessage(const QString &msg, Utils::OutputFormat /* format */)
{
    showMessage(msg, AppOutput); // FIXME: Redirect to RunControl
}

void QmlEngine::connectionEstablished()
{
    attemptBreakpointSynchronization();

    if (state() == EngineRunRequested)
        notifyEngineRunAndInferiorRunOk();
}

void QmlEngine::tryToConnect()
{
    showMessage("QML Debugger: Trying to connect ...", LogStatus);
    d->retryOnConnectFail = true;
    if (state() == EngineRunRequested) {
        if (isSlaveEngine()) {
            // Probably cpp is being debugged and hence we did not get the output yet.
            if (!masterEngine()->isDying())
                beginConnection();
            else
                appStartupFailed(tr("No application output received in time"));
        } else {
            beginConnection();
        }
    } else {
        d->automaticConnect = true;
    }
}

void QmlEngine::beginConnection()
{
    if (state() != EngineRunRequested && d->retryOnConnectFail)
        return;

    QTC_ASSERT(state() == EngineRunRequested, return);

    QObject::disconnect(d->startupMessageFilterConnection);

    QString host = runParameters().qmlServer.host();
    // Use localhost as default
    if (host.isEmpty())
        host = QHostAddress(QHostAddress::LocalHost).toString();

    // FIXME: Not needed?
    /*
     * Let plugin-specific code override the port printed by the application. This is necessary
     * in the case of port forwarding, when the port the application listens on is not the same that
     * we want to connect to.
     * NOTE: It is still necessary to wait for the output in that case, because otherwise we cannot
     * be sure that the port is already open. The usual method of trying to connect repeatedly
     * will not work, because the intermediate port is already open. So the connection
     * will be accepted on that port but the forwarding to the target port will fail and
     * the connection will be closed again (instead of returning the "connection refused"
     * error that we expect).
     */
    int port = runParameters().qmlServer.port();

    QmlDebugConnection *connection = d->connection();
    if (!connection || connection->isConnected())
        return;

    connection->connectToHost(host, port);

    //A timeout to check the connection state
    d->connectionTimer.start();
}

void QmlEngine::connectionStartupFailed()
{
    if (d->retryOnConnectFail) {
        // retry after 3 seconds ...
        QTimer::singleShot(3000, this, [this] { beginConnection(); });
        return;
    }

    QMessageBox *infoBox = new QMessageBox(ICore::mainWindow());
    infoBox->setIcon(QMessageBox::Critical);
    infoBox->setWindowTitle(Core::Constants::IDE_DISPLAY_NAME);
    infoBox->setText(tr("Could not connect to the in-process QML debugger."
                        "\nDo you want to retry?"));
    infoBox->setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel |
                                QMessageBox::Help);
    infoBox->setDefaultButton(QMessageBox::Retry);
    infoBox->setModal(true);

    connect(infoBox, &QDialog::finished,
            this, &QmlEngine::errorMessageBoxFinished);

    infoBox->show();
}

void QmlEngine::appStartupFailed(const QString &errorMessage)
{
    QString error = tr("Could not connect to the in-process QML debugger. %1").arg(errorMessage);

    if (isMasterEngine()) {
        QMessageBox *infoBox = new QMessageBox(ICore::mainWindow());
        infoBox->setIcon(QMessageBox::Critical);
        infoBox->setWindowTitle(Core::Constants::IDE_DISPLAY_NAME);
        infoBox->setText(error);
        infoBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
        infoBox->setDefaultButton(QMessageBox::Ok);
        connect(infoBox, &QDialog::finished,
                this, &QmlEngine::errorMessageBoxFinished);
        infoBox->show();
    } else {
        debuggerConsole()->printItem(ConsoleItem::WarningType, error);
    }

    notifyEngineRunFailed();
}

void QmlEngine::errorMessageBoxFinished(int result)
{
    switch (result) {
    case QMessageBox::Retry: {
        beginConnection();
        break;
    }
    case QMessageBox::Help: {
        HelpManager::handleHelpRequest("qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html");
        Q_FALLTHROUGH();
    }
    default:
        if (state() == InferiorRunOk) {
            notifyInferiorSpontaneousStop();
            notifyInferiorIll();
        } else if (state() == EngineRunRequested) {
            notifyEngineRunFailed();
        }
        break;
    }
}

void QmlEngine::gotoLocation(const Location &location)
{
    const QString fileName = location.fileName();
    if (QUrl(fileName).isLocalFile()) {
        // internal file from source files -> show generated .js
        QTC_ASSERT(d->sourceDocuments.contains(fileName), return);

        QString titlePattern = tr("JS Source for %1").arg(fileName);
        //Check if there are open documents with the same title
        foreach (IDocument *document, DocumentModel::openedDocuments()) {
            if (document->displayName() == titlePattern) {
                EditorManager::activateEditorForDocument(document);
                return;
            }
        }
        IEditor *editor = EditorManager::openEditorWithContents(
                    QmlJSEditor::Constants::C_QMLJSEDITOR_ID, &titlePattern);
        if (editor) {
            editor->document()->setProperty(Constants::OPENED_BY_DEBUGGER, true);
            if (auto plainTextEdit = qobject_cast<QPlainTextEdit *>(editor->widget()))
                plainTextEdit->setReadOnly(true);
            updateDocument(editor->document(), d->sourceDocuments.value(fileName));
        }
    } else {
        DebuggerEngine::gotoLocation(location);
    }
}

void QmlEngine::closeConnection()
{
    if (d->connectionTimer.isActive()) {
        d->connectionTimer.stop();
    } else {
        if (QmlDebugConnection *connection = d->connection())
            connection->close();
    }
}

void QmlEngine::runEngine()
{
    // we won't get any debug output
    if (!terminal()) {
        d->retryOnConnectFail = true;
        d->automaticConnect = true;
    }

    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    if (!isSlaveEngine()) {
        if (runParameters().startMode == AttachToRemoteServer)
            tryToConnect();
        else if (runParameters().startMode == AttachToRemoteProcess)
            beginConnection();
        else
            startApplicationLauncher();
    } else {
        tryToConnect();
    }
}

void QmlEngine::startApplicationLauncher()
{
    if (!d->applicationLauncher.isRunning()) {
        StandardRunnable runnable = runParameters().inferior;
        runTool()->appendMessage(tr("Starting %1 %2").arg(
                                     QDir::toNativeSeparators(runnable.executable),
                                     runnable.commandLineArguments),
                                 Utils::NormalMessageFormat);
        d->applicationLauncher.start(runnable);
    }
}

void QmlEngine::stopApplicationLauncher()
{
    if (d->applicationLauncher.isRunning()) {
        disconnect(&d->applicationLauncher, &ApplicationLauncher::processExited,
                   this, &QmlEngine::disconnected);
        d->applicationLauncher.stop();
    }
}

void QmlEngine::shutdownInferior()
{
    // End session.
    //    { "seq"     : <number>,
    //      "type"    : "request",
    //      "command" : "disconnect",
    //    }
    d->runCommand({DISCONNECT});

    if (isSlaveEngine())
        resetLocation();
    stopApplicationLauncher();
    closeConnection();

    notifyInferiorShutdownOk();
}

void QmlEngine::shutdownEngine()
{
    clearExceptionSelection();

    debuggerConsole()->setScriptEvaluator(ScriptEvaluator());

   // double check (ill engine?):
    stopApplicationLauncher();

    notifyEngineShutdownOk();
    if (!isSlaveEngine())
        showMessage(QString(), StatusBar);
}

void QmlEngine::setupEngine()
{
    notifyEngineSetupOk();
}

void QmlEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    clearExceptionSelection();
    d->continueDebugging(Continue);
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::interruptInferior()
{
    showMessage(INTERRUPT, LogInput);
    d->runDirectCommand(INTERRUPT);
    showStatusMessage(tr("Waiting for JavaScript engine to interrupt on next statement."));
}

void QmlEngine::executeStep()
{
    clearExceptionSelection();
    d->continueDebugging(StepIn);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepI()
{
    clearExceptionSelection();
    d->continueDebugging(StepIn);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepOut()
{
    clearExceptionSelection();
    d->continueDebugging(StepOut);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNext()
{
    clearExceptionSelection();
    d->continueDebugging(Next);
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNextI()
{
    executeNext();
}

void QmlEngine::executeRunToLine(const ContextData &data)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    showStatusMessage(tr("Run to line %1 (%2) requested...").arg(data.lineNumber).arg(data.fileName), 5000);
    d->setBreakpoint(SCRIPTREGEXP, data.fileName, true, data.lineNumber);
    clearExceptionSelection();
    d->continueDebugging(Continue);

    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  QmlEngine::executeRunToFunction()");
}

void QmlEngine::executeJumpToLine(const ContextData &data)
{
    Q_UNUSED(data)
    XSDEBUG("FIXME:  QmlEngine::executeJumpToLine()");
}

void QmlEngine::activateFrame(int index)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    stackHandler()->setCurrentIndex(index);
    gotoLocation(stackHandler()->frames().value(index));

    d->updateLocals();
}

void QmlEngine::selectThread(ThreadId threadId)
{
    Q_UNUSED(threadId)
}

void QmlEngine::insertBreakpoint(Breakpoint bp)
{
    BreakpointState state = bp.state();
    QTC_ASSERT(state == BreakpointInsertRequested, qDebug() << bp << this << state);
    bp.notifyBreakpointInsertProceeding();

    const BreakpointParameters &params = bp.parameters();
    if (params.type == BreakpointAtJavaScriptThrow) {
        BreakpointResponse br = bp.response();
        br.pending = false;
        bp.setResponse(br);
        bp.notifyBreakpointInsertOk();
        d->setExceptionBreak(AllExceptions, params.enabled);

    } else if (params.type == BreakpointByFileAndLine) {
        d->setBreakpoint(SCRIPTREGEXP, params.fileName,
                         params.enabled, params.lineNumber, 0,
                         params.condition, params.ignoreCount);

    } else if (params.type == BreakpointOnQmlSignalEmit) {
        d->setBreakpoint(EVENT, params.functionName, params.enabled);
        BreakpointResponse br = bp.response();
        br.pending = false;
        bp.setResponse(br);
        bp.notifyBreakpointInsertOk();
    }

    d->breakpointsSync.insert(d->sequence, bp.id());
}

void QmlEngine::resetLocation()
{
    DebuggerEngine::resetLocation();
    d->currentlyLookingUp.clear();
}

void QmlEngine::removeBreakpoint(Breakpoint bp)
{
    const BreakpointParameters &params = bp.parameters();

    BreakpointState state = bp.state();
    QTC_ASSERT(state == BreakpointRemoveRequested, qDebug() << bp << this << state);
    bp.notifyBreakpointRemoveProceeding();

    int breakpoint = d->breakpoints.value(bp.id());
    d->breakpoints.remove(bp.id());

    if (params.type == BreakpointAtJavaScriptThrow)
        d->setExceptionBreak(AllExceptions);
    else if (params.type == BreakpointOnQmlSignalEmit)
        d->setBreakpoint(EVENT, params.functionName, false);
    else
        d->clearBreakpoint(breakpoint);

    if (bp.state() == BreakpointRemoveProceeding)
        bp.notifyBreakpointRemoveOk();
}

void QmlEngine::changeBreakpoint(Breakpoint bp)
{
    BreakpointState state = bp.state();
    QTC_ASSERT(state == BreakpointChangeRequested, qDebug() << bp << this << state);
    bp.notifyBreakpointChangeProceeding();

    const BreakpointParameters &params = bp.parameters();

    BreakpointResponse br = bp.response();
    if (params.type == BreakpointAtJavaScriptThrow) {
        d->setExceptionBreak(AllExceptions, params.enabled);
        br.enabled = params.enabled;
        bp.setResponse(br);
    } else if (params.type == BreakpointOnQmlSignalEmit) {
        d->setBreakpoint(EVENT, params.functionName, params.enabled);
        br.enabled = params.enabled;
        bp.setResponse(br);
    } else {
        //V8 supports only minimalistic changes in breakpoint
        //Remove the breakpoint and add again
        bp.notifyBreakpointChangeOk();
        bp.removeBreakpoint();
        BreakHandler *handler = d->engine->breakHandler();
        handler->appendBreakpoint(params);
    }

    if (bp.state() == BreakpointChangeProceeding)
        bp.notifyBreakpointChangeOk();
}

void QmlEngine::attemptBreakpointSynchronization()
{
    if (!stateAcceptsBreakpointChanges()) {
        showMessage("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE");
        return;
    }

    BreakHandler *handler = breakHandler();

    DebuggerEngine *bpOwner = isSlaveEngine() ? masterEngine() : this;
    foreach (Breakpoint bp, handler->unclaimedBreakpoints()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(bp))
            bp.setEngine(bpOwner);
    }

    foreach (Breakpoint bp, handler->engineBreakpoints(bpOwner)) {
        switch (bp.state()) {
        case BreakpointNew:
            // Should not happen once claimed.
            QTC_CHECK(false);
            continue;
        case BreakpointInsertRequested:
            insertBreakpoint(bp);
            continue;
        case BreakpointChangeRequested:
            changeBreakpoint(bp);
            continue;
        case BreakpointRemoveRequested:
            removeBreakpoint(bp);
            continue;
        case BreakpointChangeProceeding:
        case BreakpointInsertProceeding:
        case BreakpointRemoveProceeding:
        case BreakpointInserted:
        case BreakpointDead:
            continue;
        }
        QTC_ASSERT(false, qDebug() << "UNKNOWN STATE"  << bp << state());
    }

    DebuggerEngine::attemptBreakpointSynchronization();
}

bool QmlEngine::acceptsBreakpoint(Breakpoint bp) const
{
    if (!bp.parameters().isCppBreakpoint())
            return true;

    //If it is a Cpp Breakpoint query if the type can be also handled by the debugger client
    //TODO: enable setting of breakpoints before start of debug session
    //For now, the event breakpoint can be set after the activeDebuggerClient is known
    //This is because the older client does not support BreakpointOnQmlSignalHandler
    BreakpointType type = bp.type();
    return type == BreakpointOnQmlSignalEmit
            || type == BreakpointByFileAndLine
            || type == BreakpointAtJavaScriptThrow;
}

void QmlEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void QmlEngine::loadAllSymbols()
{
}

void QmlEngine::reloadModules()
{
}

void QmlEngine::reloadSourceFiles()
{
    d->scripts(4, QList<int>(), true, QVariant());
}

void QmlEngine::updateAll()
{
    d->updateLocals();
}

void QmlEngine::requestModuleSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

bool QmlEngine::canHandleToolTip(const DebuggerToolTipContext &) const
{
    // This is processed by QML inspector, which has dependencies to
    // the qml js editor. Makes life easier.
    // FIXME: Except that there isn't any attached.
    return true;
}

void QmlEngine::assignValueInDebugger(WatchItem *item,
    const QString &expression, const QVariant &editValue)
{
    if (!expression.isEmpty()) {
        QVariant value = (editValue.type() == QVariant::String)
                ? QVariant('"' + editValue.toString().replace('"', "\\\"") + '"')
                : editValue;

        if (item->isInspect()) {
            d->inspectorAgent.assignValue(item, expression, value);
        } else {
            StackHandler *handler = stackHandler();
            QString exp = QString("%1 = %2;").arg(expression).arg(value.toString());
            if (handler->isContentsValid() && handler->currentFrame().isUsable()) {
                d->evaluate(exp, -1, [this](const QVariantMap &) { d->updateLocals(); });
            } else {
                showMessage(tr("Cannot evaluate %1 in current stack frame.")
                            .arg(expression), ConsoleOutput);
            }
        }
    }
}

void QmlEngine::expandItem(const QString &iname)
{
    const WatchItem *item = watchHandler()->findItem(iname);
    QTC_ASSERT(item, return);

    if (item->isInspect()) {
        d->inspectorAgent.updateWatchData(*item);
    } else {
        LookupItems items;
        items.insert(int(item->id), {item->iname, item->name, item->exp});
        d->lookup(items);
    }
}

void QmlEngine::updateItem(const QString &iname)
{
    const WatchItem *item = watchHandler()->findItem(iname);
    QTC_ASSERT(item, return);

    if (state() == InferiorStopOk) {
        // The Qt side Q_ASSERTs otherwise. So postpone the evaluation,
        // it will be triggered from from upateLocals() later.
        QString exp = item->exp;
        d->evaluate(exp, -1, [this, iname, exp](const QVariantMap &response) {
            d->handleEvaluateExpression(response, iname, exp);
        });
    }
}

void QmlEngine::selectWatchData(const QString &iname)
{
    const WatchItem *item = watchHandler()->findItem(iname);
    if (item && item->isInspect())
        d->inspectorAgent.watchDataSelected(item->id);
}

bool compareConsoleItems(const ConsoleItem *a, const ConsoleItem *b)
{
    if (a == 0)
        return true;
    if (b == 0)
        return false;
    return a->text() < b->text();
}

static ConsoleItem *constructLogItemTree(const QVariant &result,
                                         const QString &key = QString())
{
    bool sorted = boolSetting(SortStructMembers);
    if (!result.isValid())
        return 0;

    QString text;
    ConsoleItem *item = 0;
    if (result.type() == QVariant::Map) {
        if (key.isEmpty())
            text = "Object";
        else
            text = key + " : Object";

        QMap<QString, QVariant> resultMap = result.toMap();
        QVarLengthArray<ConsoleItem *> children(resultMap.size());
        QMapIterator<QString, QVariant> i(result.toMap());
        auto it = children.begin();
        while (i.hasNext()) {
            i.next();
            *(it++) = constructLogItemTree(i.value(), i.key());
        }

        // Sort before inserting as ConsoleItem::sortChildren causes a whole cascade of changes we
        // may not want to handle here.
        if (sorted)
            std::sort(children.begin(), children.end(), compareConsoleItems);

        item = new ConsoleItem(ConsoleItem::DefaultType, text);
        foreach (ConsoleItem *child, children) {
            if (child)
                item->appendChild(child);
        }

    } else if (result.type() == QVariant::List) {
        if (key.isEmpty())
            text = "List";
        else
            text = QString("[%1] : List").arg(key);

        QVariantList resultList = result.toList();
        QVarLengthArray<ConsoleItem *> children(resultList.size());
        for (int i = 0; i < resultList.count(); i++)
            children[i] = constructLogItemTree(resultList.at(i), QString::number(i));

        if (sorted)
            std::sort(children.begin(), children.end(), compareConsoleItems);

        item = new ConsoleItem(ConsoleItem::DefaultType, text);
        foreach (ConsoleItem *child, children) {
            if (child)
                item->appendChild(child);
        }
    } else if (result.canConvert(QVariant::String)) {
        item = new ConsoleItem(ConsoleItem::DefaultType, result.toString());
    } else {
        item = new ConsoleItem(ConsoleItem::DefaultType, "Unknown Value");
    }

    return item;
}

void QmlEngine::expressionEvaluated(quint32 queryId, const QVariant &result)
{
    if (d->queryIds.contains(queryId)) {
        d->queryIds.removeOne(queryId);
        if (ConsoleItem *item = constructLogItemTree(result))
            debuggerConsole()->printItem(item);
    }
}

bool QmlEngine::hasCapability(unsigned cap) const
{
    return cap & (AddWatcherCapability
            | AddWatcherWhileRunningCapability
            | RunToLineCapability);
    /*ReverseSteppingCapability | SnapshotCapability
        | AutoDerefPointersCapability | DisassemblerCapability
        | RegisterCapability | ShowMemoryCapability
        | JumpToLineCapability | ReloadModuleCapability
        | ReloadModuleSymbolsCapability | BreakOnThrowAndCatchCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointCapability
        | AddWatcherCapability;*/
}

void QmlEngine::quitDebugger()
{
    d->automaticConnect = false;
    d->retryOnConnectFail = false;
    shutdownInferior();
}

void QmlEngine::doUpdateLocals(const UpdateParameters &params)
{
    Q_UNUSED(params);
    d->updateLocals();
}

void QmlEngine::disconnected()
{
    showMessage(tr("QML Debugger disconnected."), StatusBar);
    notifyInferiorExited();
}

void QmlEngine::updateCurrentContext()
{
    d->inspectorAgent.enableTools(state() == InferiorRunOk);

    QString context;
    switch (state()) {
    case InferiorStopOk:
        context = stackHandler()->currentFrame().function;
        break;
    case InferiorRunOk:
        if (d->contextEvaluate || !d->unpausedEvaluate) {
            // !unpausedEvaluate means we are using the old QQmlEngineDebugService which understands
            // context. contextEvaluate means the V4 debug service can handle context.
            QModelIndex currentIndex = inspectorView()->currentIndex();
            const WatchItem *currentData = watchHandler()->watchItem(currentIndex);
            if (!currentData)
                return;
            const WatchItem *parentData = watchHandler()->watchItem(currentIndex.parent());
            const WatchItem *grandParentData = watchHandler()->watchItem(currentIndex.parent().parent());
            if (currentData->id != parentData->id)
                context = currentData->name;
            else if (parentData->id != grandParentData->id)
                context = parentData->name;
            else
                context = grandParentData->name;
        }
        break;
    default:
        // No context. Clear the label
        debuggerConsole()->setContext(QString());
        return;
    }

    debuggerConsole()->setContext(tr("Context:") + QLatin1Char(' ')
                                  + (context.isEmpty() ? tr("Global QML Context") : context));
}

void QmlEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if (!(languages & QmlLanguage))
        return;

    if (state() == InferiorStopOk) {
        StackHandler *handler = stackHandler();
        if (handler->isContentsValid() && handler->currentFrame().isUsable()) {
            d->evaluate(command, -1, CB(d->handleExecuteDebuggerCommand));
        } else {
            // Paused but no stack? Something is wrong
            d->engine->showMessage(QString("Cannot evaluate %1. The stack trace is broken.").arg(command),
                                   ConsoleOutput);
        }
    } else {
        QModelIndex currentIndex = inspectorView()->currentIndex();
        qint64 contextId = watchHandler()->watchItem(currentIndex)->id;

        if (d->unpausedEvaluate) {
            d->evaluate(command, contextId, CB(d->handleExecuteDebuggerCommand));
        } else {
            quint32 queryId = d->inspectorAgent.queryExpressionResult(contextId, command);
            if (queryId) {
                d->queryIds.append(queryId);
            } else {
                d->engine->showMessage("The application has to be stopped in a breakpoint in order to"
                                       " evaluate expressions", ConsoleOutput);
            }
        }
    }
}

void QmlEnginePrivate::updateScriptSource(const QString &fileName, int lineOffset, int columnOffset,
                                          const QString &source)
{
    QTextDocument *document = 0;
    if (sourceDocuments.contains(fileName)) {
        document = sourceDocuments.value(fileName);
    } else {
        document = new QTextDocument(this);
        sourceDocuments.insert(fileName, document);
    }

    // We're getting an unordered set of snippets that can even interleave
    // Therefore we've to carefully update the existing document

    QTextCursor cursor(document);
    for (int i = 0; i < lineOffset; ++i) {
        if (!cursor.movePosition(QTextCursor::NextBlock))
            cursor.insertBlock();
    }
    QTC_CHECK(cursor.blockNumber() == lineOffset);

    for (int i = 0; i < columnOffset; ++i) {
        if (!cursor.movePosition(QTextCursor::NextCharacter))
            cursor.insertText(" ");
    }
    QTC_CHECK(cursor.positionInBlock() == columnOffset);

    QStringList lines = source.split('\n');
    foreach (QString line, lines) {
        if (line.endsWith('\r'))
            line.remove(line.size() -1, 1);

        // line already there?
        QTextCursor existingCursor(cursor);
        existingCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (existingCursor.selectedText() != line)
            cursor.insertText(line);

        if (!cursor.movePosition(QTextCursor::NextBlock))
            cursor.insertBlock();
    }

    //update open editors
    QString titlePattern = QCoreApplication::translate("QmlEngine", "JS Source for %1").arg(fileName);
    //Check if there are open editors with the same title
    foreach (IDocument *doc, DocumentModel::openedDocuments()) {
        if (doc->displayName() == titlePattern) {
            updateDocument(doc, document);
            break;
        }
    }
}

bool QmlEnginePrivate::canEvaluateScript(const QString &script)
{
    interpreter.clearText();
    interpreter.appendText(script);
    return interpreter.canEvaluate();
}

void QmlEngine::connectionFailed()
{
    // this is only an error if we are already connected and something goes wrong.
    if (isConnected()) {
        showMessage(tr("QML Debugger: Connection failed."), StatusBar);

        if (!isSlaveEngine()) { // normal flow for slave engine when gdb exits
            notifyInferiorSpontaneousStop();
            notifyInferiorIll();
        }
    } else {
        d->connectionTimer.stop();
        connectionStartupFailed();
    }
}

void QmlEngine::checkConnectionState()
{
    if (!isConnected()) {
        closeConnection();
        connectionStartupFailed();
    }
}

bool QmlEngine::isConnected() const
{
    if (QmlDebugConnection *connection = d->connection())
        return connection->isConnected();
    else
        return false;
}

void QmlEngine::showConnectionStateMessage(const QString &message)
{
    showMessage("QML Debugger: " + message, LogStatus);
}

void QmlEngine::logServiceStateChange(const QString &service, float version,
                                      QmlDebugClient::State newState)
{
    switch (newState) {
    case QmlDebugClient::Unavailable: {
        showConnectionStateMessage(QString("Status of \"%1\" Version: %2 changed to 'unavailable'.").
                                   arg(service).arg(version));
        break;
    }
    case QmlDebugClient::Enabled: {
        showConnectionStateMessage(QString("Status of \"%1\" Version: %2 changed to 'enabled'.").
                                    arg(service).arg(version));
        break;
    }

    case QmlDebugClient::NotConnected: {
        showConnectionStateMessage(QString("Status of \"%1\" Version: %2 changed to 'not connected'.").
                                    arg(service).arg(version));
        break;
    }
    }
}

void QmlEngine::logServiceActivity(const QString &service, const QString &logMessage)
{
    showMessage(service + ' ' + logMessage, LogDebug);
}

void QmlEnginePrivate::continueDebugging(StepAction action)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "continue",
    //      "arguments" : { "stepaction" : <"in", "next" or "out">,
    //                      "stepcount"  : <number of steps (default 1)>
    //                    }
    //    }

    DebuggerCommand cmd(CONTINEDEBUGGING);

    if (action == StepIn)
        cmd.arg(STEPACTION, IN);
    else if (action == StepOut)
        cmd.arg(STEPACTION, OUT);
    else if (action == Next)
        cmd.arg(STEPACTION, NEXT);

    runCommand(cmd);

    previousStepAction = action;
}

void QmlEnginePrivate::evaluate(const QString expr, qint64 context, const QmlCallback &cb)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "evaluate",
    //      "arguments" : { "expression"    : <expression to evaluate>,
    //                      "frame"         : <number>,
    //                      "global"        : <boolean>,
    //                      "disable_break" : <boolean>,
    //                      "context"       : <object id>
    //                    }
    //    }

    // The Qt side Q_ASSERTs otherwise. So ignore the request and hope
    // it will be repeated soon enough (which it will, e.g. in updateLocals)
    QTC_ASSERT(unpausedEvaluate || engine->state() == InferiorStopOk, return);

    DebuggerCommand cmd(EVALUATE);

    cmd.arg(EXPRESSION, expr);
    StackHandler *handler = engine->stackHandler();
    if (handler->currentFrame().isUsable())
        cmd.arg(FRAME, handler->currentIndex());

    if (context >= 0)
        cmd.arg(CONTEXT, context);

    runCommand(cmd, cb);
}

void QmlEnginePrivate::handleEvaluateExpression(const QVariantMap &response,
                                                const QString &iname,
                                                const QString &exp)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "evaluate",
    //      "body"        : ...
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    QVariant bodyVal = response.value(BODY).toMap();
    QmlV8ObjectData body = extractData(bodyVal);
    WatchHandler *watchHandler = engine->watchHandler();

    auto item = new WatchItem;
    item->iname = iname;
    item->name = exp;
    item->exp = exp;
    item->id = body.handle;
    bool success = response.value("success").toBool();
    if (success) {
        item->type = body.type;
        item->value = body.value.toString();
        item->setHasChildren(body.hasChildren());
    } else {
        //Do not set type since it is unknown
        item->setError(body.value.toString());
    }
    insertSubItems(item, body.properties);
    watchHandler->insertItem(item);
    watchHandler->updateWatchersWindow();
}

void QmlEnginePrivate::lookup(const LookupItems &items)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "lookup",
    //      "arguments" : { "handles"       : <array of handles>,
    //                      "includeSource" : <boolean indicating whether
    //                                          the source will be included when
    //                                          script objects are returned>,
    //                    }
    //    }

    if (items.isEmpty())
        return;

    QList<int> handles;
    for (auto it = items.begin(); it != items.end(); ++it) {
        const int handle = it.key();
        if (!currentlyLookingUp.contains(handle)) {
            currentlyLookingUp.insert(handle, it.value());
            handles.append(handle);
        }
    }

    DebuggerCommand cmd(LOOKUP);
    cmd.arg(HANDLES, handles);
    runCommand(cmd, CB(handleLookup));
}

void QmlEnginePrivate::backtrace()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "backtrace",
    //      "arguments" : { "fromFrame" : <number>
    //                      "toFrame" : <number>
    //                      "bottom" : <boolean, set to true if the bottom of the
    //                          stack is requested>
    //                    }
    //    }

    DebuggerCommand cmd(BACKTRACE);
    runCommand(cmd, CB(handleBacktrace));
}

void QmlEnginePrivate::updateLocals()
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "frame",
    //      "arguments" : { "number" : <frame number> }
    //    }

    DebuggerCommand cmd(FRAME);
    cmd.arg(NUMBER, stackIndexLookup.value(engine->stackHandler()->currentIndex()));
    runCommand(cmd, CB(handleFrame));
}

void QmlEnginePrivate::scope(int number, int frameNumber)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scope",
    //      "arguments" : { "number" : <scope number>
    //                      "frameNumber" : <frame number, optional uses selected
    //                                      frame if missing>
    //                    }
    //    }

    DebuggerCommand cmd(SCOPE);
    cmd.arg(NUMBER, number);
    if (frameNumber != -1)
        cmd.arg(FRAMENUMBER, frameNumber);

    runCommand(cmd, CB(handleScope));
}

void QmlEnginePrivate::scripts(int types, const QList<int> ids, bool includeSource,
                               const QVariant filter)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "scripts",
    //      "arguments" : { "types"         : <types of scripts to retrieve
    //                                           set bit 0 for native scripts
    //                                           set bit 1 for extension scripts
    //                                           set bit 2 for normal scripts
    //                                         (default is 4 for normal scripts)>
    //                      "ids"           : <array of id's of scripts to return. If this is not specified all scripts are requrned>
    //                      "includeSource" : <boolean indicating whether the source code should be included for the scripts returned>
    //                      "filter"        : <string or number: filter string or script id.
    //                                         If a number is specified, then only the script with the same number as its script id will be retrieved.
    //                                         If a string is specified, then only scripts whose names contain the filter string will be retrieved.>
    //                    }
    //    }

    DebuggerCommand cmd(SCRIPTS);
    cmd.arg(TYPES, types);

    if (ids.count())
        cmd.arg(IDS, ids);

    if (includeSource)
        cmd.arg(INCLUDESOURCE, includeSource);

    if (filter.type() == QVariant::String)
        cmd.arg(FILTER, filter.toString());
    else if (filter.type() == QVariant::Int)
        cmd.arg(FILTER, filter.toInt());
    else
        QTC_CHECK(!filter.isValid());

    runCommand(cmd);
}

void QmlEnginePrivate::setBreakpoint(const QString type, const QString target,
                                     bool enabled, int line, int column,
                                     const QString condition, int ignoreCount)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setbreakpoint",
    //      "arguments" : { "type"        : <"function" or "script" or "scriptId" or "scriptRegExp">
    //                      "target"      : <function expression or script identification>
    //                      "line"        : <line in script or function>
    //                      "column"      : <character position within the line>
    //                      "enabled"     : <initial enabled state. True or false, default is true>
    //                      "condition"   : <string with break point condition>
    //                      "ignoreCount" : <number specifying the number of break point hits to ignore, default value is 0>
    //                    }
    //    }
    if (type == EVENT) {
        QPacket rs(dataStreamVersion());
        rs <<  target.toUtf8() << enabled;
        engine->showMessage(QString("%1 %2 %3")
                            .arg(BREAKONSIGNAL, target, QLatin1String(enabled ? "enabled" : "disabled")), LogInput);
        runDirectCommand(BREAKONSIGNAL, rs.data());

    } else {
        DebuggerCommand cmd(SETBREAKPOINT);
        cmd.arg(TYPE, type);
        cmd.arg(ENABLED, enabled);

        if (type == SCRIPTREGEXP)
            cmd.arg(TARGET, Utils::FileName::fromString(target).fileName());
        else
            cmd.arg(TARGET, target);

        if (line)
            cmd.arg(LINE, line - 1);
        if (column)
            cmd.arg(COLUMN, column - 1);
        if (!condition.isEmpty())
            cmd.arg(CONDITION, condition);
        if (ignoreCount != -1)
            cmd.arg(IGNORECOUNT, ignoreCount);

        runCommand(cmd);
    }
}

void QmlEnginePrivate::clearBreakpoint(int breakpoint)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "clearbreakpoint",
    //      "arguments" : { "breakpoint" : <number of the break point to clear>
    //                    }
    //    }

    DebuggerCommand cmd(CLEARBREAKPOINT);
    cmd.arg(BREAKPOINT, breakpoint);
    runCommand(cmd);
}

void QmlEnginePrivate::setExceptionBreak(Exceptions type, bool enabled)
{
    //    { "seq"       : <number>,
    //      "type"      : "request",
    //      "command"   : "setexceptionbreak",
    //      "arguments" : { "type"    : <string: "all", or "uncaught">,
    //                      "enabled" : <optional bool: enables the break type if true>
    //                    }
    //    }

    DebuggerCommand cmd(SETEXCEPTIONBREAK);
    if (type == AllExceptions)
        cmd.arg(TYPE, ALL);

    //Not Supported:
    // else if (type == UncaughtExceptions)
    //    cmd.args(TYPE, UNCAUGHT);

    if (enabled)
        cmd.arg(ENABLED, enabled);

    runCommand(cmd);
}

QmlV8ObjectData QmlEnginePrivate::extractData(const QVariant &data) const
{
    //    { "handle" : <handle>,
    //      "type"   : <"undefined", "null", "boolean", "number", "string", "object", "function" or "frame">
    //    }

    //    {"handle":<handle>,"type":"undefined"}

    //    {"handle":<handle>,"type":"null"}

    //    { "handle":<handle>,
    //      "type"  : <"boolean", "number" or "string">
    //      "value" : <JSON encoded value>
    //    }

    //    {"handle":7,"type":"boolean","value":true}

    //    {"handle":8,"type":"number","value":42}

    //    { "handle"              : <handle>,
    //      "type"                : "object",
    //      "className"           : <Class name, ECMA-262 property [[Class]]>,
    //      "constructorFunction" : {"ref":<handle>},
    //      "protoObject"         : {"ref":<handle>},
    //      "prototypeObject"     : {"ref":<handle>},
    //      "properties" : [ {"name" : <name>,
    //                        "ref"  : <handle>
    //                       },
    //                       ...
    //                     ]
    //    }

    //        { "handle" : <handle>,
    //          "type"                : "function",
    //          "className"           : "Function",
    //          "constructorFunction" : {"ref":<handle>},
    //          "protoObject"         : {"ref":<handle>},
    //          "prototypeObject"     : {"ref":<handle>},
    //          "name"                : <function name>,
    //          "inferredName"        : <inferred function name for anonymous functions>
    //          "source"              : <function source>,
    //          "script"              : <reference to function script>,
    //          "scriptId"            : <id of function script>,
    //          "position"            : <function begin position in script>,
    //          "line"                : <function begin source line in script>,
    //          "column"              : <function begin source column in script>,
    //          "properties" : [ {"name" : <name>,
    //                            "ref"  : <handle>
    //                           },
    //                           ...
    //                         ]
    //        }

    QmlV8ObjectData objectData;
    const QVariantMap dataMap = data.toMap();

    objectData.name = dataMap.value(NAME).toString();

    QString type = dataMap.value(TYPE).toString();
    objectData.handle = dataMap.value(HANDLE).toInt();

    if (type == "undefined") {
        objectData.type = "undefined";
        objectData.value = "undefined";

    } else if (type == "null") { // Deprecated. typeof(null) == "object" in JavaScript
        objectData.type = "object";
        objectData.value = "null";

    } else if (type == "boolean") {
        objectData.type = "boolean";
        objectData.value = dataMap.value(VALUE);

    } else if (type == "number") {
        objectData.type = "number";
        objectData.value = dataMap.value(VALUE);

    } else if (type == "string") {
        QLatin1Char quote('"');
        objectData.type = "string";
        objectData.value = QString(quote + dataMap.value(VALUE).toString() + quote);

    } else if (type == "object") {
        objectData.type = "object";
        // ignore "className": it doesn't make any sense.

        if (dataMap.contains("value")) {
            QVariant value = dataMap.value("value");
            // The QVariant representation of null has changed across various Qt versions
            // 5.6, 5.7: QVariant::Invalid
            // 5.8: isValid(), !isNull(), type() == 51; only typeName() is unique: "std::nullptr_t"
            // 5.9: isValid(), isNull(); We can then use isNull()
            if (!value.isValid() || value.isNull()
                    || strcmp(value.typeName(), "std::nullptr_t") == 0) {
                objectData.value = "null"; // Yes, null is an object.
            } else if (value.isValid()) {
                objectData.expectedProperties = value.toInt();
            }
        }

        if (dataMap.contains("properties"))
            objectData.properties = dataMap.value("properties").toList();
    } else if (type == "function") {
        objectData.type = "function";
        objectData.value = dataMap.value(NAME);
        objectData.properties = dataMap.value("properties").toList();
        QVariant value = dataMap.value("value");
        if (value.isValid())
            objectData.expectedProperties = value.toInt();

    } else if (type == "script") {
        objectData.type = "script";
        objectData.value = dataMap.value(NAME);
    }

    if (dataMap.contains(REF)) {
        objectData.handle = dataMap.value(REF).toInt();
        if (refVals.contains(objectData.handle)) {
            QmlV8ObjectData data = refVals.value(objectData.handle);
            if (objectData.type.isEmpty())
                objectData.type = data.type;
            if (!objectData.value.isValid())
                objectData.value = data.value;
            if (objectData.properties.isEmpty())
                objectData.properties = data.properties;
            if (objectData.expectedProperties < 0)
                objectData.expectedProperties = data.expectedProperties;
        }
    }

    return objectData;
}

void QmlEnginePrivate::runCommand(const DebuggerCommand &command, const QmlCallback &cb)
{
    QJsonObject object;
    object.insert("seq", ++sequence);
    object.insert("type", "request");
    object.insert("command", command.function);
    object.insert("arguments", command.args);
    if (cb)
        callbackForToken[sequence] = cb;

    runDirectCommand(V8REQUEST, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void QmlEnginePrivate::runDirectCommand(const QString &type, const QByteArray &msg)
{
    // Leave item as variable, serialization depends on it.
    QByteArray cmd = V8DEBUG;

    engine->showMessage(QString("%1 %2").arg(type, QString::fromLatin1(msg)), LogInput);

    QPacket rs(dataStreamVersion());
    rs << cmd << type.toLatin1() << msg;

    if (state() == Enabled)
        sendMessage(rs.data());
    else
        sendBuffer.append(rs.data());
}

void QmlEnginePrivate::memorizeRefs(const QVariant &refs)
{
    if (refs.isValid()) {
        foreach (const QVariant &ref, refs.toList()) {
            const QVariantMap refData = ref.toMap();
            int handle = refData.value(HANDLE).toInt();
            refVals[handle] = extractData(refData);
        }
    }
}

void QmlEnginePrivate::messageReceived(const QByteArray &data)
{
    QPacket ds(dataStreamVersion(), data);
    QByteArray command;
    ds >> command;

    if (command == V8DEBUG) {
        QByteArray type;
        QByteArray response;
        ds >> type >> response;

        engine->showMessage(QLatin1String(type), LogOutput);
        if (type == CONNECT) {
            //debugging session started

        } else if (type == INTERRUPT) {
            //debug break requested

        } else if (type == BREAKONSIGNAL) {
            //break on signal handler requested

        } else if (type == V8MESSAGE) {
            SDEBUG(response);
            engine->showMessage(QString(V8MESSAGE) + ' ' + QString::fromLatin1(response), LogOutput);

            const QVariantMap resp = QJsonDocument::fromJson(response).toVariant().toMap();

            const QString type = resp.value(TYPE).toString();

            if (type == "response") {

                const QString debugCommand(resp.value(COMMAND).toString());

                memorizeRefs(resp.value(REFS));

                bool success = resp.value("success").toBool();
                if (!success) {
                    SDEBUG("Request was unsuccessful");
                }

                int requestSeq = resp.value("request_seq").toInt();
                if (callbackForToken.contains(requestSeq)) {
                    callbackForToken[requestSeq](resp);

                } else if (debugCommand == DISCONNECT) {
                    //debugging session ended

                } else if (debugCommand == CONTINEDEBUGGING) {
                    //do nothing, wait for next break

                } else if (debugCommand == SETBREAKPOINT) {
                    //                { "seq"         : <number>,
                    //                  "type"        : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "setbreakpoint",
                    //                  "body"        : { "type"       : <"function" or "script">
                    //                                    "breakpoint" : <break point number of the new break point>
                    //                                  }
                    //                  "running"     : <is the VM running after sending this response>
                    //                  "success"     : true
                    //                }

                    int seq = resp.value("request_seq").toInt();
                    const QVariantMap breakpointData = resp.value(BODY).toMap();
                    int index = breakpointData.value("breakpoint").toInt();

                    if (breakpointsSync.contains(seq)) {
                        BreakpointModelId id = breakpointsSync.take(seq);
                        breakpoints.insert(id, index);

                        //Is actual position info present? Then breakpoint was
                        //accepted
                        const QVariantList actualLocations =
                                breakpointData.value("actual_locations").toList();
                        if (actualLocations.count()) {
                            //The breakpoint requested line should be same as
                            //actual line
                            BreakHandler *handler = engine->breakHandler();
                            Breakpoint bp = handler->breakpointById(id);
                            if (bp.state() != BreakpointInserted) {
                                BreakpointResponse br = bp.response();
                                br.lineNumber = breakpointData.value("line").toInt() + 1;
                                br.pending = false;
                                bp.setResponse(br);
                                bp.notifyBreakpointInsertOk();
                            }
                        }


                    } else {
                        breakpointsTemp.append(index);
                    }


                } else if (debugCommand == CLEARBREAKPOINT) {
                    // DO NOTHING

                } else if (debugCommand == SETEXCEPTIONBREAK) {
                    //                { "seq"               : <number>,
                    //                  "type"              : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "setexceptionbreak",
                    //                  "body"        : { "type"    : <string: "all" or "uncaught" corresponding to the request.>,
                    //                                    "enabled" : <bool: true if the break type is currently enabled as a result of the request>
                    //                                  }
                    //                  "running"     : true
                    //                  "success"     : true
                    //                }


                } else if (debugCommand == SCRIPTS) {
                    //                { "seq"         : <number>,
                    //                  "type"        : "response",
                    //                  "request_seq" : <number>,
                    //                  "command"     : "scripts",
                    //                  "body"        : [ { "name"             : <name of the script>,
                    //                                      "id"               : <id of the script>
                    //                                      "lineOffset"       : <line offset within the containing resource>
                    //                                      "columnOffset"     : <column offset within the containing resource>
                    //                                      "lineCount"        : <number of lines in the script>
                    //                                      "data"             : <optional data object added through the API>
                    //                                      "source"           : <source of the script if includeSource was specified in the request>
                    //                                      "sourceStart"      : <first 80 characters of the script if includeSource was not specified in the request>
                    //                                      "sourceLength"     : <total length of the script in characters>
                    //                                      "scriptType"       : <script type (see request for values)>
                    //                                      "compilationType"  : < How was this script compiled:
                    //                                                               0 if script was compiled through the API
                    //                                                               1 if script was compiled through eval
                    //                                                            >
                    //                                      "evalFromScript"   : <if "compilationType" is 1 this is the script from where eval was called>
                    //                                      "evalFromLocation" : { line   : < if "compilationType" is 1 this is the line in the script from where eval was called>
                    //                                                             column : < if "compilationType" is 1 this is the column in the script from where eval was called>
                    //                                  ]
                    //                  "running"     : <is the VM running after sending this response>
                    //                  "success"     : true
                    //                }

                    if (success) {
                        const QVariantList body = resp.value(BODY).toList();

                        QStringList sourceFiles;
                        for (int i = 0; i < body.size(); ++i) {
                            const QVariantMap entryMap = body.at(i).toMap();
                            const int lineOffset = entryMap.value("lineOffset").toInt();
                            const int columnOffset = entryMap.value("columnOffset").toInt();
                            const QString name = entryMap.value("name").toString();
                            const QString source = entryMap.value("source").toString();

                            if (name.isEmpty())
                                continue;

                            if (!sourceFiles.contains(name))
                                sourceFiles << name;

                            updateScriptSource(name, lineOffset, columnOffset, source);
                        }

                        QMap<QString,QString> files;
                        foreach (const QString &file, sourceFiles) {
                            QString shortName = file;
                            QString fullName = engine->toFileInProject(file);
                            files.insert(shortName, fullName);
                        }

                        engine->sourceFilesHandler()->setSourceFiles(files);
                        //update open editors
                    }
                } else {
                    // DO NOTHING
                }

            } else if (type == EVENT) {
                const QString eventType(resp.value(EVENT).toString());

                if (eventType == "break") {

                    clearRefs();
                    const QVariantMap breakData = resp.value(BODY).toMap();
                    const QString invocationText = breakData.value("invocationText").toString();
                    const QString scriptUrl = breakData.value("script").toMap().value("name").toString();
                    const QString sourceLineText = breakData.value("sourceLineText").toString();

                    bool inferiorStop = true;

                    QList<int> v8BreakpointIds;
                    {
                        const QVariantList v8BreakpointIdList = breakData.value("breakpoints").toList();
                        foreach (const QVariant &breakpointId, v8BreakpointIdList)
                            v8BreakpointIds << breakpointId.toInt();
                    }

                    if (!v8BreakpointIds.isEmpty() && invocationText.startsWith("[anonymous]()")
                            && scriptUrl.endsWith(".qml")
                            && sourceLineText.trimmed().startsWith('(')) {

                        // we hit most likely the anonymous wrapper function automatically generated for bindings
                        // -> relocate the breakpoint to column: 1 and continue

                        int newColumn = sourceLineText.indexOf('(') + 1;
                        BreakHandler *handler = engine->breakHandler();

                        foreach (int v8Id, v8BreakpointIds) {
                            const BreakpointModelId id = breakpoints.key(v8Id);
                            Breakpoint bp = handler->breakpointById(id);
                            if (bp.isValid()) {
                                const BreakpointParameters &params = bp.parameters();

                                clearBreakpoint(v8Id);
                                setBreakpoint(SCRIPTREGEXP,
                                              params.fileName,
                                              params.enabled,
                                              params.lineNumber,
                                              newColumn,
                                              params.condition,
                                              params.ignoreCount);
                                breakpointsSync.insert(sequence, id);
                            }
                        }
                        continueDebugging(Continue);
                        inferiorStop = false;
                    }

                    //Skip debug break if this is an internal function
                    if (sourceLineText == INTERNAL_FUNCTION) {
                        continueDebugging(previousStepAction);
                        inferiorStop = false;
                    }

                    if (inferiorStop) {
                        //Update breakpoint data
                        BreakHandler *handler = engine->breakHandler();
                        foreach (int v8Id, v8BreakpointIds) {
                            const BreakpointModelId id = breakpoints.key(v8Id);
                            Breakpoint bp = handler->breakpointById(id);
                            if (bp) {
                                BreakpointResponse br = bp.response();
                                if (br.functionName.isEmpty()) {
                                    br.functionName = invocationText;
                                    bp.setResponse(br);
                                }
                                if (bp.state() != BreakpointInserted) {
                                    br.lineNumber = breakData.value("sourceLine").toInt() + 1;
                                    br.pending = false;
                                    bp.setResponse(br);
                                    bp.notifyBreakpointInsertOk();
                                }
                            }
                        }

                        if (engine->state() == InferiorRunOk) {
                            foreach (const QVariant &breakpointId, v8BreakpointIds) {
                                if (breakpointsTemp.contains(breakpointId.toInt()))
                                    clearBreakpoint(breakpointId.toInt());
                            }
                            engine->notifyInferiorSpontaneousStop();
                            backtrace();
                        } else if (engine->state() == InferiorStopRequested) {
                            engine->notifyInferiorStopOk();
                            backtrace();
                        }
                    }

                } else if (eventType == "exception") {
                    const QVariantMap body = resp.value(BODY).toMap();
                    int lineNumber = body.value("sourceLine").toInt() + 1;

                    const QVariantMap script = body.value("script").toMap();
                    QUrl fileUrl(script.value(NAME).toString());
                    QString filePath = engine->toFileInProject(fileUrl);

                    const QVariantMap exception = body.value("exception").toMap();
                    QString errorMessage = exception.value("text").toString();

                    QStringList messages = highlightExceptionCode(lineNumber, filePath, errorMessage);
                    foreach (const QString msg, messages)
                        engine->showMessage(msg, ConsoleOutput);

                    if (engine->state() == InferiorRunOk) {
                        engine->notifyInferiorSpontaneousStop();
                        backtrace();
                    }

                    if (engine->state() == InferiorStopOk)
                        backtrace();

                } else if (eventType == "afterCompile") {
                    //Currently break point relocation is disabled.
                    //Uncomment the line below when it will be enabled.
//                    listBreakpoints();
                }

                //Sometimes we do not get event type!
                //This is most probably due to a wrong eval expression.
                //Redirect output to console.
                if (eventType.isEmpty()) {
                    debuggerConsole()->printItem(new ConsoleItem(
                                                     ConsoleItem::ErrorType,
                                                     resp.value(MESSAGE).toString()));
                }

            } //EVENT
        } //V8MESSAGE

    } else {
        //DO NOTHING
    }
}

void QmlEnginePrivate::handleBacktrace(const QVariantMap &response)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "backtrace",
    //      "body"        : { "fromFrame" : <number>
    //                        "toFrame" : <number>
    //                        "totalFrames" : <number>
    //                        "frames" : <array of frames - see frame request for details>
    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = response.value(BODY).toMap();
    const QVariantList frames = body.value("frames").toList();

    int fromFrameIndex = body.value("fromFrame").toInt();

    QTC_ASSERT(0 == fromFrameIndex, return);

    StackHandler *stackHandler = engine->stackHandler();
    StackFrames stackFrames;
    int i = 0;
    stackIndexLookup.clear();
    foreach (const QVariant &frame, frames) {
        StackFrame stackFrame = extractStackFrame(frame);
        if (stackFrame.level.isEmpty())
            continue;
        stackIndexLookup.insert(i, stackFrame.level.toInt());
        stackFrames << stackFrame;
        i++;
    }
    stackHandler->setFrames(stackFrames);
    stackHandler->setCurrentIndex(0);

    updateLocals();
}

StackFrame QmlEnginePrivate::extractStackFrame(const QVariant &bodyVal)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "frame",
    //      "body"        : { "index"          : <frame number>,
    //                        "receiver"       : <frame receiver>,
    //                        "func"           : <function invoked>,
    //                        "script"         : <script for the function>,
    //                        "constructCall"  : <boolean indicating whether the function was called as constructor>,
    //                        "debuggerFrame"  : <boolean indicating whether this is an internal debugger frame>,
    //                        "arguments"      : [ { name: <name of the argument - missing of anonymous argument>,
    //                                               value: <value of the argument>
    //                                             },
    //                                             ... <the array contains all the arguments>
    //                                           ],
    //                        "locals"         : [ { name: <name of the local variable>,
    //                                               value: <value of the local variable>
    //                                             },
    //                                             ... <the array contains all the locals>
    //                                           ],
    //                        "position"       : <source position>,
    //                        "line"           : <source line>,
    //                        "column"         : <source column within the line>,
    //                        "sourceLineText" : <text for current source line>,
    //                        "scopes"         : [ <array of scopes, see scope request below for format> ],

    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }

    const QVariantMap body = bodyVal.toMap();

    StackFrame stackFrame;
    stackFrame.level = body.value("index").toString();
    //Do not insert the frame corresponding to the internal function
    if (body.value("sourceLineText") == INTERNAL_FUNCTION) {
        stackFrame.level.clear();
        return stackFrame;
    }

    auto extractString = [this](const QVariant &item) {
        return ((item.type() == QVariant::String) ? item : extractData(item).value).toString();
    };

    stackFrame.function = extractString(body.value("func"));
    if (stackFrame.function.isEmpty())
        stackFrame.function = QCoreApplication::translate("QmlEngine", "Anonymous Function");
    stackFrame.file = engine->toFileInProject(extractString(body.value("script")));
    stackFrame.usable = QFileInfo(stackFrame.file).isReadable();
    stackFrame.receiver = extractString(body.value("receiver"));
    stackFrame.line = body.value("line").toInt() + 1;

    return stackFrame;
}

void QmlEnginePrivate::handleFrame(const QVariantMap &response)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "frame",
    //      "body"        : { "index"          : <frame number>,
    //                        "receiver"       : <frame receiver>,
    //                        "func"           : <function invoked>,
    //                        "script"         : <script for the function>,
    //                        "constructCall"  : <boolean indicating whether the function was called as constructor>,
    //                        "debuggerFrame"  : <boolean indicating whether this is an internal debugger frame>,
    //                        "arguments"      : [ { name: <name of the argument - missing of anonymous argument>,
    //                                               value: <value of the argument>
    //                                             },
    //                                             ... <the array contains all the arguments>
    //                                           ],
    //                        "locals"         : [ { name: <name of the local variable>,
    //                                               value: <value of the local variable>
    //                                             },
    //                                             ... <the array contains all the locals>
    //                                           ],
    //                        "position"       : <source position>,
    //                        "line"           : <source line>,
    //                        "column"         : <source column within the line>,
    //                        "sourceLineText" : <text for current source line>,
    //                        "scopes"         : [ <array of scopes, see scope request below for format> ],

    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    QVariantMap body = response.value(BODY).toMap();

    StackHandler *stackHandler = engine->stackHandler();
    WatchHandler * watchHandler = engine->watchHandler();
    watchHandler->notifyUpdateStarted();

    const int frameIndex = stackHandler->currentIndex();
    if (frameIndex < 0)
        return;
    const StackFrame frame = stackHandler->currentFrame();
    if (!frame.isUsable())
        return;

    // Always add a "this" variable
    {
        QString iname = "local.this";
        QString exp = "this";
        QmlV8ObjectData objectData = extractData(body.value("receiver"));

        auto item = new WatchItem;
        item->iname = iname;
        item->name = exp;
        item->id = objectData.handle;
        item->type = objectData.type;
        item->value = objectData.value.toString();
        item->setHasChildren(objectData.hasChildren());
        // In case of global object, we do not get children
        // Set children nevertheless and query later.
        if (item->value == "global") {
            item->setHasChildren(true);
            item->id = 0;
        }
        watchHandler->insertItem(item);
        evaluate(exp, -1, [this, iname, exp](const QVariantMap &response) {
            handleEvaluateExpression(response, iname, exp);
        });
    }

    currentFrameScopes.clear();
    const QVariantList scopes = body.value("scopes").toList();
    foreach (const QVariant &scope, scopes) {
        //Do not query for global types (0)
        //Showing global properties increases clutter.
        if (scope.toMap().value("type").toInt() == 0)
            continue;
        int scopeIndex = scope.toMap().value("index").toInt();
        currentFrameScopes.append(scopeIndex);
        this->scope(scopeIndex);
    }
    engine->gotoLocation(stackHandler->currentFrame());

    // Send watchers list
    if (stackHandler->isContentsValid() && stackHandler->currentFrame().isUsable()) {
        QStringList watchers = watchHandler->watchedExpressions();
        foreach (const QString &exp, watchers) {
            const QString iname = watchHandler->watcherName(exp);
            evaluate(exp, -1, [this, iname, exp](const QVariantMap &response) {
                handleEvaluateExpression(response, iname, exp);
            });
        }
    }
}

void QmlEnginePrivate::handleScope(const QVariantMap &response)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "scope",
    //      "body"        : { "index"      : <index of this scope in the scope chain. Index 0 is the top scope
    //                                        and the global scope will always have the highest index for a
    //                                        frame>,
    //                        "frameIndex" : <index of the frame>,
    //                        "type"       : <type of the scope:
    //                                         0: Global
    //                                         1: Local
    //                                         2: With
    //                                         3: Closure
    //                                         4: Catch >,
    //                        "object"     : <the scope object defining the content of the scope.
    //                                        For local and closure scopes this is transient objects,
    //                                        which has a negative handle value>
    //                      }
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    QVariantMap bodyMap = response.value(BODY).toMap();

    //Check if the frameIndex is same as current Stack Index
    StackHandler *stackHandler = engine->stackHandler();
    if (bodyMap.value("frameIndex").toInt() != stackHandler->currentIndex())
        return;

    QmlV8ObjectData objectData = extractData(bodyMap.value("object"));

    LookupItems itemsToLookup;
    foreach (const QVariant &property, objectData.properties) {
        QmlV8ObjectData localData = extractData(property);
        std::unique_ptr<WatchItem> item(new WatchItem);
        item->exp = localData.name;
        //Check for v8 specific local data
        if (item->exp.startsWith('.') || item->exp.isEmpty())
            continue;

        item->name = item->exp;
        item->iname = "local." + item->exp;
        item->id = localData.handle;
        item->type = localData.type;
        item->value = localData.value.toString();
        item->setHasChildren(localData.hasChildren());

        if (localData.value.isValid() || item->wantsChildren || localData.expectedProperties == 0) {
            engine->watchHandler()->insertItem(item.release());
        } else {
            itemsToLookup.insert(int(item->id), {item->iname, item->name, item->exp});
        }
    }
    lookup(itemsToLookup);
    checkForFinishedUpdate();
}

void QmlEnginePrivate::checkForFinishedUpdate()
{
    if (currentlyLookingUp.isEmpty())
        engine->watchHandler()->notifyUpdateFinished();
}

ConsoleItem *QmlEnginePrivate::constructLogItemTree(const QmlV8ObjectData &objectData)
{
    QList<int> handles;
    return constructLogItemTree(objectData, handles);
}

void QmlEnginePrivate::constructChildLogItems(ConsoleItem *item, const QmlV8ObjectData &objectData,
                                              QList<int> &seenHandles)
{
    // We cannot sort the children after attaching them to the parent as that would cause layout
    // changes, invalidating cached indices. So we presort them before inserting.
    QVarLengthArray<ConsoleItem *> children(objectData.properties.size());
    auto it = children.begin();
    foreach (const QVariant &property, objectData.properties)
        *(it++) = constructLogItemTree(extractData(property), seenHandles);

    if (boolSetting(SortStructMembers))
        std::sort(children.begin(), children.end(), compareConsoleItems);

    foreach (ConsoleItem *child, children)
        item->appendChild(child);
}

ConsoleItem *QmlEnginePrivate::constructLogItemTree(const QmlV8ObjectData &objectData,
                                                    QList<int> &seenHandles)
{
    QString text;
    if (objectData.value.isValid()) {
        text = objectData.value.toString();
    } else if (!objectData.type.isEmpty()) {
        text = objectData.type;
    } else {
        int handle = objectData.handle;
        ConsoleItem *item = new ConsoleItem(ConsoleItem::DefaultType,
                                            objectData.name,
                                            [this, handle](ConsoleItem *item)
        {
            DebuggerCommand cmd(LOOKUP);
            cmd.arg(HANDLES, QList<int>() << handle);
            runCommand(cmd, [this, item, handle](const QVariantMap &response) {
                const QVariantMap body = response.value(BODY).toMap();
                QStringList handlesList = body.keys();
                foreach (const QString &handleString, handlesList) {
                    if (handle != handleString.toInt())
                        continue;

                    QmlV8ObjectData objectData = extractData(body.value(handleString));

                    // keep original name, if possible
                    QString name = item->expression();
                    if (name.isEmpty())
                        name = objectData.name;

                    QString value = objectData.value.isValid() ?
                                objectData.value.toString() : objectData.type;

                    // We can do setData() and cause dataChanged() here, but only because this
                    // callback is executed after fetchMore() has returned.
                    item->model()->setData(item->index(),
                                           QString("%1: %2").arg(name, value),
                                           ConsoleItem::ExpressionRole);

                    QList<int> newHandles;
                    constructChildLogItems(item, objectData, newHandles);

                    break;
                }
            });
        });
        return item;
    }

    if (!objectData.name.isEmpty())
        text = QString("%1: %2").arg(objectData.name, text);

    if (objectData.properties.isEmpty())
        return new ConsoleItem(ConsoleItem::DefaultType, text);

    if (seenHandles.contains(objectData.handle)) {
        ConsoleItem *item = new ConsoleItem(ConsoleItem::DefaultType, text,
                                            [this, objectData](ConsoleItem *item)
        {
            QList<int> newHandles;
            constructChildLogItems(item, objectData, newHandles);
        });
        return item;
    }

    seenHandles.append(objectData.handle);
    ConsoleItem *item = new ConsoleItem(ConsoleItem::DefaultType, text);
    constructChildLogItems(item, objectData, seenHandles);
    seenHandles.removeLast();

    return item;
}

void QmlEnginePrivate::insertSubItems(WatchItem *parent, const QVariantList &properties)
{
    QTC_ASSERT(parent, return);
    LookupItems itemsToLookup;

    const QSet<QString> expandedINames = engine->watchHandler()->expandedINames();
    foreach (const QVariant &property, properties) {
        QmlV8ObjectData propertyData = extractData(property);
        std::unique_ptr<WatchItem> item(new WatchItem);
        item->name = propertyData.name;

        // Check for v8 specific local data
        if (item->name.startsWith('.') || item->name.isEmpty())
            continue;
        if (parent->type == "object") {
            if (parent->value == "Array")
                item->exp = parent->exp + '[' + item->name + ']';
            else if (parent->value == "Object")
                item->exp = parent->exp + '.' + item->name;
        } else {
            item->exp = item->name;
        }

        item->iname = parent->iname + '.' + item->name;
        item->id = propertyData.handle;
        item->type = propertyData.type;
        item->value = propertyData.value.toString();
        if (item->type.isEmpty() || expandedINames.contains(item->iname))
            itemsToLookup.insert(propertyData.handle, {item->iname, item->name, item->exp});
        item->setHasChildren(propertyData.hasChildren());
        parent->appendChild(item.release());
    }

    if (boolSetting(SortStructMembers)) {
        parent->sortChildren([](const WatchItem *item1, const WatchItem *item2) {
            return item1->name < item2->name;
        });
    }

    lookup(itemsToLookup);
}

void QmlEnginePrivate::handleExecuteDebuggerCommand(const QVariantMap &response)
{
    auto it = response.constFind(SUCCESS);
    if (it != response.constEnd() && it.value().toBool()) {
        debuggerConsole()->printItem(constructLogItemTree(extractData(response.value(BODY))));

        // Update the locals
        foreach (int index, currentFrameScopes)
            scope(index);
    } else {
        debuggerConsole()->printItem(new ConsoleItem(ConsoleItem::ErrorType,
                                                     response.value(MESSAGE).toString()));
    }
}

void QmlEnginePrivate::handleLookup(const QVariantMap &response)
{
    //    { "seq"         : <number>,
    //      "type"        : "response",
    //      "request_seq" : <number>,
    //      "command"     : "lookup",
    //      "body"        : <array of serialized objects indexed using their handle>
    //      "running"     : <is the VM running after sending this response>
    //      "success"     : true
    //    }
    const QVariantMap body = response.value(BODY).toMap();

    QStringList handlesList = body.keys();
    foreach (const QString &handleString, handlesList) {
        int handle = handleString.toInt();
        QmlV8ObjectData bodyObjectData = extractData(body.value(handleString));
        QList<LookupData> vals = currentlyLookingUp.values(handle);
        currentlyLookingUp.remove(handle);
        foreach (const LookupData &res, vals) {
            auto item = new WatchItem;
            item->exp = res.exp;
            item->iname = res.iname;
            item->name = res.name;
            item->id = handle;

            item->type = bodyObjectData.type;
            item->value = bodyObjectData.value.toString();

            item->setHasChildren(bodyObjectData.hasChildren());
            insertSubItems(item, bodyObjectData.properties);

            engine->watchHandler()->insertItem(item);
        }
    }
    checkForFinishedUpdate();
}

void QmlEnginePrivate::stateChanged(State state)
{
    engine->logServiceStateChange(name(), serviceVersion(), state);

    if (state == QmlDebugClient::Enabled) {
        /// Start session.
        flushSendBuffer();
        QJsonObject parameters;
        parameters.insert("redundantRefs", false);
        parameters.insert("namesAsObjects", false);
        runDirectCommand(CONNECT, QJsonDocument(parameters).toJson());
        runCommand({VERSION}, CB(handleVersion));
    }
}

void QmlEnginePrivate::handleVersion(const QVariantMap &response)
{
    unpausedEvaluate = response.value(BODY).toMap().value("UnpausedEvaluate", false).toBool();
    contextEvaluate = response.value(BODY).toMap().value("ContextEvaluate", false).toBool();
}

void QmlEnginePrivate::flushSendBuffer()
{
    QTC_ASSERT(state() == Enabled, return);
    foreach (const QByteArray &msg, sendBuffer)
        sendMessage(msg);
    sendBuffer.clear();
}

DebuggerEngine *createQmlEngine()
{
    return new QmlEngine;
}

} // Internal
} // Debugger
