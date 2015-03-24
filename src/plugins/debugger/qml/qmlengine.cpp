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

#include "qmlengine.h"
#include "baseqmldebuggerclient.h"
#include "qmlinspectoragent.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerstringutils.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/localsandexpressionswindow.h>
#include <debugger/threaddata.h>
#include <debugger/watchwindow.h>

#include <debugger/breakhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/watchhandler.h>
#include <debugger/sourcefileshandler.h>

#include <qmljseditor/qmljseditorconstants.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/consolemanagerinterface.h>

#include <utils/qtcassert.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QMessageBox>
#include <QPlainTextEdit>

#define DEBUG_QML 1
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

using namespace QmlJS;
using namespace AST;

namespace Debugger {
namespace Internal {

static QTreeView *inspectorTreeView()
{
    return Internal::inspectorView();
}

class ASTWalker : public Visitor
{
public:
    void operator()(Node *ast, quint32 *l, quint32 *c)
    {
        done = false;
        line = l;
        column = c;
        Node::accept(ast, this);
    }

    bool preVisit(Node *ast)
    {
        return ast->lastSourceLocation().startLine >= *line && !done;
    }

    //Case 1: Breakpoint is between sourceStart(exclusive) and
    //        sourceEnd(inclusive) --> End tree walk.
    //Case 2: Breakpoint is on sourceStart --> Check for the start
    //        of the first executable code. Set the line number and
    //        column number. End tree walk.
    //Case 3: Breakpoint is on "unbreakable" code --> Find the next "breakable"
    //        code and check for Case 2. End tree walk.

    //Add more types when suitable.

    bool visit(UiScriptBinding *ast)
    {
        if (!ast->statement)
            return true;

        quint32 sourceStartLine = ast->firstSourceLocation().startLine;
        quint32 statementStartLine;
        quint32 statementColumn;

        if (ast->statement->kind == Node::Kind_ExpressionStatement) {
            statementStartLine = ast->statement->firstSourceLocation().
                    startLine;
            statementColumn = ast->statement->firstSourceLocation().startColumn;

        } else if (ast->statement->kind == Node::Kind_Block) {
            Block *block = static_cast<Block *>(ast->statement);
            if (!block || !block->statements)
                return true;
            statementStartLine = block->statements->firstSourceLocation().
                    startLine;
            statementColumn = block->statements->firstSourceLocation().
                    startColumn;

        } else {
            return true;
        }


        //Case 1
        //Check for possible relocation within the binding statement

        //Rewritten to (function <token>() { { }})
        //The offset 16 is position of inner lbrace without token length.
        const int offset = 16;

        //Case 2
        if (statementStartLine == *line) {
            if (sourceStartLine == *line)
                *column = offset + ast->qualifiedId->identifierToken.length;
            done = true;
        }

        //Case 3
        if (statementStartLine > *line) {
            *line = statementStartLine;
            if (sourceStartLine == *line)
                *column = offset + ast->qualifiedId->identifierToken.length;
            else
                *column = statementColumn;
            done = true;
        }
        return true;
    }

    bool visit(FunctionDeclaration *ast) {
        quint32 sourceStartLine = ast->firstSourceLocation().startLine;
        quint32 sourceStartColumn = ast->firstSourceLocation().startColumn;
        quint32 statementStartLine = ast->body->firstSourceLocation().startLine;
        quint32 statementColumn = ast->body->firstSourceLocation().startColumn;

        //Case 1
        //Check for possible relocation within the function declaration

        //Case 2
        if (statementStartLine == *line) {
            if (sourceStartLine == *line)
                *column = statementColumn - sourceStartColumn + 1;
            done = true;
        }

        //Case 3
        if (statementStartLine > *line) {
            *line = statementStartLine;
            if (sourceStartLine == *line)
                *column = statementColumn - sourceStartColumn + 1;
            else
                *column = statementColumn;
            done = true;
        }
        return true;
    }

    bool visit(EmptyStatement *ast)
    {
        *line = ast->lastSourceLocation().startLine + 1;
        return true;
    }

