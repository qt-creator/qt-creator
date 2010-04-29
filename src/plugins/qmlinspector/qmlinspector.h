/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef QMLINSPECTORMODE_H
#define QMLINSPECTORMODE_H

#include "qmlinspector_global.h"
#include "inspectorsettings.h"
#include <coreplugin/basemode.h>
#include <qmlprojectmanager/qmlprojectrunconfiguration.h>

#include <QtGui/QAction>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

class QDockWidget;
class QToolButton;
class QLineEdit;
class QSpinBox;
class QLabel;

class QDeclarativeEngineDebug;
class QDeclarativeDebugConnection;
class QDeclarativeDebugExpressionQuery;
class QDeclarativeDebugEnginesQuery;
class QDeclarativeDebugRootContextQuery;
class QDeclarativeDebugObjectReference;
QT_END_NAMESPACE


namespace ProjectExplorer {
    class Project;
    class Environment;
}

namespace Core {
    class IContext;
}
namespace Debugger {
namespace Internal {
    class DebuggerRunControl;
} // Internal
} // Debugger

namespace Qml {

    namespace Internal {
        class EngineComboBox;
        class InspectorContext;
        class ObjectTree;
        class ObjectPropertiesView;
        class WatchTableModel;
        class WatchTableView;
        class CanvasFrameRate;
        class ExpressionQueryWidget;
    }

const int MaxConnectionAttempts = 50;
const int ConnectionAttemptDefaultInterval = 75;
// used when debugging with c++ - connection can take a lot of time
const int ConnectionAttemptSimultaneousInterval = 500;

class QMLINSPECTOR_EXPORT QmlInspector : public QObject
{
    Q_OBJECT

public:
    enum DebugMode {
        StandaloneMode,
        CppProjectWithQmlEngines,
        QmlProjectWithCppPlugins
    };

    QmlInspector(QObject *parent = 0);
    ~QmlInspector();
    void shutdown();

    void createDockWidgets();
    bool connectToViewer(); // using host, port from widgets
    Core::IContext *context() const;

    // returns false if project is not debuggable.
    bool setDebugConfigurationDataFromProject(ProjectExplorer::Project *projectToDebug);
    void startQmlProjectDebugger();

    static QmlInspector *instance();
    bool canEditProperty(const QString &propertyType);
    QDeclarativeDebugExpressionQuery *executeExpression(int objectDebugId, const QString &objectId,
                                                        const QString &propertyName, const QVariant &value);

signals:
    void statusMessage(const QString &text);

public slots:
    void disconnectFromViewer();
    void setSimpleDockWidgetArrangement();

private slots:
    void connectionStateChanged();
    void connectionError();
    void reloadEngines();
    void enginesChanged();
    void queryEngineContext(int);
    void contextChanged();
    void treeObjectActivated(const QDeclarativeDebugObjectReference &obj);
    void simultaneouslyDebugQmlCppApplication();

    void debuggerStateChanged(int newState);
    void pollInspector();

private:
    void updateMenuActions();
    Debugger::Internal::DebuggerRunControl *createDebuggerRunControl(ProjectExplorer::RunConfiguration *runConfig,
                                                                     const QString &executableFile = QString(),
                                                                     const QString &executableArguments = QString());
    QString executeDebuggerRunControl(Debugger::Internal::DebuggerRunControl *debuggableRunControl, ProjectExplorer::Environment *environment);
    QString attachToQmlViewerAsExternalApp(ProjectExplorer::Project *project);
    QString attachToExternalCppAppWithQml(ProjectExplorer::Project *project);

    bool addQuotesForData(const QVariant &value) const;
    void resetViews();

    QDeclarativeDebugConnection *m_conn;
    QDeclarativeEngineDebug *m_client;

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
    QList<QDockWidget*> m_dockWidgets;

    Internal::InspectorContext *m_context;
    Internal::InspectorContext *m_propWatcherContext;

    QAction *m_simultaneousDebugAction;

    QTimer *m_connectionTimer;
    int m_connectionAttempts;

    Internal::InspectorSettings m_settings;
    QmlProjectManager::QmlProjectRunConfigurationDebugData m_runConfigurationDebugData;

    QStringList m_editablePropertyTypes;

    // simultaneous debug mode stuff
    int m_cppDebuggerState;
    bool m_connectionInitialized;
    bool m_simultaneousCppAndQmlDebugMode;
    DebugMode m_debugMode;

    static QmlInspector *m_instance;
};

} // Qml

#endif
