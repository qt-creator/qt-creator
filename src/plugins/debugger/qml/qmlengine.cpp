/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlengine.h"
#include "qmladapter.h"
#include "interactiveinterpreter.h"

#include "debuggerstartparameters.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerinternalconstants.h"
#include "debuggermainwindow.h"
#include "debuggerrunner.h"
#include "debuggerstringutils.h"
#include "debuggertooltipmanager.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "sourcefileshandler.h"
#include "watchutils.h"
#include "qtmessageloghandler.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/applicationlauncher.h>
#include <qmljsdebugclient/qdeclarativeoutputparser.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <texteditor/itexteditor.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <QAction>
#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QToolTip>
#include <QTextDocument>

#include <QTcpSocket>
#include <QHostAddress>

#define DEBUG_QML 1
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

using namespace ProjectExplorer;
using namespace QmlJS;
using namespace AST;

namespace Debugger {
namespace Internal {

class QmlEnginePrivate
{
public:
    explicit QmlEnginePrivate(QmlEngine *q);

private:
    friend class QmlEngine;
    QmlAdapter m_adapter;
    ApplicationLauncher m_applicationLauncher;
    QTimer m_noDebugOutputTimer;
    QmlJsDebugClient::QDeclarativeOutputParser m_outputParser;
    QHash<QString, QTextDocument*> m_sourceDocuments;
    QHash<QString, QWeakPointer<TextEditor::ITextEditor> > m_sourceEditors;
    InteractiveInterpreter m_interpreter;
    bool m_validContext;
    QHash<QString,BreakpointModelId> pendingBreakpoints;
    bool m_retryOnConnectFail;
};

QmlEnginePrivate::QmlEnginePrivate(QmlEngine *q)
    : m_adapter(q),
      m_validContext(false),
      m_retryOnConnectFail(false)
{}

class ASTWalker: public Visitor
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

///////////////////////////////////////////////////////////////////////
//
// QmlEngine
//
///////////////////////////////////////////////////////////////////////

QmlEngine::QmlEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine)
  : DebuggerEngine(startParameters, QmlLanguage, masterEngine),
    d(new QmlEnginePrivate(this))
{
    setObjectName(QLatin1String("QmlEngine"));

    ExtensionSystem::PluginManager *pluginManager =
        ExtensionSystem::PluginManager::instance();
    pluginManager->addObject(this);

    connect(&d->m_adapter, SIGNAL(connectionError(QAbstractSocket::SocketError)),
        SLOT(connectionError(QAbstractSocket::SocketError)));
    connect(&d->m_adapter, SIGNAL(serviceConnectionError(QString)),
        SLOT(serviceConnectionError(QString)));
    connect(&d->m_adapter, SIGNAL(connected()),
        SLOT(connectionEstablished()));
    connect(&d->m_adapter, SIGNAL(connectionStartupFailed()),
        SLOT(connectionStartupFailed()));

    connect(this, SIGNAL(stateChanged(Debugger::DebuggerState)),
            SLOT(updateCurrentContext()));
    connect(this->stackHandler(), SIGNAL(currentIndexChanged()),
            SLOT(updateCurrentContext()));
    connect(&d->m_adapter, SIGNAL(selectionChanged()),
            SLOT(updateCurrentContext()));
    connect(d->m_adapter.messageClient(),
            SIGNAL(message(QtMsgType,QString,
                           QmlJsDebugClient::QDebugContextInfo)),
            SLOT(appendDebugOutput(QtMsgType,QString,
                                   QmlJsDebugClient::QDebugContextInfo)));

    connect(&d->m_applicationLauncher,
        SIGNAL(processExited(int)),
        SLOT(disconnected()));
    connect(&d->m_applicationLauncher,
        SIGNAL(appendMessage(QString,Utils::OutputFormat)),
        SLOT(appendMessage(QString,Utils::OutputFormat)));
    connect(&d->m_applicationLauncher,
            SIGNAL(processStarted()),
            &d->m_noDebugOutputTimer,
            SLOT(start()));

    d->m_outputParser.setNoOutputText(ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput());
    connect(&d->m_outputParser, SIGNAL(waitingForConnectionOnPort(quint16)),
            this, SLOT(beginConnection(quint16)));
    connect(&d->m_outputParser, SIGNAL(waitingForConnectionViaOst()),
            this, SLOT(beginConnection()));
    connect(&d->m_outputParser, SIGNAL(noOutputMessage()),
            this, SLOT(tryToConnect()));
    connect(&d->m_outputParser, SIGNAL(errorMessage(QString)),
            this, SLOT(appStartupFailed(QString)));

    // Only wait 8 seconds for the 'Waiting for connection' on application ouput, then just try to connect
    // (application output might be redirected / blocked)
    d->m_noDebugOutputTimer.setSingleShot(true);
    d->m_noDebugOutputTimer.setInterval(8000);
    connect(&d->m_noDebugOutputTimer, SIGNAL(timeout()), this, SLOT(tryToConnect()));

    qtMessageLogHandler()->setHasEditableRow(true);

    connect(ModelManagerInterface::instance(),
            SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
            this,
            SLOT(documentUpdated(QmlJS::Document::Ptr)));

    // we won't get any debug output
    d->m_retryOnConnectFail = startParameters.useTerminal;
}

QmlEngine::~QmlEngine()
{
    ExtensionSystem::PluginManager *pluginManager =
        ExtensionSystem::PluginManager::instance();

    if (pluginManager->allObjects().contains(this)) {
        pluginManager->removeObject(this);
    }

    QList<Core::IEditor *> editorsToClose;

    QHash<QString, QWeakPointer<TextEditor::ITextEditor> >::iterator iter;
    for (iter = d->m_sourceEditors.begin(); iter != d->m_sourceEditors.end(); ++iter) {
        QWeakPointer<TextEditor::ITextEditor> textEditPtr = iter.value();
        if (textEditPtr)
            editorsToClose << textEditPtr.data();
    }
    Core::EditorManager::instance()->closeEditors(editorsToClose);

    delete d;
}

void QmlEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    notifyInferiorSetupOk();
}