    bool visit(VariableStatement *ast) { test(ast); return true; }
    bool visit(VariableDeclarationList *ast) { test(ast); return true; }
    bool visit(VariableDeclaration *ast) { test(ast); return true; }
    bool visit(ExpressionStatement *ast) { test(ast); return true; }
    bool visit(IfStatement *ast) { test(ast); return true; }
    bool visit(DoWhileStatement *ast) { test(ast); return true; }
    bool visit(WhileStatement *ast) { test(ast); return true; }
    bool visit(ForStatement *ast) { test(ast); return true; }
    bool visit(LocalForStatement *ast) { test(ast); return true; }
    bool visit(ForEachStatement *ast) { test(ast); return true; }
    bool visit(LocalForEachStatement *ast) { test(ast); return true; }
    bool visit(ContinueStatement *ast) { test(ast); return true; }
    bool visit(BreakStatement *ast) { test(ast); return true; }
    bool visit(ReturnStatement *ast) { test(ast); return true; }
    bool visit(WithStatement *ast) { test(ast); return true; }
    bool visit(SwitchStatement *ast) { test(ast); return true; }
    bool visit(CaseBlock *ast) { test(ast); return true; }
    bool visit(CaseClauses *ast) { test(ast); return true; }
    bool visit(CaseClause *ast) { test(ast); return true; }
    bool visit(DefaultClause *ast) { test(ast); return true; }
    bool visit(LabelledStatement *ast) { test(ast); return true; }
    bool visit(ThrowStatement *ast) { test(ast); return true; }
    bool visit(TryStatement *ast) { test(ast); return true; }
    bool visit(Catch *ast) { test(ast); return true; }
    bool visit(Finally *ast) { test(ast); return true; }
    bool visit(FunctionExpression *ast) { test(ast); return true; }
    bool visit(DebuggerStatement *ast) { test(ast); return true; }

    void test(Node *ast)
    {
        quint32 statementStartLine = ast->firstSourceLocation().startLine;
        //Case 1/2
        if (statementStartLine <= *line &&
                *line <= ast->lastSourceLocation().startLine)
            done = true;

        //Case 3
        if (statementStartLine > *line) {
            *line = statementStartLine;
            *column = ast->firstSourceLocation().startColumn;
            done = true;
        }
    }

    bool done;
    quint32 *line;
    quint32 *column;
};

ConsoleManagerInterface *qmlConsoleManager()
{
    return ConsoleManagerInterface::instance();
}

///////////////////////////////////////////////////////////////////////
//
// QmlEngine
//
///////////////////////////////////////////////////////////////////////

QmlEngine::QmlEngine(const DebuggerStartParameters &startParameters, DebuggerEngine *masterEngine)
  : DebuggerEngine(startParameters)
  , m_adapter(this)
  , m_inspectorAdapter(&m_adapter, this)
  , m_retryOnConnectFail(false)
  , m_automaticConnect(false)
{
    setObjectName(QLatin1String("QmlEngine"));

    if (masterEngine)
        setMasterEngine(masterEngine);

    connect(&m_adapter, SIGNAL(connectionError(QDebugSupport::Error)),
        SLOT(connectionError(QDebugSupport::Error)));
    connect(&m_adapter, SIGNAL(serviceConnectionError(QString)),
        SLOT(serviceConnectionError(QString)));
    connect(&m_adapter, SIGNAL(connected()),
        SLOT(connectionEstablished()));
    connect(&m_adapter, SIGNAL(connectionStartupFailed()),
        SLOT(connectionStartupFailed()));

    connect(stackHandler(), SIGNAL(stackChanged()),
            SLOT(updateCurrentContext()));
    connect(stackHandler(), SIGNAL(currentIndexChanged()),
            SLOT(updateCurrentContext()));
    connect(inspectorTreeView(), SIGNAL(currentIndexChanged(QModelIndex)),
            SLOT(updateCurrentContext()));
    connect(m_inspectorAdapter.agent(), SIGNAL(
                expressionResult(quint32,QVariant)),
            SLOT(expressionEvaluated(quint32,QVariant)));
    connect(m_adapter.messageClient(),
            SIGNAL(message(QtMsgType,QString,
                           QmlDebug::QDebugContextInfo)),
            SLOT(appendDebugOutput(QtMsgType,QString,
                                   QmlDebug::QDebugContextInfo)));


    connect(&m_applicationLauncher,
        SIGNAL(processExited(int,QProcess::ExitStatus)),
        SLOT(disconnected()));
    connect(&m_applicationLauncher,
        SIGNAL(appendMessage(QString,Utils::OutputFormat)),
        SLOT(appendMessage(QString,Utils::OutputFormat)));
    connect(&m_applicationLauncher,
            SIGNAL(processStarted()),
            &m_noDebugOutputTimer,
            SLOT(start()));

    m_outputParser.setNoOutputText(ProjectExplorer::ApplicationLauncher
                                   ::msgWinCannotRetrieveDebuggingOutput());
    connect(&m_outputParser, SIGNAL(waitingForConnectionOnPort(quint16)),
            this, SLOT(beginConnection(quint16)));
    connect(&m_outputParser, SIGNAL(noOutputMessage()),
            this, SLOT(tryToConnect()));
    connect(&m_outputParser, SIGNAL(errorMessage(QString)),
            this, SLOT(appStartupFailed(QString)));

    // Only wait 8 seconds for the 'Waiting for connection' on application output,
    // then just try to connect (application output might be redirected / blocked)
    m_noDebugOutputTimer.setSingleShot(true);
    m_noDebugOutputTimer.setInterval(8000);
    connect(&m_noDebugOutputTimer, SIGNAL(timeout()), this, SLOT(tryToConnect()));

    ModelManagerInterface *mmIface = ModelManagerInterface::instance();
    if (mmIface) {
        connect(ModelManagerInterface::instance(), SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
                this, SLOT(documentUpdated(QmlJS::Document::Ptr)));
    }
    // we won't get any debug output
    if (startParameters.useTerminal) {
        m_noDebugOutputTimer.setInterval(0);
        m_retryOnConnectFail = true;
        m_automaticConnect = true;
    }
    if (qmlConsoleManager())
        qmlConsoleManager()->setScriptEvaluator(this);
}

QmlEngine::~QmlEngine()
{
    QSet<Core::IDocument *> documentsToClose;

    QHash<QString, QWeakPointer<TextEditor::BaseTextEditor> >::iterator iter;
    for (iter = m_sourceEditors.begin(); iter != m_sourceEditors.end(); ++iter) {
        QWeakPointer<TextEditor::BaseTextEditor> textEditPtr = iter.value();
        if (textEditPtr)
            documentsToClose << textEditPtr.data()->document();
    }
    Core::EditorManager::closeDocuments(documentsToClose.toList());
}

void QmlEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    notifyInferiorSetupOk();

