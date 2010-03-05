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
#include "qmlinspectorconstants.h"
#include "qmlinspector.h"
#include "qmlinspectorplugin.h"

#include <debugger/debuggeruiswitcher.h>
#include <debugger/debuggerconstants.h>

#include <qmlprojectmanager/qmlproject.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <private/qdeclarativedebug_p.h>
#include <private/qdeclarativedebugclient_p.h>

#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QStringList>
#include <QtCore/QtPlugin>
#include <QtCore/QTimer>

#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>

#include <QtCore/QDebug>


using namespace Qml;

static QToolButton *createToolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

QmlInspectorPlugin::QmlInspectorPlugin()
    : m_inspector(0), m_connectionTimer(new QTimer(this)),
    m_connectionAttempts(0)
{
    m_connectionTimer->setInterval(75);
}

QmlInspectorPlugin::~QmlInspectorPlugin()
{
}

void QmlInspectorPlugin::shutdown()
{
    removeObject(m_inspector);
    delete m_inspector;
    m_inspector = 0;
}

bool QmlInspectorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            SLOT(prepareDebugger(Core::IMode*)));

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();
    uiSwitcher->addLanguage(Qml::Constants::LANG_QML);

    m_inspector = new QmlInspector;
    addObject(m_inspector);

    connect(m_connectionTimer, SIGNAL(timeout()), SLOT(pollInspector()));

    return true;
}

void QmlInspectorPlugin::extensionsInitialized()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();
    //connect(uiSwitcher, SIGNAL(languageChanged(QString)), SLOT(activateDebugger(QString)));
    connect(uiSwitcher, SIGNAL(dockArranged(QString)), SLOT(setDockWidgetArrangement(QString)));

    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();
    if (pex) {
        connect(pex, SIGNAL(aboutToExecuteProject(ProjectExplorer::Project*)),
                SLOT(activateDebuggerForProject(ProjectExplorer::Project*)));
    }

    QWidget *configBar = new QWidget;
    configBar->setProperty("topBorder", true);

    QHBoxLayout *configBarLayout = new QHBoxLayout(configBar);
    configBarLayout->setMargin(0);
    configBarLayout->setSpacing(5);

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();
    configBarLayout->addWidget(createToolButton(am->command(ProjectExplorer::Constants::DEBUG)->action()));
    configBarLayout->addWidget(createToolButton(am->command(ProjectExplorer::Constants::STOP)->action()));

    configBarLayout->addStretch();

    uiSwitcher->setToolbar(Qml::Constants::LANG_QML, configBar);
}

void QmlInspectorPlugin::activateDebugger(const QString &langName)
{
    if (langName == Qml::Constants::LANG_QML) {
        m_inspector->connectToViewer();
    }
}

void QmlInspectorPlugin::activateDebuggerForProject(ProjectExplorer::Project *project)
{
    QmlProjectManager::QmlProject *qmlproj = qobject_cast<QmlProjectManager::QmlProject*>(project);
    if (qmlproj)
        m_connectionTimer->start();

}
void QmlInspectorPlugin::pollInspector()
{
    ++m_connectionAttempts;
    if (m_inspector->connectToViewer() || m_connectionAttempts == MaxConnectionAttempts) {
        m_connectionTimer->stop();
        m_connectionAttempts = 0;
    }
}

void QmlInspectorPlugin::prepareDebugger(Core::IMode *mode)
{
    if (mode->id() != Debugger::Constants::MODE_DEBUG)
        return;

    Core::EditorManager *editorManager = Core::EditorManager::instance();

    if (editorManager->currentEditor() &&
        editorManager->currentEditor()->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
        Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();
        uiSwitcher->setActiveLanguage(Qml::Constants::LANG_QML);
    }
}

void QmlInspectorPlugin::setDockWidgetArrangement(const QString &activeLanguage)
{
    if (activeLanguage == Qml::Constants::LANG_QML || activeLanguage.isEmpty())
        m_inspector->setSimpleDockWidgetArrangement();
}


Q_EXPORT_PLUGIN(QmlInspectorPlugin)
