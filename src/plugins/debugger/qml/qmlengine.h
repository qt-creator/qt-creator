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


namespace Debugger {
namespace Internal {

class ScriptAgent;
class WatchData;
class QmlResponse;
class CanvasFrameRate;
class QmlDebuggerClient;

class DEBUGGER_EXPORT QmlEngine : public DebuggerEngine
{
    Q_OBJECT

    int m_ping;
public:
    explicit QmlEngine(const DebuggerStartParameters &startParameters);
    ~QmlEngine();

    void messageReceived(const QByteArray &message);

private:
    // DebuggerEngine implementation
    bool isSynchroneous() const { return true; }
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

    bool supportsThreads() const { return true; }
    void updateWatchData(const WatchData &data);

    unsigned int debuggerCapabilities() const;

    void setupConnection();
signals:
    void sendMessage(const QByteArray &msg);

private:
    void expandObject(const QByteArray &iname, quint64 objectId);
    void sendPing();

#if 0
    void createDockWidgets();
    bool connectToViewer(); // using host, port from widgets

    // returns false if project is not debuggable.
    bool setDebugConfigurationDataFromProject(ProjectExplorer::Project *projectToDebug);
    void startQmlProjectDebugger();

    bool canEditProperty(const QString &propertyType);
    QDeclarativeDebugExpressionQuery *executeExpression(int objectDebugId,
        const QString &objectId, const QString &propertyName, const QVariant &value);

public slots:
    void disconnectFromViewer();
    void setSimpleDockWidgetArrangement();

private slots:
    void treeObjectActivated(const QDeclarativeDebugObjectReference &obj);
    void simultaneouslyDebugQmlCppApplication();

private:
    void updateMenuActions();
    QString attachToQmlViewerAsExternalApp(ProjectExplorer::Project *project);
    QString attachToExternalCppAppWithQml(ProjectExplorer::Project *project);

    bool addQuotesForData(const QVariant &value) const;
    void resetViews();

    QDeclarativeDebugEnginesQuery *m_engineQuery;
    QDeclarativeDebugRootContextQuery *m_contextQuery;

    Internal::ObjectTree *m_objectTreeWidget;
    Internal::ObjectPropertiesView *m_propertiesWidget;
    Internal::WatchTableModel *m_watchTableModel;
    Internal::WatchTableView *m_watchTableView;
    Internal::CanvasFrameRate *m_frameRateWidget;
    Internal::ExpressionQueryWidget *m_expressionWidget;

    Internal::EngineComboBox *m_engineComboBox;

    QDockWidget *m_objectTreeDock;
    QDockWidget *m_frameRateDock;
    QDockWidget *m_expressionQueryDock;
    QDockWidget *m_propertyWatcherDock;
    QDockWidget *m_inspectorOutputDock;

    Internal::InspectorSettings m_settings;
    QmlProjectManager::QmlProjectRunConfigurationDebugData m_runConfigurationDebugData;

    QStringList m_editablePropertyTypes;

    // simultaneous debug mode stuff
    int m_cppDebuggerState;
    bool m_connectionInitialized;
    bool m_simultaneousCppAndQmlDebugMode;
#endif
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_QMLENGINE_H