    if (m_automaticConnect)
        beginConnection();
}

void QmlEngine::appendMessage(const QString &msg, Utils::OutputFormat /* format */)
{
    showMessage(msg, AppOutput); // FIXME: Redirect to RunControl
}

void QmlEngine::connectionEstablished()
{
    attemptBreakpointSynchronization();

    if (!watchHandler()->watcherNames().isEmpty())
        synchronizeWatchers();
    connect(watchModel(),SIGNAL(layoutChanged()),this,SLOT(synchronizeWatchers()));

    if (state() == EngineRunRequested)
        notifyEngineRunAndInferiorRunOk();
}

void QmlEngine::tryToConnect(quint16 port)
{
    showMessage(QLatin1String("QML Debugger: No application output received in time, trying to connect ..."), LogStatus);
    m_retryOnConnectFail = true;
    if (state() == EngineRunRequested) {
        if (isSlaveEngine()) {
            // Probably cpp is being debugged and hence we did not get the output yet.
            if (!masterEngine()->isDying()) {
                m_noDebugOutputTimer.setInterval(4000);
                m_noDebugOutputTimer.start();
            }
            else
                appStartupFailed(tr("No application output received in time"));
        } else {
            beginConnection(port);
        }
    } else {
        m_automaticConnect = true;
    }
}

void QmlEngine::beginConnection(quint16 port)
{
    m_noDebugOutputTimer.stop();

    if (state() != EngineRunRequested && m_retryOnConnectFail)
        return;

    QTC_ASSERT(state() == EngineRunRequested, return);

    QString host =  startParameters().qmlServerAddress;
    // Use localhost as default
    if (host.isEmpty())
        host = QLatin1String("localhost");

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
    if (startParameters().qmlServerPort > 0)
        port = startParameters().qmlServerPort;

    m_adapter.beginConnectionTcp(host, port);
}


void QmlEngine::connectionStartupFailed()
{
    if (m_retryOnConnectFail) {
        // retry after 3 seconds ...
        QTimer::singleShot(3000, this, SLOT(beginConnection()));
        return;
    }

    QMessageBox *infoBox = new QMessageBox(Core::ICore::mainWindow());
    infoBox->setIcon(QMessageBox::Critical);
    infoBox->setWindowTitle(tr("Qt Creator"));
    infoBox->setText(tr("Could not connect to the in-process QML debugger."
                        "\nDo you want to retry?"));
    infoBox->setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel |
                                QMessageBox::Help);
    infoBox->setDefaultButton(QMessageBox::Retry);
    infoBox->setModal(true);

    connect(infoBox, SIGNAL(finished(int)),
            this, SLOT(errorMessageBoxFinished(int)));

    infoBox->show();
}

