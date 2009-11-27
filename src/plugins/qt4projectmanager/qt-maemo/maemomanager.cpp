/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "maemomanager.h"

#include "maemosettingspage.h"
#include "maemotoolchain.h"
#include "maemorunconfiguration.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QList>
#include <QtCore/QMutexLocker>

#include <QtGui/QAction>

namespace Qt4ProjectManager {
    namespace Internal {


QMutex MaemoManager::m_mutex;
MaemoManager *MaemoManager::m_instance = 0;

const QSize iconSize = QSize(24, 20);

MaemoManager::MaemoManager()
    : QObject(0)
    , m_runControlFactory(new MaemoRunControlFactory(this))
    , m_runConfigurationFactory(new MaemoRunConfigurationFactory(this))
    , m_settingsPage(new MaemoSettingsPage(this))
    , m_qemuCommand(0)
{

    icon.addFile(":/qt-maemo/images/qemu-run.png", iconSize);
    icon.addFile(":/qt-maemo/images/qemu-stop.png", iconSize, QIcon::Normal,
        QIcon::On);

    ExtensionSystem::PluginManager::instance()->addObject(m_runControlFactory);
    ExtensionSystem::PluginManager::instance()->addObject(m_runConfigurationFactory);
    ExtensionSystem::PluginManager::instance()->addObject(m_settingsPage);
}

MaemoManager::~MaemoManager()
{
    ExtensionSystem::PluginManager::instance()->removeObject(m_runControlFactory);
    ExtensionSystem::PluginManager::instance()->removeObject(m_runConfigurationFactory);
    ExtensionSystem::PluginManager::instance()->removeObject(m_settingsPage);
}

MaemoManager *MaemoManager::instance()
{
    if (!m_instance) {
        QMutexLocker _(&m_mutex);
        if (!m_instance)
            m_instance = new MaemoManager;
    }
    return m_instance;
}

ProjectExplorer::ToolChain*
MaemoManager::maemoToolChain(const QtVersion *version) const
{
    return new MaemoToolChain(version);
}

void
MaemoManager::addQemuSimulatorStarter(Project *project)
{
    projects.insert(project);

    if (m_qemuCommand) {
        m_qemuCommand->action()->setVisible(true);
        return;
    }

    Core::ICore *core = Core::ICore::instance();
    Core::ModeManager *modeManager = core->modeManager();
    Core::ActionManager *actionManager = core->actionManager();

    QAction *action = new QAction("Qemu", this);
    action->setIcon(icon.pixmap(iconSize));
    action->setToolTip(tr("Start Qemu"));
    m_qemuCommand = actionManager->registerAction(action, "qemu",
        QList<int>() << Core::Constants::C_GLOBAL_ID);
    modeManager->addAction(m_qemuCommand, 1);
    m_qemuCommand->action()->setEnabled(true);
    m_qemuCommand->setAttribute(Core::Command::CA_UpdateText);
    m_qemuCommand->setAttribute(Core::Command::CA_UpdateIcon);

    connect(m_qemuCommand->action(), SIGNAL(triggered()), this, SLOT(triggered()));
}

void
MaemoManager::removeQemuSimulatorStarter(Project *project)
{
    projects.remove(project);
    if (projects.isEmpty() && m_qemuCommand)
        m_qemuCommand->action()->setVisible(false);
}

void
MaemoManager::setQemuSimulatorStarterEnabled(bool enable)
{
    if (m_qemuCommand)
        m_qemuCommand->action()->setEnabled(enable);
}

void
MaemoManager::triggered()
{
    emit startStopQemu();
}

void
MaemoManager::updateQemuSimulatorStarter(bool running)
{
    if (m_qemuCommand) {
        QIcon::State state = QIcon::Off;
        QString toolTip(tr("Start Qemu"));
        if (running) {
            state = QIcon::On;
            toolTip = tr("Stop Qemu");
        }

        QAction *action = m_qemuCommand->action();
        action->setToolTip(toolTip);
        action->setIcon(icon.pixmap(iconSize, QIcon::Normal, state));
    }
}

    } // namespace Internal
} // namespace Qt4ProjectManager