void QmlEngine::appendMessage(const QString &msg, Utils::OutputFormat /* format */)
{
    showMessage(msg, AppOutput); // FIXME: Redirect to RunControl
}

void QmlEngine::connectionEstablished()
{
    attemptBreakpointSynchronization();

    if (!watchHandler()->watcherNames().isEmpty()) {
        synchronizeWatchers();
    }
    connect(watchersModel(),SIGNAL(layoutChanged()),this,SLOT(synchronizeWatchers()));

    if (state() == EngineRunRequested)
        notifyEngineRunAndInferiorRunOk();
}

void QmlEngine::tryToConnect(quint16 port)
{
    d->m_retryOnConnectFail = true;
    beginConnection(port);
}

void QmlEngine::beginConnection(quint16 port)
{
    d->m_noDebugOutputTimer.stop();
    if (port > 0) {
        QTC_CHECK(startParameters().communicationChannel
                  == DebuggerStartParameters::CommunicationChannelTcpIp);
        QTC_ASSERT(startParameters().connParams.port == 0
                   || startParameters().connParams.port == port,
                   qWarning() << "Port " << port << "from application output does not match"
                   << startParameters().connParams.port << "from start parameters.")
        d->m_adapter.beginConnectionTcp(startParameters().qmlServerAddress, port);
        return;
    }
    if (startParameters().communicationChannel
           == DebuggerStartParameters::CommunicationChannelTcpIp) {
        // no port from application output, use the one from start parameters ...
        d->m_adapter.beginConnectionTcp(startParameters().qmlServerAddress,
                                        startParameters().qmlServerPort);
    } else {
        QTC_CHECK(startParameters().communicationChannel
                  == DebuggerStartParameters::CommunicationChannelUsb);
        d->m_adapter.beginConnectionOst(startParameters().remoteChannel);
    }
}