void QmlEngine::appStartupFailed(const QString &errorMessage)
{
    QString error = tr("Could not connect to the in-process QML debugger."
                       "\n%1").arg(errorMessage);

    if (isMasterEngine()) {
        QMessageBox *infoBox = new QMessageBox(Core::ICore::mainWindow());
        infoBox->setIcon(QMessageBox::Critical);
        infoBox->setWindowTitle(tr("Qt Creator"));
        infoBox->setText(error);
        infoBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
        infoBox->setDefaultButton(QMessageBox::Ok);
        connect(infoBox, SIGNAL(finished(int)),
                this, SLOT(errorMessageBoxFinished(int)));
        infoBox->show();
    } else {
        showMessage(error, StatusBar);
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
        Core::HelpManager::handleHelpRequest(QLatin1String("qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"));
        // fall through
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

void QmlEngine::connectionError(QDebugSupport::Error error)
{
    if (error == QDebugSupport::RemoteClosedConnectionError)
        showMessage(tr("QML Debugger: Remote host closed connection."), StatusBar);

    if (!isSlaveEngine()) { // normal flow for slave engine when gdb exits
        notifyInferiorSpontaneousStop();
        notifyInferiorIll();
    }
}

void QmlEngine::serviceConnectionError(const QString &serviceName)
{
    showMessage(tr("QML Debugger: Could not connect to service \"%1\".")
        .arg(serviceName), StatusBar);
}

bool QmlEngine::canDisplayTooltip() const
{
    return false;
}

void QmlEngine::filterApplicationMessage(const QString &output, int /*channel*/)
{
    m_outputParser.processOutput(output);
}

void QmlEngine::showMessage(const QString &msg, int channel, int timeout) const
{
    if (channel == AppOutput || channel == AppError)
        const_cast<QmlEngine*>(this)->filterApplicationMessage(msg, channel);
    DebuggerEngine::showMessage(msg, channel, timeout);
}

void QmlEngine::gotoLocation(const Location &location)
{
    const QString fileName = location.fileName();
    if (QUrl(fileName).isLocalFile()) {
        // internal file from source files -> show generated .js
        QTC_ASSERT(m_sourceDocuments.contains(fileName), return);

        QString titlePattern = tr("JS Source for %1").arg(fileName);
        //Check if there are open documents with the same title
        foreach (Core::IDocument *document, Core::DocumentModel::openedDocuments()) {
            if (document->displayName() == titlePattern) {
                Core::EditorManager::activateEditorForDocument(document);
                return;
            }
        }
        Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                    QmlJSEditor::Constants::C_QMLJSEDITOR_ID, &titlePattern);
        if (editor) {
            editor->document()->setProperty(Constants::OPENED_BY_DEBUGGER, true);
            QPlainTextEdit *plainTextEdit =
                    qobject_cast<QPlainTextEdit *>(editor->widget());
            if (plainTextEdit)
                plainTextEdit->setReadOnly(true);
            updateDocument(editor->document(), m_sourceDocuments.value(fileName));
        }
    } else {
        DebuggerEngine::gotoLocation(location);
    }
}

void QmlEngine::closeConnection()
{
    disconnect(watchModel(),SIGNAL(layoutChanged()),this,SLOT(synchronizeWatchers()));
    m_adapter.closeConnection();
}

void QmlEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    if (!isSlaveEngine()) {
        if (startParameters().startMode == AttachToRemoteServer)
            m_noDebugOutputTimer.start();
        else if (startParameters().startMode == AttachToRemoteProcess)
            beginConnection();
        else
            startApplicationLauncher();
    } else {
        m_noDebugOutputTimer.start();
    }
}

void QmlEngine::startApplicationLauncher()
{
    if (!m_applicationLauncher.isRunning()) {
        appendMessage(tr("Starting %1 %2").arg(
                          QDir::toNativeSeparators(startParameters().executable),
                          startParameters().processArgs)
                      + QLatin1Char('\n')
                     , Utils::NormalMessageFormat);
        m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui,
                                    startParameters().executable,
                                    startParameters().processArgs);
    }
}

void QmlEngine::stopApplicationLauncher()
{
    if (m_applicationLauncher.isRunning()) {
        disconnect(&m_applicationLauncher, SIGNAL(processExited(int,QProcess::ExitStatus)),
                   this, SLOT(disconnected()));
        m_applicationLauncher.stop();
    }
}

