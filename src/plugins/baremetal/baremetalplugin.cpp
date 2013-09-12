/****************************************************************************
**
** Copyright (C) 2013 Tim Sander <tim@krieglstein.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baremetalplugin.h"
#include "baremetalconstants.h"
#include "baremetaldeviceconfigurationfactory.h"
#include "baremetalruncontrolfactory.h"
#include "baremetalrunconfigurationfactory.h"
#include "baremetaldeployconfigurationfactory.h"
#include "baremetaldeploystepfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QtPlugin>

namespace BareMetal {
namespace Internal {

BareMetalPlugin::BareMetalPlugin()
{
   // Create your members
    setObjectName(QLatin1String("BareMetalPlugin"));
}

BareMetalPlugin::~BareMetalPlugin()
{
   // Unregister objects from the plugin manager's object pool
   // Delete members
}

bool BareMetalPlugin::initialize(const QStringList &arguments, QString *errorString)
{
   // Register objects in the plugin manager's object pool
   // Load settings
   // Add actions to menus
   // Connect to other plugins' signals
   // In the initialize method, a plugin can be sure that the plugins it
   // depends on have initialized their members.

   Q_UNUSED(arguments)
   Q_UNUSED(errorString)

   addAutoReleasedObject(new BareMetalDeviceConfigurationFactory);
   addAutoReleasedObject(new BareMetalRunControlFactory);
   addAutoReleasedObject(new BareMetalRunConfigurationFactory);
   addAutoReleasedObject(new BareMetalDeployConfigurationFactory);
   //addAutoReleasedObject(new BareMetalDeployStepFactory);

    /*
   QAction *action = new QAction(tr("BareMetal action"), this);
   Core::Command *cmd = Core::ActionManager::registerAction(action, Constants::ACTION_ID,
                                                            Core::Context(Core::Constants::C_GLOBAL));
   cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Meta+A")));
   connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

   Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
   menu->menu()->setTitle(tr("BareMetal"));
   menu->addAction(cmd);
   Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);
   */

   return true;
}

void BareMetalPlugin::extensionsInitialized()
{
   // Retrieve objects from the plugin manager's object pool
   // In the extensionsInitialized method, a plugin can be sure that all
   // plugins that depend on it are completely initialized.
}

/*
ExtensionSystem::IPlugin::ShutdownFlag BareMetalPlugin::aboutToShutdown()
{
   // Save settings
   // Disconnect from signals that are not needed during shutdown
   // Hide UI (if you add UI that is not in the main window directly)
   return SynchronousShutdown;
}

void BareMetalPlugin::triggerAction()
{
   QMessageBox::information(Core::ICore::mainWindow(),
                            tr("Action triggered"),
                            tr("This is an action from BareMetal."));
}
*/

} // namespace Internal
} // namespace BareMetal

Q_EXPORT_PLUGIN(BareMetal::Internal::BareMetalPlugin)