void QmlEngine::connectionStartupFailed()
{
    if (isSlaveEngine()) {
        if (masterEngine()->state() != InferiorRunOk) {
            // we're right now debugging C++, just try longer ...
            beginConnection();
            return;
        }
    }
    if (d->m_retryOnConnectFail) {
        beginConnection();
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
    QMessageBox *infoBox = new QMessageBox(Core::ICore::mainWindow());
    infoBox->setIcon(QMessageBox::Critical);
    infoBox->setWindowTitle(tr("Qt Creator"));
    infoBox->setText(tr("Could not connect to the in-process QML debugger."
                        "\n%1").arg(errorMessage));
    infoBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
    infoBox->setDefaultButton(QMessageBox::Ok);
    connect(infoBox, SIGNAL(finished(int)),
            this, SLOT(errorMessageBoxFinished(int)));
    infoBox->show();

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
        Core::HelpManager *helpManager = Core::HelpManager::instance();
        helpManager->handleHelpRequest(QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-debugging-qml.html"));
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

void QmlEngine::connectionError(QAbstractSocket::SocketError socketError)
{
    if (socketError == QAbstractSocket::RemoteHostClosedError)
        showMessage(tr("QML Debugger: Remote host closed connection."), StatusBar);

    if (!isSlaveEngine()) { // normal flow for slave engine when gdb exits
        notifyInferiorSpontaneousStop();
        notifyInferiorIll();
    }
}

void QmlEngine::serviceConnectionError(const QString &serviceName)
{
    showMessage(tr("QML Debugger: Could not connect to service '%1'.")
        .arg(serviceName), StatusBar);
}

bool QmlEngine::canDisplayTooltip() const
{
    return state() == InferiorRunOk || state() == InferiorStopOk;
}

void QmlEngine::filterApplicationMessage(const QString &output, int /*channel*/)
{
    d->m_outputParser.processOutput(output);
}

void QmlEngine::showMessage(const QString &msg, int channel, int timeout) const
{
    if (channel == AppOutput || channel == AppError) {
        const_cast<QmlEngine*>(this)->filterApplicationMessage(msg, channel);
    }
    DebuggerEngine::showMessage(msg, channel, timeout);
}

void QmlEngine::gotoLocation(const Location &location)
{
    const QString fileName = location.fileName();
    // TODO: QUrl::isLocalFile() once we depend on Qt 4.8
    if (QUrl(fileName).scheme().compare(QLatin1String("file"), Qt::CaseInsensitive) == 0) {
        // internal file from source files -> show generated .js
        QTC_ASSERT(d->m_sourceDocuments.contains(fileName), return);
        Core::IEditor *editor = 0;

        Core::EditorManager *editorManager = Core::EditorManager::instance();
        QString titlePattern = tr("JS Source for %1").arg(fileName);
        //Check if there are open editors with the same title
        QList<Core::IEditor *> editors = editorManager->openedEditors();
        foreach (Core::IEditor *ed, editors) {
            if (ed->displayName() == titlePattern) {
                editor = ed;
                break;
            }
        }
        if (!editor) {
            editor = editorManager->openEditorWithContents(QmlJSEditor::Constants::C_QMLJSEDITOR_ID,
                                                           &titlePattern);
            if (editor) {
                editor->setProperty(Constants::OPENED_BY_DEBUGGER, true);
            }

            updateEditor(editor, d->m_sourceDocuments.value(fileName));
        }
        editorManager->activateEditor(editor);

    } else {
        DebuggerEngine::gotoLocation(location);
    }
}

void QmlEngine::closeConnection()
{
    disconnect(watchersModel(),SIGNAL(layoutChanged()),this,SLOT(synchronizeWatchers()));
    d->m_adapter.closeConnection();
}

void QmlEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    if (!isSlaveEngine()) {
        if (startParameters().startMode == AttachToRemoteServer)
            beginConnection();
        else
            startApplicationLauncher();
    } else {
        d->m_noDebugOutputTimer.start();
    }
}

void QmlEngine::startApplicationLauncher()
{
    if (!d->m_applicationLauncher.isRunning()) {
        appendMessage(tr("Starting %1 %2").arg(
                          QDir::toNativeSeparators(startParameters().executable),
                          startParameters().processArgs)
                      + QLatin1Char('\n')
                     , Utils::NormalMessageFormat);
        d->m_applicationLauncher.start(ApplicationLauncher::Gui,
                                    startParameters().executable,
                                    startParameters().processArgs);
    }
}

void QmlEngine::stopApplicationLauncher()
{
    if (d->m_applicationLauncher.isRunning()) {
        disconnect(&d->m_applicationLauncher, SIGNAL(processExited(int)), this, SLOT(disconnected()));
        d->m_applicationLauncher.stop();
    }
}

void QmlEngine::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    Q_UNUSED(gdbServerPort);

    if (qmlPort != -1)
        startParameters().qmlServerPort = qmlPort;

    notifyEngineRemoteSetupDone();
    notifyEngineSetupOk();
}

void QmlEngine::handleRemoteSetupFailed(const QString &message)
{
    if (isMasterEngine())
        QMessageBox::critical(0,tr("Failed to start application"),
            tr("Application startup failed: %1").arg(message));

    notifyEngineRemoteSetupFailed();
    notifyEngineSetupFailed();
}

void QmlEngine::shutdownInferior()
{
    d->m_noDebugOutputTimer.stop();

    if (d->m_adapter.activeDebuggerClient())
        d->m_adapter.activeDebuggerClient()->endSession();

    if (isSlaveEngine()) {
        resetLocation();
    }
    stopApplicationLauncher();
    closeConnection();

    notifyInferiorShutdownOk();
}

void QmlEngine::shutdownEngine()
{
    // double check (ill engine?):
    stopApplicationLauncher();

    notifyEngineShutdownOk();
    if (!isSlaveEngine())
        showMessage(QString(), StatusBar);
}

void QmlEngine::setupEngine()
{
    if (startParameters().requestRemoteSetup) {
        // we need to get the port first
        notifyEngineRequestRemoteSetup();
    } else {
        d->m_applicationLauncher.setEnvironment(startParameters().environment);
        d->m_applicationLauncher.setWorkingDirectory(startParameters().workingDirectory);

        // We can't do this in the constructore because runControl() isn't yet defined
        connect(&d->m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
                runControl(), SLOT(bringApplicationToForeground(qint64)),
                Qt::UniqueConnection);

        notifyEngineSetupOk();
    }
}

void QmlEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->continueInferior();
    }
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::interruptInferior()
{
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->interruptInferior();
    }
    notifyInferiorStopOk();
}