void QmlEngine::notifyEngineRemoteSetupFinished(const RemoteSetupResult &result)
{
    DebuggerEngine::notifyEngineRemoteSetupFinished(result);

    if (result.success) {
        if (result.qmlServerPort != InvalidPort)
            startParameters().qmlServerPort = result.qmlServerPort;

        notifyEngineSetupOk();

        // The remote setup can take while especialy with mixed debugging.
        // Just waiting for 8 seconds is not enough. Increase the timeout
        // to 60 s
        // In case we get an output the m_outputParser will start the connection.
        m_noDebugOutputTimer.setInterval(60000);
    }
    else {
        if (isMasterEngine())
            QMessageBox::critical(Core::ICore::dialogParent(), tr("Failed to start application"),
                                  tr("Application startup failed: %1").arg(result.reason));
        notifyEngineSetupFailed();
    }
}

void QmlEngine::notifyEngineRemoteServerRunning(const QByteArray &serverChannel, int pid)
{
    bool ok = false;
    quint16 qmlPort = serverChannel.toUInt(&ok);
    if (ok)
        startParameters().qmlServerPort = qmlPort;
    else
        qWarning() << tr("QML debugging port not set: Unable to convert %1 to unsigned int.").arg(QString::fromLatin1(serverChannel));

    DebuggerEngine::notifyEngineRemoteServerRunning(serverChannel, pid);
    notifyEngineSetupOk();

    // The remote setup can take a while especially with mixed debugging.
    // Just waiting for 8 seconds is not enough. Increase the timeout
    // to 60 s
    // In case we get an output the m_outputParser will start the connection.
    m_noDebugOutputTimer.setInterval(60000);
}

void QmlEngine::shutdownInferior()
{
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->endSession();

    if (isSlaveEngine())
        resetLocation();
    stopApplicationLauncher();
    closeConnection();

    notifyInferiorShutdownOk();
}

void QmlEngine::shutdownEngine()
{
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->resetSession();

    if (qmlConsoleManager())
        qmlConsoleManager()->setScriptEvaluator(0);
    m_noDebugOutputTimer.stop();

   // double check (ill engine?):
    stopApplicationLauncher();

    notifyEngineShutdownOk();
    if (!isSlaveEngine())
        showMessage(QString(), StatusBar);
}

void QmlEngine::setupEngine()
{
    if (startParameters().remoteSetupNeeded) {
        // we need to get the port first
        notifyEngineRequestRemoteSetup();
    } else {
        m_applicationLauncher.setEnvironment(startParameters().environment);
        m_applicationLauncher.setWorkingDirectory(startParameters().workingDirectory);

        // We can't do this in the constructore because runControl() isn't yet defined
        connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
                runControl(), SLOT(bringApplicationToForeground(qint64)),
                Qt::UniqueConnection);

        notifyEngineSetupOk();
    }
}

void QmlEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->continueInferior();
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::interruptInferior()
{
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->interruptInferior();
    notifyInferiorStopOk();
}

