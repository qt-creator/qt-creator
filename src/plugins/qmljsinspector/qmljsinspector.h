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

#ifndef QMLJSINSPECTOR_H
#define QMLJSINSPECTOR_H

#include "qmljsprivateapi.h"

#include <coreplugin/basemode.h>
#include <qmlprojectmanager/qmlprojectrunconfiguration.h>

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QtGui/QAction>
#include <QtCore/QObject>

namespace ProjectExplorer {
    class Project;
    class Environment;
}

namespace Core {
    class IContext;
}

namespace Debugger {
    class DebuggerRunControl;
}

namespace QmlJSInspector {
namespace Internal {

class ClientProxy;
class InspectorContext;

const int MaxConnectionAttempts = 50;
const int ConnectionAttemptDefaultInterval = 75;
// used when debugging with c++ - connection can take a lot of time
const int ConnectionAttemptSimultaneousInterval = 500;

class Inspector : public QObject
{
    Q_OBJECT

public:
    enum DebugMode {
        StandaloneMode,
        CppProjectWithQmlEngines,
        QmlProjectWithCppPlugins
    };

    Inspector(QObject *parent = 0);
    ~Inspector();

    void shutdown();

    bool connectToViewer(); // using host, port from widgets

    // returns false if project is not debuggable.
    bool setDebugConfigurationDataFromProject(ProjectExplorer::Project *projectToDebug);
    void startQmlProjectDebugger();

    QDeclarativeDebugExpressionQuery *executeExpression(int objectDebugId, const QString &objectId,
                                                        const QString &propertyName, const QVariant &value);

    QDeclarativeDebugExpressionQuery *setBindingForObject(int objectDebugId, const QString &objectId,
                                                     const QString &propertyName, const QVariant &value,
                                                     bool isLiteralValue);

signals:
    void statusMessage(const QString &text);

public slots:
    void setSimpleDockWidgetArrangement();
    void reloadQmlViewer();

private slots:
    void gotoObjectReferenceDefinition(const QDeclarativeDebugObjectReference &obj);
    void simultaneouslyDebugQmlCppApplication();

    void debuggerStateChanged(int newState);
    void pollInspector();

    void setSelectedItemsByObjectReference(QList<QDeclarativeDebugObjectReference> objectReferences);
    void changeSelectedItem(int engineId, const QDeclarativeDebugObjectReference &object);

    void updateMenuActions();
    void connected(QDeclarativeEngineDebug *client);
    void aboutToReloadEngines();
    void updateEngineList();

    void disconnectWidgets();
    void disconnected();

private:
    Debugger::DebuggerRunControl *createDebuggerRunControl(ProjectExplorer::RunConfiguration *runConfig,
                                                           const QString &executableFile = QString(),
                                                           const QString &executableArguments = QString());

    QString executeDebuggerRunControl(Debugger::DebuggerRunControl *debuggableRunControl,
                                      ProjectExplorer::Environment *environment);

    QString attachToQmlViewerAsExternalApp(ProjectExplorer::Project *project);
    QString attachToExternalCppAppWithQml(ProjectExplorer::Project *project);

    bool addQuotesForData(const QVariant &value) const;
    void resetViews();

private:
    QWeakPointer<QDeclarativeEngineDebug> m_client;
    InspectorContext *m_context;

    QTimer *m_connectionTimer;
    int m_connectionAttempts;

    QmlProjectManager::QmlProjectRunConfigurationDebugData m_runConfigurationDebugData;

    // simultaneous debug mode stuff
    int m_cppDebuggerState;
    bool m_connectionInitialized;
    bool m_simultaneousCppAndQmlDebugMode;
    DebugMode m_debugMode;
    ClientProxy *m_clientProxy;
};

} // Internal
} // QmlJSInspector

#endif