void QmlEngine::executeStep()
{
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->executeStep();
    }
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepI()
{
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->executeStepI();
    }
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepOut()
{
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->executeStepOut();
    }
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNext()
{
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->executeNext();
    }
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
    showStatusMessage(tr("Run to line  %1 (%2) requested...").arg(data.lineNumber).arg(data.fileName), 5000);
    resetLocation();
    ContextData modifiedData = data;
    quint32 line = data.lineNumber;
    quint32 column;
    bool valid;
    if (adjustBreakpointLineAndColumn(data.fileName, &line, &column, &valid))
        modifiedData.lineNumber = line;
    if (d->m_adapter.activeDebuggerClient())
        d->m_adapter.activeDebuggerClient()->executeRunToLine(modifiedData);
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

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->activateFrame(index);
    }
    gotoLocation(stackHandler()->frames().value(index));
}

void QmlEngine::selectThread(int index)
{
    Q_UNUSED(index)
}

void QmlEngine::insertBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    BreakpointState state = handler->state(id);
    QTC_ASSERT(state == BreakpointInsertRequested, qDebug() << id << this << state);
    handler->notifyBreakpointInsertProceeding(id);

    const BreakpointParameters &params = handler->breakpointData(id);
    quint32 line = params.lineNumber;
    quint32 column = 0;
    if (params.type == BreakpointByFileAndLine) {
        bool valid = false;
        if (!adjustBreakpointLineAndColumn(params.fileName, &line, &column,
                                           &valid)) {
            d->pendingBreakpoints.insertMulti(params.fileName, id);
            return;
        }
        if (!valid)
            return;
    }

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->insertBreakpoint(id, line, column);
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients()) {
            client->insertBreakpoint(id, line, column);
        }
    }
}

