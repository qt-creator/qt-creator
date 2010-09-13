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

#ifndef DEBUGGER_QMLENGINE_H
#define DEBUGGER_QMLENGINE_H

#include "debuggerengine.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QProcess>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>

#include <projectexplorer/applicationlauncher.h>

namespace Core {
    class TextEditor;
}

namespace Debugger {
namespace Internal {

class ScriptAgent;
class WatchData;
class QmlAdapter;
class QmlResponse;
class QmlDebuggerClient;

class DEBUGGER_EXPORT QmlEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit QmlEngine(const DebuggerStartParameters &startParameters);
    ~QmlEngine();

    void setAttachToRunningExternalApp(bool value);
    void shutdownInferiorAsSlave();
    void shutdownEngineAsSlave();
    void pauseConnection();
    void gotoLocation(const QString &fileName, int lineNumber, bool setMarker);
    void gotoLocation(const StackFrame &frame, bool setMarker);

public slots:
    void messageReceived(const QByteArray &message);
    void disconnected();

private:
    // DebuggerEngine implementation
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

    void setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos);

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const QString &fileName, int lineNumber);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const QString &fileName, int lineNumber);

    void activateFrame(int index);
    void selectThread(int index);

    void attemptBreakpointSynchronization();

    void assignValueInDebugger(const QString &expr, const QString &value);
    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles() {}
    void reloadFullStack() {}

    bool supportsThreads() const { return false; }
    void updateWatchData(const WatchData &data, const WatchUpdateFlags &flags);
    void executeDebuggerCommand(const QString& command);

    unsigned int debuggerCapabilities() const;

signals:
    void sendMessage(const QByteArray &msg);
    void tooltipRequested(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos);

private slots:
    void connectionEstablished();
    void connectionStartupFailed();
    void connectionError(QAbstractSocket::SocketError error);

    void slotMessage(QString, bool);
    void slotAddToOutputWindow(QString, bool);
private:
    void expandObject(const QByteArray &iname, quint64 objectId);
    void sendPing();

    bool isShadowBuildProject() const;
    QString fromShadowBuildFilename(const QString &filename) const;
    QString mangleFilenamePaths(const QString &filename, const QString &oldBasePath, const QString &newBasePath) const;
    QString toShadowBuildFilename(const QString &filename) const;
    QString qmlImportPath() const;

private:
    friend class QmlCppEngine;

    int m_ping;
    QmlAdapter *m_adapter;
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    bool m_addedAdapterToObjectPool;
    bool m_attachToRunningExternalApp;
    bool m_hasShutdown;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_QMLENGINE_H