void QmlEngine::executeStep()
{
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->executeStep();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepI()
{
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->executeStepI();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepOut()
{
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->executeStepOut();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNext()
{
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->executeNext();
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
    resetLocation();
    ContextData modifiedData = data;
    quint32 line = data.lineNumber;
    quint32 column;
    bool valid;
    if (adjustBreakpointLineAndColumn(data.fileName, &line, &column, &valid))
        modifiedData.lineNumber = line;
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->executeRunToLine(modifiedData);
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

    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->activateFrame(index);
    gotoLocation(stackHandler()->frames().value(index));
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
    quint32 line = params.lineNumber;
    quint32 column = 0;
    if (params.type == BreakpointByFileAndLine) {
        bool valid = false;
        if (!adjustBreakpointLineAndColumn(params.fileName, &line, &column,
                                           &valid)) {
            pendingBreakpoints.insertMulti(params.fileName, bp);
            return;
        }
        if (!valid)
            return;
    }

    if (m_adapter.activeDebuggerClient()) {
        m_adapter.activeDebuggerClient()->insertBreakpoint(bp, line, column);
    } else {
        foreach (BaseQmlDebuggerClient *client, m_adapter.debuggerClients()) {
            client->insertBreakpoint(bp, line, column);
        }
    }
}

void QmlEngine::removeBreakpoint(Breakpoint bp)
{
    const BreakpointParameters &params = bp.parameters();
    if (params.type == BreakpointByFileAndLine &&
            pendingBreakpoints.contains(params.fileName)) {
        auto it = pendingBreakpoints.find(params.fileName);
        while (it != pendingBreakpoints.end() && it.key() == params.fileName) {
            if (it.value() == bp.id()) {
                pendingBreakpoints.erase(it);
                return;
            }
            ++it;
        }
    }

    BreakpointState state = bp.state();
    QTC_ASSERT(state == BreakpointRemoveRequested, qDebug() << bp << this << state);
    bp.notifyBreakpointRemoveProceeding();

    if (m_adapter.activeDebuggerClient()) {
        m_adapter.activeDebuggerClient()->removeBreakpoint(bp);
    } else {
        foreach (BaseQmlDebuggerClient *client, m_adapter.debuggerClients()) {
            client->removeBreakpoint(bp);
        }
    }

    if (bp.state() == BreakpointRemoveProceeding)
        bp.notifyBreakpointRemoveOk();
}

void QmlEngine::changeBreakpoint(Breakpoint bp)
{
    BreakpointState state = bp.state();
    QTC_ASSERT(state == BreakpointChangeRequested, qDebug() << bp << this << state);
    bp.notifyBreakpointChangeProceeding();

    if (m_adapter.activeDebuggerClient()) {
        m_adapter.activeDebuggerClient()->changeBreakpoint(bp);
    } else {
        foreach (BaseQmlDebuggerClient *client, m_adapter.debuggerClients()) {
            client->changeBreakpoint(bp);
        }
    }

    if (bp.state() == BreakpointChangeProceeding)
        bp.notifyBreakpointChangeOk();
}

void QmlEngine::attemptBreakpointSynchronization()
{
    if (!stateAcceptsBreakpointChanges()) {
        showMessage(_("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE"));
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

    if (m_adapter.activeDebuggerClient()) {
        m_adapter.activeDebuggerClient()->synchronizeBreakpoints();
    } else {
        foreach (BaseQmlDebuggerClient *client, m_adapter.debuggerClients()) {
            client->synchronizeBreakpoints();
        }
    }
}

bool QmlEngine::acceptsBreakpoint(Breakpoint bp) const
{
    if (!bp.parameters().isCppBreakpoint())
            return true;

    //If it is a Cpp Breakpoint query if the type can be also handled by the debugger client
    //TODO: enable setting of breakpoints before start of debug session
    //For now, the event breakpoint can be set after the activeDebuggerClient is known
    //This is because the older client does not support BreakpointOnQmlSignalHandler
    bool acceptBreakpoint = false;
    if (m_adapter.activeDebuggerClient())
        acceptBreakpoint = m_adapter.activeDebuggerClient()->acceptsBreakpoint(bp);
    return acceptBreakpoint;
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
    if (m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->getSourceFiles();
}

void QmlEngine::requestModuleSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

bool QmlEngine::setToolTipExpression(const DebuggerToolTipContext &context)
{
    // This is processed by QML inspector, which has dependencies to
    // the qml js editor. Makes life easier.
    // FIXME: Except that there isn't any attached.
    emit tooltipRequested(context);
    return true;
}

//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void QmlEngine::assignValueInDebugger(WatchItem *item,
    const QString &expression, const QVariant &valueV)
{
    if (!expression.isEmpty()) {
        if (item->isInspect() && m_inspectorAdapter.agent())
            m_inspectorAdapter.agent()->assignValue(item, expression, valueV);
        else if (m_adapter.activeDebuggerClient())
            m_adapter.activeDebuggerClient()->assignValueInDebugger(item, expression, valueV);
    }
}

void QmlEngine::updateWatchItem(WatchItem *item)
{
//    qDebug() << "UPDATE WATCH DATA" << data.toString();
    //showStatusMessage(tr("Stopped."), 5000);

    if (item->isInspect()) {
        m_inspectorAdapter.agent()->updateWatchData(*item);
    } else {
        if (!item->name.isEmpty() && m_adapter.activeDebuggerClient()) {
            if (item->isValueNeeded())
                m_adapter.activeDebuggerClient()->updateWatchData(*item);
            if (item->isChildrenNeeded() && watchHandler()->isExpandedIName(item->iname)) {
                m_adapter.activeDebuggerClient()->expandObject(item->iname, item->id);
            }
        }
        synchronizeWatchers();
    }
}

void QmlEngine::watchDataSelected(const QByteArray &iname)
{
    const WatchItem *item = watchHandler()->findItem(iname);
    if (item && item->isInspect())
        m_inspectorAdapter.agent()->watchDataSelected(item->id);
}

void QmlEngine::synchronizeWatchers()
{
    QStringList watchedExpressions = watchHandler()->watchedExpressions();
    // send watchers list
    if (m_adapter.activeDebuggerClient()) {
        m_adapter.activeDebuggerClient()->synchronizeWatchers(watchedExpressions);
    } else {
        foreach (BaseQmlDebuggerClient *client, m_adapter.debuggerClients())
            client->synchronizeWatchers(watchedExpressions);
    }
}

ConsoleItem *constructLogItemTree(ConsoleItem *parent,
                                                 const QVariant &result,
                                                 const QString &key = QString())
{
    using namespace QmlJS;
    bool sorted = boolSetting(SortStructMembers);
    if (!result.isValid())
        return 0;

    ConsoleItem *item = new ConsoleItem(parent);
    if (result.type() == QVariant::Map) {
        if (key.isEmpty())
            item->setText(_("Object"));
        else
            item->setText(key + _(" : Object"));

        QMapIterator<QString, QVariant> i(result.toMap());
        while (i.hasNext()) {
            i.next();
            ConsoleItem *child = constructLogItemTree(item, i.value(), i.key());
            if (child)
                item->insertChild(child, sorted);
        }
    } else if (result.type() == QVariant::List) {
        if (key.isEmpty())
            item->setText(_("List"));
        else
            item->setText(QString(_("[%1] : List")).arg(key));
        QVariantList resultList = result.toList();
        for (int i = 0; i < resultList.count(); i++) {
            ConsoleItem *child = constructLogItemTree(item, resultList.at(i),
                                                          QString::number(i));
            if (child)
                item->insertChild(child, sorted);
        }
    } else if (result.canConvert(QVariant::String)) {
        item->setText(result.toString());
    } else {
        item->setText(_("Unknown Value"));
    }

    return item;
}

void QmlEngine::expressionEvaluated(quint32 queryId, const QVariant &result)
{
    if (queryIds.contains(queryId)) {
        queryIds.removeOne(queryId);
        using namespace QmlJS;
        ConsoleManagerInterface *consoleManager = qmlConsoleManager();
        if (consoleManager) {
            ConsoleItem *item = constructLogItemTree(consoleManager->rootItem(), result);
            if (item)
                consoleManager->printToConsolePane(item);
        }
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
    m_noDebugOutputTimer.stop();
    m_automaticConnect = false;
    m_retryOnConnectFail = false;
    DebuggerEngine::quitDebugger();
}

void QmlEngine::inferiorSpontaneousStop()
{
    if (state() == InferiorRunOk)
        notifyInferiorSpontaneousStop();
}

void QmlEngine::disconnected()
{
    showMessage(tr("QML Debugger disconnected."), StatusBar);
    notifyInferiorExited();
}

void QmlEngine::documentUpdated(Document::Ptr doc)
{
    QString fileName = doc->fileName();
    if (pendingBreakpoints.contains(fileName)) {
        QList<Breakpoint> bps = pendingBreakpoints.values(fileName);
        pendingBreakpoints.remove(fileName);
        foreach (const Breakpoint bp, bps)
            insertBreakpoint(bp);
    }
}

void QmlEngine::updateCurrentContext()
{
    QString context;
    if (state() == InferiorStopOk) {
        context = stackHandler()->currentFrame().function;
    } else {
        QModelIndex currentIndex = inspectorTreeView()->currentIndex();
        const WatchData *currentData = watchHandler()->watchItem(currentIndex);
        if (!currentData)
            return;
        const WatchData *parentData = watchHandler()->watchItem(currentIndex.parent());
        const WatchData *grandParentData = watchHandler()->watchItem(
                    currentIndex.parent().parent());
        if (currentData->id != parentData->id)
            context = currentData->name;
        else if (parentData->id != grandParentData->id)
            context = parentData->name;
        else
            context = grandParentData->name;
    }

    synchronizeWatchers();

    ConsoleManagerInterface *consoleManager = qmlConsoleManager();
    if (consoleManager)
        consoleManager->setContext(tr("Context:") + QLatin1Char(' ') + context);
}

void QmlEngine::appendDebugOutput(QtMsgType type, const QString &message,
                                  const QmlDebug::QDebugContextInfo &info)
{
    using namespace QmlJS;
    ConsoleItem::ItemType itemType;
    switch (type) {
    case QtDebugMsg:
        itemType = ConsoleItem::DebugType;
        break;
    case QtWarningMsg:
        itemType = ConsoleItem::WarningType;
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        itemType = ConsoleItem::ErrorType;
        break;
    default:
        //This case is not possible
        return;
    }
    ConsoleManagerInterface *consoleManager = qmlConsoleManager();
    if (consoleManager) {
        ConsoleItem *item = new ConsoleItem(consoleManager->rootItem(), itemType, message);
        item->file = info.file;
        item->line = info.line;
        consoleManager->printToConsolePane(item);
    }
}

void QmlEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if ((languages & QmlLanguage) && m_adapter.activeDebuggerClient())
        m_adapter.activeDebuggerClient()->executeDebuggerCommand(command);
}

bool QmlEngine::evaluateScript(const QString &expression)
{
    bool didEvaluate = true;
    // Evaluate expression based on engine state
    // When engine->state() == InferiorStopOk, the expression is sent to debuggerClient.
    if (state() != InferiorStopOk) {
        QModelIndex currentIndex = inspectorTreeView()->currentIndex();
        QmlInspectorAgent *agent = m_inspectorAdapter.agent();
        quint32 queryId = agent->queryExpressionResult(watchHandler()->watchItem(currentIndex)->id,
                                                       expression);
        if (queryId) {
            queryIds << queryId;
        } else {
            didEvaluate = false;
            using namespace QmlJS;
            ConsoleManagerInterface *consoleManager = qmlConsoleManager();
            if (consoleManager) {
                consoleManager->printToConsolePane(ConsoleItem::ErrorType,
                                                   _("Error evaluating expression."));
            }
        }
    } else {
        executeDebuggerCommand(expression, QmlLanguage);
    }
    return didEvaluate;
}

QString QmlEngine::qmlImportPath() const
{
    return startParameters().environment.value(QLatin1String("QML_IMPORT_PATH"));
}

void QmlEngine::logMessage(const QString &service, LogDirection direction, const QString &message)
{
    QString msg = service;
    msg += direction == LogSend ? QLatin1String(": sending ") : QLatin1String(": receiving ");
    msg += message;
    showMessage(msg, LogDebug);
}

void QmlEngine::setSourceFiles(const QStringList &fileNames)
{
    QMap<QString,QString> files;
    foreach (const QString &file, fileNames) {
        QString shortName = file;
        QString fullName = toFileInProject(file);
        files.insert(shortName, fullName);
    }

    sourceFilesHandler()->setSourceFiles(files);
    //update open editors

}

void QmlEngine::updateScriptSource(const QString &fileName, int lineOffset, int columnOffset,
                                   const QString &source)
{
    QTextDocument *document = 0;
    if (m_sourceDocuments.contains(fileName)) {
        document = m_sourceDocuments.value(fileName);
    } else {
        document = new QTextDocument(this);
        m_sourceDocuments.insert(fileName, document);
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
            cursor.insertText(QLatin1String(" "));
    }
    QTC_CHECK(cursor.positionInBlock() == columnOffset);

    QStringList lines = source.split(QLatin1Char('\n'));
    foreach (QString line, lines) {
        if (line.endsWith(QLatin1Char('\r')))
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
    QString titlePattern = tr("JS Source for %1").arg(fileName);
    //Check if there are open editors with the same title
    foreach (Core::IDocument *doc, Core::DocumentModel::openedDocuments()) {
        if (doc->displayName() == titlePattern) {
            updateDocument(doc, document);
            break;
        }
    }
}

void QmlEngine::updateDocument(Core::IDocument *document, const QTextDocument *textDocument)
{
    TextEditor::TextDocument *baseTextDocument
            = qobject_cast<TextEditor::TextDocument *>(document);
    if (!baseTextDocument)
        return;

    baseTextDocument->document()->setPlainText(textDocument->toPlainText());
}

bool QmlEngine::canEvaluateScript(const QString &script)
{
    m_interpreter.clearText();
    m_interpreter.appendText(script);
    return m_interpreter.canEvaluate();
}

bool QmlEngine::adjustBreakpointLineAndColumn(
        const QString &filePath, quint32 *line, quint32 *column, bool *valid)
{
    bool success = false;
    //check if file is in the latest snapshot
    //ignoring documentChangedOnDisk
    //TODO:: update breakpoints if document is changed.
    ModelManagerInterface *mmIface = ModelManagerInterface::instance();
    if (mmIface) {
        Document::Ptr doc = mmIface->newestSnapshot().
                document(filePath);
        if (doc.isNull()) {
            ModelManagerInterface::instance()->updateSourceFiles(
                        QStringList() << filePath, false);
        } else {
            ASTWalker walker;
            walker(doc->ast(), line, column);
            *valid = walker.done;
            success = true;
        }
    }
    return success;
}

DebuggerEngine *createQmlEngine(const DebuggerStartParameters &sp)
{
    return new QmlEngine(sp);
}

} // namespace Internal
} // namespace Debugger