void QmlEngine::removeBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();

    const BreakpointParameters &params = handler->breakpointData(id);
    if (params.type == BreakpointByFileAndLine &&
            d->pendingBreakpoints.contains(params.fileName)) {
        QHash<QString, BreakpointModelId>::iterator i =
                d->pendingBreakpoints.find(params.fileName);
        while (i != d->pendingBreakpoints.end() && i.key() == params.fileName) {
            if (i.value() == id) {
                d->pendingBreakpoints.erase(i);
                return;
            }
            ++i;
        }
    }

    BreakpointState state = handler->state(id);
    QTC_ASSERT(state == BreakpointRemoveRequested, qDebug() << id << this << state);
    handler->notifyBreakpointRemoveProceeding(id);

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->removeBreakpoint(id);
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients()) {
            client->removeBreakpoint(id);
        }
    }

    if (handler->state(id) == BreakpointRemoveProceeding) {
        handler->notifyBreakpointRemoveOk(id);
    }
}

void QmlEngine::changeBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    BreakpointState state = handler->state(id);
    QTC_ASSERT(state == BreakpointChangeRequested, qDebug() << id << this << state);
    handler->notifyBreakpointChangeProceeding(id);

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->changeBreakpoint(id);
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients()) {
            client->changeBreakpoint(id);
        }
    }

    if (handler->state(id) == BreakpointChangeProceeding) {
        handler->notifyBreakpointChangeOk(id);
    }
}

void QmlEngine::attemptBreakpointSynchronization()
{
    if (!stateAcceptsBreakpointChanges()) {
        showMessage(_("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE"));
        return;
    }

    BreakHandler *handler = breakHandler();

    DebuggerEngine *bpOwner = isSlaveEngine() ? masterEngine() : this;
    foreach (BreakpointModelId id, handler->unclaimedBreakpointIds()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(id))
            handler->setEngine(id, bpOwner);
    }

    foreach (BreakpointModelId id, handler->engineBreakpointIds(bpOwner)) {
        switch (handler->state(id)) {
        case BreakpointNew:
            // Should not happen once claimed.
            QTC_CHECK(false);
            continue;
        case BreakpointInsertRequested:
            insertBreakpoint(id);
            continue;
        case BreakpointChangeRequested:
            changeBreakpoint(id);
            continue;
        case BreakpointRemoveRequested:
            removeBreakpoint(id);
            continue;
        case BreakpointChangeProceeding:
        case BreakpointInsertProceeding:
        case BreakpointRemoveProceeding:
        case BreakpointInserted:
        case BreakpointDead:
            continue;
        }
        QTC_ASSERT(false, qDebug() << "UNKNOWN STATE"  << id << state());
    }

    DebuggerEngine::attemptBreakpointSynchronization();

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->synchronizeBreakpoints();
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients()) {
            client->synchronizeBreakpoints();
        }
    }
}

bool QmlEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    if (!breakHandler()->breakpointData(id).isCppBreakpoint())
            return true;

    //If it is a Cpp Breakpoint query if the type can be also handled by the debugger client
    //TODO: enable setting of breakpoints before start of debug session
    //For now, the event breakpoint can be set after the activeDebuggerClient is known
    //This is because the older client does not support BreakpointOnQmlSignalHandler
    bool acceptBreakpoint = false;
    if (d->m_adapter.activeDebuggerClient()) {
        acceptBreakpoint = d->m_adapter.activeDebuggerClient()->acceptsBreakpoint(id);
    }
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
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->getSourceFiles();
    }
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

bool QmlEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, const DebuggerToolTipContext &ctx)
{
    // This is processed by QML inspector, which has dependencies to
    // the qml js editor. Makes life easier.
    emit tooltipRequested(mousePos, editor, ctx.position);
    return true;
}

//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void QmlEngine::assignValueInDebugger(const WatchData *data,
    const QString &expression, const QVariant &valueV)
{
    if (!expression.isEmpty() && d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->assignValueInDebugger(data,
                                                                   expression,
                                                                   valueV);
    }
}

