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

#ifndef QMLENGINE_H
#define QMLENGINE_H

#include "interactiveinterpreter.h"
#include "qmladapter.h"
#include "qmlinspectoradapter.h"
#include <debugger/debuggerengine.h>

#include <projectexplorer/applicationlauncher.h>
#include <qmldebug/qdebugmessageclient.h>
#include <qmldebug/qmloutputparser.h>
#include <qmljs/iscriptevaluator.h>
#include <qmljs/qmljsdocument.h>

QT_FORWARD_DECLARE_CLASS(QTextDocument)

namespace Core { class IDocument; }

namespace TextEditor { class BaseTextEditor; }

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

    void notifyEngineRemoteServerRunning(const QByteArray &, int pid);
    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result);

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

    void insertBreakpoint(Breakpoint bp);

signals:
    void tooltipRequested(const DebuggerToolTipContext &context);

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
    void connectionError(QDebugSupport::Error error);
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

    bool setToolTipExpression(const DebuggerToolTipContext &);

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);

    void activateFrame(int index);
    void selectThread(ThreadId threadId);

    void attemptBreakpointSynchronization();
    void removeBreakpoint(Breakpoint bp);
    void changeBreakpoint(Breakpoint bp);
    bool acceptsBreakpoint(Breakpoint bp) const;

    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles();
    void reloadFullStack() {}

    bool supportsThreads() const { return false; }
    void updateWatchItem(WatchItem *item);
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

    void updateDocument(Core::IDocument *document, const QTextDocument *textDocument);
    bool canEvaluateScript(const QString &script);
    bool adjustBreakpointLineAndColumn(const QString &filePath, quint32 *line,
                                       quint32 *column, bool *valid);

    QmlAdapter m_adapter;
    QmlInspectorAdapter m_inspectorAdapter;
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    QTimer m_noDebugOutputTimer;
    QmlDebug::QmlOutputParser m_outputParser;
    QHash<QString, QTextDocument*> m_sourceDocuments;
    QHash<QString, QWeakPointer<TextEditor::BaseTextEditor> > m_sourceEditors;
    InteractiveInterpreter m_interpreter;
    QHash<QString,Breakpoint> pendingBreakpoints;
    QList<quint32> queryIds;
    bool m_retryOnConnectFail;
    bool m_automaticConnect;

    friend class QmlCppEngine;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLENGINE_H
