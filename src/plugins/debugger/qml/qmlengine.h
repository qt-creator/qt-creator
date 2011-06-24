/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_QMLENGINE_H
#define DEBUGGER_QMLENGINE_H

#include "debuggerengine.h"

#include <utils/outputformat.h>

#include <QtCore/QScopedPointer>
#include <QtNetwork/QAbstractSocket>

namespace Debugger {
namespace Internal {

class QmlEnginePrivate;

class QmlEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    QmlEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine);
    ~QmlEngine();

    void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    void handleRemoteSetupFailed(const QString &message);

    bool canDisplayTooltip() const;

    void showMessage(const QString &msg, int channel = LogDebug,
                     int timeout = -1) const;
    void filterApplicationMessage(const QString &msg, int channel);
    virtual bool acceptsWatchesWhileRunning() const;

public slots:
    void messageReceived(const QByteArray &message);
    void disconnected();

private slots:
    void retryMessageBoxFinished(int result);
    void wrongSetupMessageBoxFinished(int result);

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
    void selectThread(int index);

    void attemptBreakpointSynchronization();
    bool acceptsBreakpoint(BreakpointModelId id) const;

    void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);
    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles() {}
    void reloadFullStack() {}

    bool supportsThreads() const { return false; }
    void updateWatchData(const WatchData &data,
        const WatchUpdateFlags &flags);
    void executeDebuggerCommand(const QString &command);

    unsigned int debuggerCapabilities() const;

signals:
    void sendMessage(const QByteArray &msg);
    void tooltipRequested(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);

private slots:
    void connectionEstablished();
    void connectionStartupFailed();
    void connectionError(QAbstractSocket::SocketError error);
    void serviceConnectionError(const QString &service);
    void appendMessage(const QString &msg, Utils::OutputFormat);

    void synchronizeWatchers();

private:
    void expandObject(const QByteArray &iname, quint64 objectId);
    void sendPing();

    void closeConnection();
    void startApplicationLauncher();
    void stopApplicationLauncher();

    bool isShadowBuildProject() const;
    QString fromShadowBuildFilename(const QString &filename) const;
    QString mangleFilenamePaths(const QString &filename,
        const QString &oldBasePath, const QString &newBasePath) const;
    QString qmlImportPath() const;

    enum LogDirection {
        LogSend,
        LogReceive
    };
    void logMessage(LogDirection direction, const QString &str);
    QString toFileInProject(const QString &file);

private:
    friend class QmlCppEngine;

    QScopedPointer<QmlEnginePrivate> d;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_QMLENGINE_H