void QmlEngine::updateWatchData(const WatchData &data,
    const WatchUpdateFlags &)
{
//    qDebug() << "UPDATE WATCH DATA" << data.toString();
    //watchHandler()->rebuildModel();
    showStatusMessage(tr("Stopped."), 5000);

    if (!data.name.isEmpty() && d->m_adapter.activeDebuggerClient()) {
        if (data.isValueNeeded()) {
            d->m_adapter.activeDebuggerClient()->updateWatchData(data);
        }
        if (data.isChildrenNeeded()
                && watchHandler()->isExpandedIName(data.iname)) {
            d->m_adapter.activeDebuggerClient()->expandObject(data.iname, data.id);
        }
    }

    synchronizeWatchers();

    if (!data.isSomethingNeeded())
        watchHandler()->insertData(data);
}

void QmlEngine::synchronizeWatchers()
{
    QStringList watchedExpressions = watchHandler()->watchedExpressions();
    // send watchers list
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->synchronizeWatchers(watchedExpressions);
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients())
            client->synchronizeWatchers(watchedExpressions);
    }
}

void QmlEngine::onDebugQueryStateChanged(
        QmlJsDebugClient::QDeclarativeDebugQuery::State state)
{
    QmlJsDebugClient::QDeclarativeDebugExpressionQuery *query =
            qobject_cast<QmlJsDebugClient::QDeclarativeDebugExpressionQuery *>(
                sender());
    if (query && state != QmlJsDebugClient::QDeclarativeDebugQuery::Error) {
        QtMessageLogItem *item = constructLogItemTree(query->result());
        if (item)
            qtMessageLogHandler()->appendItem(item);
    } else
        qtMessageLogHandler()->
                appendItem(new QtMessageLogItem(QtMessageLogHandler::ErrorType,
                                             _("Error evaluating expression.")));
    delete query;
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

void QmlEngine::documentUpdated(QmlJS::Document::Ptr doc)
{
    QString fileName = doc->fileName();
    if (d->pendingBreakpoints.contains(fileName)) {
        QList<BreakpointModelId> ids = d->pendingBreakpoints.values(fileName);
        d->pendingBreakpoints.remove(fileName);
        foreach (const BreakpointModelId &id, ids)
            insertBreakpoint(id);
    }
}

void QmlEngine::updateCurrentContext()
{
    const QString context = state() == InferiorStopOk ?
                stackHandler()->currentFrame().function :
                d->m_adapter.currentSelectedDisplayName();
    d->m_validContext = !context.isEmpty();
    showMessage(tr("Context: ").append(context), QtMessageLogStatus);
}

void QmlEngine::appendDebugOutput(QtMsgType type, const QString &message,
                                  const QmlJsDebugClient::QDebugContextInfo &info)
{
    QtMessageLogHandler::ItemType itemType;
    switch (type) {
    case QtDebugMsg:
        itemType = QtMessageLogHandler::DebugType;
        break;
    case QtWarningMsg:
        itemType = QtMessageLogHandler::WarningType;
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        itemType = QtMessageLogHandler::ErrorType;
        break;
    default:
        //This case is not possible
        return;
    }
    QtMessageLogItem *item = new QtMessageLogItem(itemType, message);
    item->file = info.file;
    item->line = info.line;
    qtMessageLogHandler()->appendItem(item);
}

void QmlEngine::executeDebuggerCommand(const QString& command)
{
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->executeDebuggerCommand(command);
    }
}

