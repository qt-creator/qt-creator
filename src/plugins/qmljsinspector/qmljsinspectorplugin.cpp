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

#include "qmljsinspectorconstants.h"
#include "qmljsinspectorplugin.h"
#include "qmljsinspector.h"
#include "qmljsclientproxy.h"
#include "qmlinspectortoolbar.h"

#include <debugger/debuggeruiswitcher.h>
#include <debugger/debuggerconstants.h>

#include <qmlprojectmanager/qmlproject.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>

#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <QtCore/QStringList>
#include <QtCore/QtPlugin>
#include <QtCore/QTimer>

#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>

#include <QtCore/QDebug>

using namespace QmlJSInspector::Internal;
using namespace QmlJSInspector::Constants;

namespace {

InspectorPlugin *g_instance = 0; // the global QML/JS inspector instance

QToolButton *createToolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

} // end of anonymous namespace

InspectorPlugin::InspectorPlugin()
{
    Q_ASSERT(! g_instance);
    g_instance = this;

    _clientProxy = new ClientProxy(this);
    _inspector = new Inspector(this);
    m_toolbar = new QmlInspectorToolbar(this);
}

InspectorPlugin::~InspectorPlugin()
{
}

QmlJS::ModelManagerInterface *InspectorPlugin::modelManager() const
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

ClientProxy *InspectorPlugin::clientProxy() const
{
    return _clientProxy;
}

InspectorPlugin *InspectorPlugin::instance()
{
    return g_instance;
}

Inspector *InspectorPlugin::inspector() const
{
    return _inspector;
}

ExtensionSystem::IPlugin::ShutdownFlag InspectorPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

bool InspectorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            SLOT(prepareDebugger(Core::IMode*)));

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();

    uiSwitcher->addLanguage(LANG_QML, Core::Context(C_INSPECTOR));

#ifdef __GNUC__
#  warning set up the QML/JS Inspector UI
#endif

#if 0
    _inspector->createDockWidgets();
#endif

    return true;
}

void InspectorPlugin::extensionsInitialized()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();

    connect(uiSwitcher, SIGNAL(dockArranged(QString)), SLOT(setDockWidgetArrangement(QString)));

    if (ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance()) {
        connect(pex, SIGNAL(aboutToExecuteProject(ProjectExplorer::Project*, QString)),
                SLOT(activateDebuggerForProject(ProjectExplorer::Project*, QString)));
    }
    m_toolbar->createActions(Core::Context(C_INSPECTOR));

    connect(_clientProxy, SIGNAL(connected(QDeclarativeEngineDebug*)), m_toolbar, SLOT(enable()));
    connect(_clientProxy, SIGNAL(disconnected()), m_toolbar, SLOT(disable()));

    connect(m_toolbar, SIGNAL(designModeSelected(bool)), _clientProxy, SLOT(setDesignModeBehavior(bool)));
    connect(m_toolbar, SIGNAL(reloadSelected()), _clientProxy, SLOT(reloadQmlViewer()));
    connect(m_toolbar, SIGNAL(animationSpeedChanged(qreal)), _clientProxy, SLOT(setAnimationSpeed(qreal)));
    connect(m_toolbar, SIGNAL(colorPickerSelected()), _clientProxy, SLOT(changeToColorPickerTool()));
    connect(m_toolbar, SIGNAL(zoomToolSelected()), _clientProxy, SLOT(changeToZoomTool()));
    connect(m_toolbar, SIGNAL(selectToolSelected()), _clientProxy, SLOT(changeToSelectTool()));
    connect(m_toolbar, SIGNAL(marqueeSelectToolSelected()), _clientProxy, SLOT(changeToSelectMarqueeTool()));
    connect(m_toolbar, SIGNAL(applyChangesFromQmlFileTriggered(bool)), _inspector, SLOT(setApplyChangesToQmlObserver(bool)));

    connect(_clientProxy, SIGNAL(colorPickerActivated()), m_toolbar, SLOT(activateColorPicker()));
    connect(_clientProxy, SIGNAL(selectToolActivated()), m_toolbar, SLOT(activateSelectTool()));
    connect(_clientProxy, SIGNAL(selectMarqueeToolActivated()), m_toolbar, SLOT(activateMarqueeSelectTool()));
    connect(_clientProxy, SIGNAL(zoomToolActivated()), m_toolbar, SLOT(activateZoomTool()));
    connect(_clientProxy, SIGNAL(designModeBehaviorChanged(bool)), m_toolbar, SLOT(setDesignModeBehavior(bool)));
    connect(_clientProxy, SIGNAL(selectedColorChanged(QColor)), m_toolbar, SLOT(setSelectedColor(QColor)));

    connect(_clientProxy, SIGNAL(animationSpeedChanged(qreal)), m_toolbar, SLOT(changeAnimationSpeed(qreal)));
}

void InspectorPlugin::activateDebuggerForProject(ProjectExplorer::Project *project, const QString &runMode)
{
    Q_UNUSED(project);
    Q_UNUSED(runMode);

    if (runMode == QLatin1String(ProjectExplorer::Constants::DEBUGMODE)) {
#ifdef __GNUC__
#  warning start a QML/JS debugging session using the information stored in the current project
#endif

        // FIXME we probably want to activate the debugger for other projects than QmlProjects,
        // if they contain Qml files. Some kind of options should exist for this behavior.
        if (QmlProjectManager::QmlProject *qmlproj = qobject_cast<QmlProjectManager::QmlProject*>(project)) {
            if (_inspector->setDebugConfigurationDataFromProject(qmlproj))
                _inspector->startQmlProjectDebugger();
        }
    }
}

void InspectorPlugin::prepareDebugger(Core::IMode *mode)
{
    if (mode->id() != QLatin1String(Debugger::Constants::MODE_DEBUG))
        return;

    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();

    if (pex->startupProject() && pex->startupProject()->id() == QLatin1String("QmlProjectManager.QmlProject")) {
        ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
        Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();
        uiSwitcher->setActiveLanguage(LANG_QML);
    }
}

void InspectorPlugin::setDockWidgetArrangement(const QString &activeLanguage)
{
    Q_UNUSED(activeLanguage);

#if 0
    if (activeLanguage == Qml::Constants::LANG_QML || activeLanguage.isEmpty())
        m_inspector->setSimpleDockWidgetArrangement();
#endif
}


Q_EXPORT_PLUGIN(InspectorPlugin)
