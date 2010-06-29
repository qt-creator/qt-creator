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

QmlInspectorPlugin *g_instance = 0; // the global QML/JS inspector instance

QToolButton *createToolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

} // end of anonymous namespace

QmlInspectorPlugin::QmlInspectorPlugin()
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT(! g_instance);
    g_instance = this;
}

QmlInspectorPlugin::~QmlInspectorPlugin()
{
    qDebug() << Q_FUNC_INFO;
}

void QmlInspectorPlugin::aboutToShutdown()
{
    qDebug() << Q_FUNC_INFO;
}

bool QmlInspectorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    qDebug() << Q_FUNC_INFO;

    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            SLOT(prepareDebugger(Core::IMode*)));

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();

    uiSwitcher->addLanguage(LANG_QML, Core::Context(C_INSPECTOR));
#if 0
    m_inspector = new QmlInspector;
    m_inspector->createDockWidgets();
    addObject(m_inspector);
#endif

    return true;
}

void QmlInspectorPlugin::extensionsInitialized()
{
    qDebug() << Q_FUNC_INFO;

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();

    connect(uiSwitcher, SIGNAL(dockArranged(QString)), SLOT(setDockWidgetArrangement(QString)));

    if (ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance()) {
        connect(pex, SIGNAL(aboutToExecuteProject(ProjectExplorer::Project*, QString)),
                SLOT(activateDebuggerForProject(ProjectExplorer::Project*, QString)));
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

    uiSwitcher->setToolbar(LANG_QML, configBar);
}

void QmlInspectorPlugin::activateDebuggerForProject(ProjectExplorer::Project *project, const QString &runMode)
{
    Q_UNUSED(project);
    Q_UNUSED(runMode);

    qDebug() << Q_FUNC_INFO;

    if (runMode == QLatin1String(ProjectExplorer::Constants::DEBUGMODE)) {
#if 0
        // FIXME we probably want to activate the debugger for other projects than QmlProjects,
        // if they contain Qml files. Some kind of options should exist for this behavior.
        QmlProjectManager::QmlProject *qmlproj = qobject_cast<QmlProjectManager::QmlProject*>(project);
        if (qmlproj && m_inspector->setDebugConfigurationDataFromProject(qmlproj))
            m_inspector->startQmlProjectDebugger();
#endif
    }
}

void QmlInspectorPlugin::prepareDebugger(Core::IMode *mode)
{
    qDebug() << Q_FUNC_INFO;

    if (mode->id() != QLatin1String(Debugger::Constants::MODE_DEBUG))
        return;

    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();

    if (pex->startupProject() && pex->startupProject()->id() == QLatin1String("QmlProjectManager.QmlProject")) {
        ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
        Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();
        uiSwitcher->setActiveLanguage(LANG_QML);
    }
}

void QmlInspectorPlugin::setDockWidgetArrangement(const QString &activeLanguage)
{
    Q_UNUSED(activeLanguage);

    qDebug() << Q_FUNC_INFO;

#if 0
    if (activeLanguage == Qml::Constants::LANG_QML || activeLanguage.isEmpty())
        m_inspector->setSimpleDockWidgetArrangement();
#endif
}


Q_EXPORT_PLUGIN(QmlInspectorPlugin)