bool QmlEngine::evaluateScriptExpression(const QString& expression)
{
    bool didEvaluate = true;
    //Check if string is only white spaces
    if (!expression.trimmed().isEmpty()) {
        //Check for a valid context
        if (d->m_validContext) {
            //check if it can be evaluated
            if (canEvaluateScript(expression)) {
                //Evaluate expression based on engine state
                //When engine->state() == InferiorStopOk, the expression
                //is sent to V8DebugService. In all other cases, the
                //expression is evaluated by QDeclarativeEngine.
                if (state() != InferiorStopOk) {
                    QDeclarativeEngineDebug *engineDebug =
                            d->m_adapter.engineDebugClient();

                    int id = d->m_adapter.currentSelectedDebugId();
                    if (engineDebug && id != -1) {
                        QDeclarativeDebugExpressionQuery *query =
                                engineDebug->queryExpressionResult(id, expression);
                        connect(query,
                                SIGNAL(stateChanged(
                                           QmlJsDebugClient::QDeclarativeDebugQuery
                                           ::State)),
                                this,
                                SLOT(onDebugQueryStateChanged(
                                         QmlJsDebugClient::QDeclarativeDebugQuery
                                         ::State)));
                    }
                } else {
                    executeDebuggerCommand(expression);
                }
            } else {
                didEvaluate = false;
            }
        } else {
            //Incase of invalid context, show Error message
            qtMessageLogHandler()->
                            appendItem(new QtMessageLogItem(
                                           QtMessageLogHandler::ErrorType,
                                           _("Cannot evaluate without"
                                             "a valid QML/JS Context.")),
                                       qtMessageLogHandler()->rowCount());
        }
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
    if (d->m_sourceDocuments.contains(fileName)) {
        document = d->m_sourceDocuments.value(fileName);
    } else {
        document = new QTextDocument(this);
        d->m_sourceDocuments.insert(fileName, document);
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
    QList<Core::IEditor *> editors = Core::EditorManager::instance()->openedEditors();
    foreach (Core::IEditor *editor, editors) {
        if (editor->displayName() == titlePattern) {
            updateEditor(editor, document);
            break;
        }
    }
}

void QmlEngine::updateEditor(Core::IEditor *editor, const QTextDocument *document)
{
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);
    if (!textEditor)
        return;

    QPlainTextEdit *plainTextEdit =
            qobject_cast<QPlainTextEdit *>(editor->widget());
    if (!plainTextEdit)
        return;
    plainTextEdit->setPlainText(document->toPlainText());
    plainTextEdit->setReadOnly(true);
}

bool QmlEngine::canEvaluateScript(const QString &script)
{
    d->m_interpreter.clearText();
    d->m_interpreter.appendText(script);
    return d->m_interpreter.canEvaluate();
}

QtMessageLogItem *QmlEngine::constructLogItemTree(
        const QVariant &result, const QString &key)
{
    if (!result.isValid())
        return 0;

    QtMessageLogItem *item = new QtMessageLogItem();
    if (result.type() == QVariant::Map) {
        if (key.isEmpty())
            item->text = _("Object");
        else
            item->text = QString(_("%1: Object")).arg(key);

        QMapIterator<QString, QVariant> i(result.toMap());
        while (i.hasNext()) {
            i.next();
            QtMessageLogItem *child = constructLogItemTree(i.value(), i.key());
            if (child)
                item->insertChild(item->childCount(), child);
        }
    } else if (result.type() == QVariant::List) {
        if (key.isEmpty())
            item->text = _("List");
        else
            item->text = QString(_("[%1] : List")).arg(key);
        QVariantList resultList = result.toList();
        for (int i = 0; i < resultList.count(); i++) {
            QtMessageLogItem *child = constructLogItemTree(resultList.at(i),
                                                          QString::number(i));
            if (child)
                item->insertChild(item->childCount(), child);
        }
    } else if (result.canConvert(QVariant::String)) {
        item->text = result.toString();
    } else {
        item->text = _("Unknown Value");
    }

    return item;
}

bool QmlEngine::adjustBreakpointLineAndColumn(
        const QString &filePath, quint32 *line, quint32 *column, bool *valid)
{
    bool success = true;
    //check if file is in the latest snapshot
    //ignoring documentChangedOnDisk
    //TODO:: update breakpoints if document is changed.
    Document::Ptr doc = ModelManagerInterface::instance()->newestSnapshot().
            document(filePath);
    if (doc.isNull()) {
        ModelManagerInterface::instance()->updateSourceFiles(
                    QStringList() << filePath, false);
        success = false;
    } else {
        ASTWalker walker;
        walker(doc->ast(), line, column);
        *valid = walker.done;
    }
    return success;
}

QmlAdapter *QmlEngine::adapter() const
{
    return &d->m_adapter;
}

QmlEngine *createQmlEngine(const DebuggerStartParameters &sp,
    DebuggerEngine *masterEngine)
{
    return new QmlEngine(sp, masterEngine);
}

} // namespace Internal
} // namespace Debugger

