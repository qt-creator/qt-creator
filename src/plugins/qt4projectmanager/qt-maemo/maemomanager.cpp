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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "maemomanager.h"
#include "qtversionmanager.h"

#include "maemodeviceconfigurations.h"
#include "maemopackagecreationfactory.h"
#include "maemorunfactories.h"
#include "maemosettingspage.h"
#include "maemotoolchain.h"
#include "maemorunconfiguration.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>

#include <coreplugin/actionmanager/command.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
    namespace Internal {

MaemoManager *MaemoManager::m_instance = 0;

const QSize iconSize = QSize(24, 20);

MaemoManager::MaemoManager()
    : QObject(0)
    , m_runControlFactory(new MaemoRunControlFactory(this))
    , m_runConfigurationFactory(new MaemoRunConfigurationFactory(this))
    , m_packageCreationFactory(new MaemoPackageCreationFactory(this))
    , m_settingsPage(new MaemoSettingsPage(this))
    , m_qemuAction(0)
{
    Q_ASSERT(!m_instance);

    m_instance = this;
    MaemoDeviceConfigurations::instance(this);

    icon.addFile(":/qt-maemo/images/qemu-run.png", iconSize);
    icon.addFile(":/qt-maemo/images/qemu-stop.png", iconSize, QIcon::Normal,
        QIcon::On);

    ExtensionSystem::PluginManager *pluginManager
        = ExtensionSystem::PluginManager::instance();
    pluginManager->addObject(m_runControlFactory);
    pluginManager->addObject(m_runConfigurationFactory);
    pluginManager->addObject(m_packageCreationFactory);
    pluginManager->addObject(m_settingsPage);
}

MaemoManager::~MaemoManager()
{
    ExtensionSystem::PluginManager *pluginManager
        = ExtensionSystem::PluginManager::instance();
    pluginManager->removeObject(m_runControlFactory);
    pluginManager->removeObject(m_runConfigurationFactory);
    pluginManager->removeObject(m_packageCreationFactory);
    pluginManager->removeObject(m_settingsPage);

    m_instance = 0;
}

MaemoManager &MaemoManager::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

bool
MaemoManager::isValidMaemoQtVersion(const Qt4ProjectManager::QtVersion *version) const
{
    QString path = QDir::cleanPath(version->qmakeCommand());
    path = path.remove(QLatin1String("/bin/qmake" EXEC_SUFFIX));

    QDir dir(path);
    dir.cdUp(); dir.cdUp();

    QFile file(dir.absolutePath() + QLatin1String("/cache/madde.conf"));
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString &target = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString &line = stream.readLine().trimmed();
            if (line.startsWith(QLatin1String("target"))
                && line.endsWith(target)) {
                    return true;
            }
        }
    }
    return false;
}

ProjectExplorer::ToolChain*
MaemoManager::maemoToolChain(const QtVersion *version) const
{
    QString targetRoot = QDir::cleanPath(version->qmakeCommand());
    targetRoot.remove(QLatin1String("/bin/qmake" EXEC_SUFFIX));
    return new MaemoToolChain(targetRoot);
}

void
MaemoManager::addQemuSimulatorStarter(Project *project)
{
    if (projects.contains(project))
        return;
    projects.insert(project);

    if (m_qemuAction) {
        m_qemuAction->setVisible(true);
        return;
    }

    m_qemuAction = new QAction("Maemo Emulator", this);
    m_qemuAction->setEnabled(false);
    m_qemuAction->setIcon(icon.pixmap(iconSize));
    m_qemuAction->setToolTip(tr("Start Maemo Emulator"));
    connect(m_qemuAction, SIGNAL(triggered()), this, SLOT(triggered()));

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *actionManager = core->actionManager();
    Core::Command *qemuCommand = actionManager->registerAction(m_qemuAction,
        "MaemoEmulator", QList<int>() << Core::Constants::C_GLOBAL_ID);
    qemuCommand->setAttribute(Core::Command::CA_UpdateText);
    qemuCommand->setAttribute(Core::Command::CA_UpdateIcon);

    Core::ModeManager *modeManager = core->modeManager();
    modeManager->addAction(qemuCommand, 1);
}

void
MaemoManager::removeQemuSimulatorStarter(Project *project)
{
    if (projects.contains(project)) {
        projects.remove(project);
        if (projects.isEmpty() && m_qemuAction)
            m_qemuAction->setVisible(false);
    }
}

void
MaemoManager::setQemuSimulatorStarterEnabled(bool enable)
{
    if (m_qemuAction)
        m_qemuAction->setEnabled(enable);
}

void
MaemoManager::triggered()
{
    emit startStopQemu();
}

void
MaemoManager::qemuStatusChanged(QemuStatus status, const QString &error)
{
    if (!m_qemuAction)
        return;

    bool running;
    QString message;
    switch (status) {
    case QemuStarting:
        running = true;
        break;
    case QemuFailedToStart:
        running = false;
        message = tr("Qemu failed to start: %1").arg(error);
        break;
    case QemuCrashed:
        running = false;
        message = tr("Qemu crashed");
        break;
    case QemuFinished:
        running = false;
        break;
    default:
        Q_ASSERT(!"Missing handling of Qemu status");
    }

    if (!message.isEmpty())
        QMessageBox::warning(0, tr("Qemu error"), message);
    updateQemuIcon(running);
}

void MaemoManager::updateQemuIcon(bool running)
{
    if (!m_qemuAction)
        return;

    QIcon::State state;
    QString toolTip;
    if (running) {
        state = QIcon::On;
        toolTip = tr("Stop Maemo Emulator");
    } else {
        state = QIcon::Off;
        toolTip = tr("Start Maemo Emulator");
    }

    m_qemuAction->setToolTip(toolTip);
    m_qemuAction->setIcon(icon.pixmap(iconSize, QIcon::Normal, state));
}

} // namespace Internal
} // namespace Qt4ProjectManager
