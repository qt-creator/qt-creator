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

#ifndef QMLENGINE_H
#define QMLENGINE_H

#include "debuggerengine.h"
#include "interactiveinterpreter.h"
#include "qmladapter.h"
#include "qmlinspectoradapter.h"

#include <projectexplorer/applicationlauncher.h>
#include <qmldebug/qdebugmessageclient.h>
#include <qmldebug/qmloutputparser.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/iscriptevaluator.h>
#include <utils/outputformat.h>

#include <QAbstractSocket>
#include <QTextDocument>

namespace Core {
class IEditor;
}

namespace Debugger {
namespace Internal {

class QmlAdapter;
class WatchTreeView;

class QmlEngine : public DebuggerEngine, QmlJS::IScriptEvaluator
{
    Q_OBJECT

public:
    explicit QmlEngine(const DebuggerStartParameters &startParameters,
                       DebuggerEngine *masterEngine = 0);
    ~QmlEngine();

    void notifyEngineRemoteSetupDone(int gdbServerPort, int qmlPort);
    void notifyEngineRemoteSetupFailed(const QString &message);

    bool canDisplayTooltip() const;

    void showMessage(const QString &msg, int channel = LogDebug,
                     int timeout = -1) const;
    void gotoLocation(const Internal::Location &location);

    void filterApplicationMessage(const QString &msg, int channel);
    void inferiorSpontaneousStop();

    enum LogDirection {
        LogSend,
        LogReceive
    };

    void logMessage(const QString &service, LogDirection direction,
                    const QString &str);

    void setSourceFiles(const QStringList &fileNames);
    void updateScriptSource(const QString &fileName, int lineOffset,
                            int columnOffset, const QString &source);

    void insertBreakpoint(BreakpointModelId id);

signals:
    void tooltipRequested(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);

private slots:
    void disconnected();
    void documentUpdated(QmlJS::Document::Ptr doc);
    void expressionEvaluated(quint32 queryId, const QVariant &result);

    void errorMessageBoxFinished(int result);
    void updateCurrentContext();
    void appendDebugOutput(QtMsgType type, const QString &message,
                           const QmlDebug::QDebugContextInfo &info);

    void tryToConnect(quint16 port = 0);
    void beginConnection(quint16 port = 0);
    void connectionEstablished();
    void connectionStartupFailed();
    void appStartupFailed(const QString &errorMessage);
    void connectionError(QAbstractSocket::SocketError error);
    void serviceConnectionError(const QString &service);
    void appendMessage(const QString &msg, Utils::OutputFormat);

    void synchronizeWatchers();

private:
    // DebuggerEngine implementation.
    bool isSynchronous() const { return false; }
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    void setupEngine();
    void setupInferior();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();

    bool setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, const DebuggerToolTipContext &);

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);

    void activateFrame(int index);
    void selectThread(ThreadId threadId);

    void attemptBreakpointSynchronization();
    void removeBreakpoint(BreakpointModelId id);
    void changeBreakpoint(BreakpointModelId id);
    bool acceptsBreakpoint(BreakpointModelId id) const;

    void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);


    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles();
    void reloadFullStack() {}

    bool supportsThreads() const { return false; }
    void updateWatchData(const WatchData &data,
        const WatchUpdateFlags &flags);
    void watchDataSelected(const QByteArray &iname);
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);
    bool evaluateScript(const QString &expression);

    bool hasCapability(unsigned) const;
    void quitDebugger();

private:
    void closeConnection();
    void startApplicationLauncher();
    void stopApplicationLauncher();

    bool isShadowBuildProject() const;
    QString fromShadowBuildFilename(const QString &filename) const;
    QString mangleFilenamePaths(const QString &filename,
        const QString &oldBasePath, const QString &newBasePath) const;
    QString qmlImportPath() const;

    void updateEditor(Core::IEditor *editor, const QTextDocument *document);
    bool canEvaluateScript(const QString &script);
    bool adjustBreakpointLineAndColumn(const QString &filePath, quint32 *line,
                                       quint32 *column, bool *valid);

    WatchTreeView *inspectorTreeView() const;

    QmlAdapter m_adapter;
    QmlInspectorAdapter m_inspectorAdapter;
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    QTimer m_noDebugOutputTimer;
    QmlDebug::QmlOutputParser m_outputParser;
    QHash<QString, QTextDocument*> m_sourceDocuments;
    QHash<QString, QWeakPointer<TextEditor::ITextEditor> > m_sourceEditors;
    InteractiveInterpreter m_interpreter;
    QHash<QString,BreakpointModelId> pendingBreakpoints;
    QList<quint32> queryIds;
    bool m_retryOnConnectFail;
    bool m_automaticConnect;

    friend class QmlCppEngine;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLENGINE_H
